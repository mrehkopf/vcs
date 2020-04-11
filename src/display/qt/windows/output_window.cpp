/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS main qt window
 *
 * The program's main window. About half of what it does is display the filtered and
 * scaled captured frames, and the other half is relay messages from the non-ui parts
 * of the program to the control panel dialog, which needs to reflect changes in the
 * rest of the system (e.g. user-facing information about the current capture resolution).
 *
 */

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QTextDocument>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QShortcut>
#include <QPainter>
#include <QScreen>
#include <QImage>
#include <QLabel>
#include <cmath>
#include "display/qt/subclasses/QOpenGLWidget_opengl_renderer.h"
#include "display/qt/dialogs/output_resolution_dialog.h"
#include "display/qt/dialogs/video_parameter_dialog.h"
#include "display/qt/dialogs/input_resolution_dialog.h"
#include "display/qt/dialogs/signal_dialog.h"
#include "display/qt/dialogs/filter_graph_dialog.h"
#include "display/qt/dialogs/resolution_dialog.h"
#include "display/qt/dialogs/anti_tear_dialog.h"
#include "display/qt/dialogs/overlay_dialog.h"
#include "display/qt/dialogs/record_dialog.h"
#include "display/qt/windows/output_window.h"
#include "display/qt/dialogs/alias_dialog.h"
#include "display/qt/dialogs/about_dialog.h"
#include "display/qt/persistent_settings.h"
#include "filter/anti_tear.h"
#include "common/propagate/propagate.h"
#include "capture/capture_api.h"
#include "capture/capture.h"
#include "capture/alias.h"
#include "common/globals.h"
#include "record/record.h"
#include "scaler/scaler.h"
#include "ui_output_window.h"

/// Temp. Stores the number of milliseconds passed for each frame update. This
/// number includes everything done to the frame - capture, scaling, and display.
int UPDATE_LATENCY_PEAK = 0;
int UPDATE_LATENCY_AVG = 0;

/// Temporary.
uint CURRENT_OUTPUT_FRAMERATE = 0;

// Set to true to have the capture's horizontal and vertical positions be
// adjusted next frame to align the captured image with the edges of the
// screen.
bool ALIGN_CAPTURE = false;

// For an optional OpenGL render surface.
static OGLWidget *OGL_SURFACE = nullptr;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowFlags(this->defaultWindowFlags);

    ui->centralwidget->setMouseTracking(true);
    ui->centralwidget->setFocusPolicy(Qt::StrongFocus);

    ui->menuBar->setMouseTracking(true);
    ui->menuBar->setFocusPolicy(Qt::StrongFocus);

    // Set up a layout for the central widget, so we can add the OpenGL
    // render surface to it when OpenGL is enabled.
    QVBoxLayout *const mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);

    ui->menuBar->setVisible(false);

    // Set up the child dialogs.
    {
        outputResolutionDlg = new OutputResolutionDialog;
        inputResolutionDlg = new InputResolutionDialog;
        videoParamDlg = new VideoParameterDialog;
        filterGraphDlg = new FilterGraphDialog;
        antitearDlg = new AntiTearDialog;
        overlayDlg = new OverlayDialog;
        signalDlg = new SignalDialog;
        aliasDlg = new AliasDialog;
        aboutDlg = new AboutDialog;
        recordDlg = new RecordDialog;

        this->dialogs << outputResolutionDlg
                      << inputResolutionDlg
                      << filterGraphDlg
                      << videoParamDlg
                      << antitearDlg
                      << overlayDlg
                      << signalDlg
                      << aliasDlg
                      << aboutDlg
                      << recordDlg;
    }

    // Create the window's menu bar.
    {
        std::vector<QMenu*> menus;

        // File...
        {
            QMenu *menu = new QMenu("File", this);
            menus.push_back(menu);

            connect(menu->addAction("Exit"), &QAction::triggered, this, [=]{this->close();});
        }

        // Window...
        {
            QMenu *menu = new QMenu("Window", this);
            menus.push_back(menu);

            {
                QMenu *rendererMenu = new QMenu("Renderer", this);

                QActionGroup *group = new QActionGroup(this);

                QAction *opengl = new QAction("OpenGL", this);
                opengl->setActionGroup(group);
                opengl->setCheckable(true);
                rendererMenu->addAction(opengl);

                QAction *software = new QAction("Software", this);
                software->setActionGroup(group);
                software->setCheckable(true);
                rendererMenu->addAction(software);

                connect(this, &MainWindow::entered_fullscreen, this, [=]
                {
                    rendererMenu->setEnabled(false);
                });

                connect(this, &MainWindow::left_fullscreen, this, [=]
                {
                    rendererMenu->setEnabled(true);
                });

                connect(opengl, &QAction::triggered, this, [=]
                {
                    this->set_opengl_enabled(true);
                });

                connect(software, &QAction::triggered, this, [=]
                {
                    this->set_opengl_enabled(false);
                });

                if (kpers_value_of(INI_GROUP_OUTPUT, "renderer", "Software").toString() == "Software")
                {
                    software->setChecked(true);
                }
                else
                {
                    opengl->setChecked(true);
                }

                menu->addMenu(rendererMenu);
            }

            {
                menu->addSeparator();

                QMenu *positionMenu = new QMenu("Position", this);

                connect(this, &MainWindow::entered_fullscreen, this, [=]
                {
                    positionMenu->setEnabled(false);
                });

                connect(this, &MainWindow::left_fullscreen, this, [=]
                {
                    positionMenu->setEnabled(true);
                });

                connect(positionMenu->addAction("Center"), &QAction::triggered, this, [=]
                {
                    this->move(this->pos() + (QGuiApplication::primaryScreen()->geometry().center() - this->geometry().center()));
                });

                QAction *topLeft = new QAction("Top left", this);
                topLeft->setShortcut(QKeySequence("f2"));
                positionMenu->addAction(topLeft);
                connect(topLeft, &QAction::triggered, this, [=]
                {
                    this->move(0, 0);
                });

                menu->addMenu(positionMenu);
            }

            {
                QAction *customTitle = new QAction("Custom title...", this);

                connect(customTitle, &QAction::triggered, this, [=]
                {
                    const QString newTitle = QInputDialog::getText(this,
                                                                   "VCS - Enter a custom window title",
                                                                   "Title (empty to restore default):",
                                                                   QLineEdit::Normal,
                                                                   this->windowTitleOverride);

                    if (!newTitle.isNull())
                    {
                        this->windowTitleOverride = newTitle;
                        this->update_window_title();
                    }
                });

                connect(this, &MainWindow::entered_fullscreen, this, [=]
                {
                    customTitle->setEnabled(false);
                });

                connect(this, &MainWindow::left_fullscreen, this, [=]
                {
                    customTitle->setEnabled(true);
                });

                menu->addAction(customTitle);
            }

            {
                menu->addSeparator();

                QAction *showBorder = new QAction("Show border", this);

                showBorder->setCheckable(true);
                showBorder->setChecked(this->window_has_border());
                showBorder->setShortcut(QKeySequence("f1"));

                connect(this, &MainWindow::border_hidden, this, [=]
                {
                    showBorder->setChecked(false);
                });

                connect(this, &MainWindow::border_revealed, this, [=]
                {
                    showBorder->setChecked(true);
                });

                connect(this, &MainWindow::entered_fullscreen, this, [=]
                {
                    showBorder->setEnabled(false);
                });

                connect(this, &MainWindow::left_fullscreen, this, [=]
                {
                    showBorder->setEnabled(true);
                });

                connect(showBorder, &QAction::triggered, this, [this]
                {
                    this->toggle_window_border();
                });

                menu->addAction(showBorder);
            }

            {
                menu->addSeparator();

                QAction *fullscreen = new QAction("Fullscreen", this);

                fullscreen->setCheckable(true);
                fullscreen->setChecked(this->isFullScreen());
                fullscreen->setShortcut(QKeySequence("f11"));

                connect(this, &MainWindow::entered_fullscreen, this, [=]
                {
                    fullscreen->setChecked(true);
                });

                connect(this, &MainWindow::left_fullscreen, this, [=]
                {
                    fullscreen->setChecked(false);
                });

                connect(fullscreen, &QAction::triggered, this, [this]
                {
                    if (this->isFullScreen())
                    {
                        this->showNormal();
                    }
                    else
                    {
                        this->showFullScreen();
                    }
                });

                menu->addAction(fullscreen);
            }
        }

        // Input...
        {
            QMenu *menu = new QMenu("Input", this);
            menus.push_back(menu);

            QMenu *channel = new QMenu("Channel", this);
            {
                QActionGroup *group = new QActionGroup(this);

                for (int i = 0; i < kc_capture_api().get_device_maximum_input_count(); i++)
                {
                    QAction *inputChannel = new QAction(QString::number(i+1), this);
                    inputChannel->setActionGroup(group);
                    inputChannel->setCheckable(true);
                    channel->addAction(inputChannel);

                    if (i == int(INPUT_CHANNEL_IDX))
                    {
                        inputChannel->setChecked(true);
                    }

                    connect(inputChannel, &QAction::triggered, this, [=]{kc_capture_api().set_input_channel(i);});
                }
            }

            QMenu *colorDepth = new QMenu("Color depth", this);
            {
                QActionGroup *group = new QActionGroup(this);

                QAction *c24 = new QAction("24-bit (RGB-888)", this);
                c24->setActionGroup(group);
                c24->setCheckable(true);
                c24->setChecked(true);
                colorDepth->addAction(c24);

                // Note: on Linux, we don't currently support any other capture color depth than RGB888.
                #ifndef __linux__
                    QAction *c16 = new QAction("16-bit (RGB-565)", this);
                    c16->setActionGroup(group);
                    c16->setCheckable(true);
                    colorDepth->addAction(c16);

                    QAction *c15 = new QAction("15-bit (RGB-555)", this);
                    c15->setActionGroup(group);
                    c15->setCheckable(true);
                    colorDepth->addAction(c15);

                    connect(c16, &QAction::triggered, this, [=]{kc_capture_api().set_pixel_format(capture_pixel_format_e::rgb_565);});
                    connect(c15, &QAction::triggered, this, [=]{kc_capture_api().set_pixel_format(capture_pixel_format_e::rgb_555);});
                #endif
                connect(c24, &QAction::triggered, this, [=]{kc_capture_api().set_pixel_format(capture_pixel_format_e::rgb_888);});
            }

            menu->addMenu(channel);
            menu->addSeparator();
            menu->addMenu(colorDepth);
            menu->addSeparator();

            connect(menu->addAction("Aliases..."), &QAction::triggered, this, [=]{this->open_alias_dialog();});

            QAction *resolution = new QAction("Resolution...", this);
            resolution->setShortcut(QKeySequence("ctrl+i"));
            menu->addAction(resolution);

            QAction *signal = new QAction("Signal info...", this);
            signal->setShortcut(QKeySequence("ctrl+s"));
            menu->addAction(signal);

            QAction *videoParams = new QAction("Video presets...", this);
            videoParams->setShortcut(QKeySequence("ctrl+v"));
            menu->addAction(videoParams);

            connect(signal, &QAction::triggered, this, [=]{this->open_video_dialog();});
            connect(videoParams, &QAction::triggered, this, [=]{this->open_video_parameter_graph_dialog();});
            connect(resolution, &QAction::triggered, this, [=]{this->open_input_resolution_dialog();});
        }

        // Output...
        {
            QMenu *menu = new QMenu("Output", this);
            menus.push_back(menu);

            const std::vector<std::string> scalerNames = ks_list_of_scaling_filter_names();
            k_assert(!scalerNames.empty(), "Expected to receive a list of scalers, but got an empty list.");

            QMenu *upscaler = new QMenu("Upscaler", this);
            {
                QActionGroup *group = new QActionGroup(this);

                const QString defaultUpscalerName = kpers_value_of(INI_GROUP_OUTPUT, "upscaler", "Linear").toString();

                for (const auto &scalerName: scalerNames)
                {
                    QAction *scaler = new QAction(QString::fromStdString(scalerName), this);
                    scaler->setActionGroup(group);
                    scaler->setCheckable(true);
                    upscaler->addAction(scaler);

                    connect(scaler, &QAction::toggled, this, [=](const bool checked){if (checked) ks_set_upscaling_filter(scalerName);});

                    if (QString::fromStdString(scalerName) == defaultUpscalerName)
                    {
                        scaler->setChecked(true);
                    }
                }
            }

            QMenu *downscaler = new QMenu("Downscaler", this);
            {
                QActionGroup *group = new QActionGroup(this);

                const QString defaultDownscalerName = kpers_value_of(INI_GROUP_OUTPUT, "downscaler", "Linear").toString();

                for (const auto &scalerName: scalerNames)
                {
                    QAction *scaler = new QAction(QString::fromStdString(scalerName), this);
                    scaler->setActionGroup(group);
                    scaler->setCheckable(true);
                    downscaler->addAction(scaler);

                    connect(scaler, &QAction::toggled, this, [=](const bool checked){if (checked) ks_set_downscaling_filter(scalerName);});


                    if (QString::fromStdString(scalerName) == defaultDownscalerName)
                    {
                        scaler->setChecked(true);
                    }
                }
            }

            QMenu *aspectRatio = new QMenu("Aspect ratio", this);
            {
                QActionGroup *group = new QActionGroup(this);

                QAction *native = new QAction("Native", this);
                native->setActionGroup(group);
                native->setCheckable(true);
                aspectRatio->addAction(native);

                QAction *always43 = new QAction("Always 4:3", this);
                always43->setActionGroup(group);
                always43->setCheckable(true);
                aspectRatio->addAction(always43);

                QAction *traditional43 = new QAction("Traditional 4:3", this);
                traditional43->setActionGroup(group);
                traditional43->setCheckable(true);
                aspectRatio->addAction(traditional43);

                const QString defaultAspectRatio = kpers_value_of(INI_GROUP_OUTPUT, "aspect_mode", "Native").toString();

                connect(native, &QAction::toggled, this,
                        [=](const bool checked){if (checked) { ks_set_forced_aspect_enabled(false); ks_set_aspect_mode(aspect_mode_e::native);}});
                connect(traditional43, &QAction::toggled, this,
                        [=](const bool checked){if (checked) { ks_set_forced_aspect_enabled(true); ks_set_aspect_mode(aspect_mode_e::traditional_4_3);}});
                connect(always43, &QAction::toggled, this,
                        [=](const bool checked){if (checked) { ks_set_forced_aspect_enabled(true); ks_set_aspect_mode(aspect_mode_e::always_4_3);}});

                if (defaultAspectRatio == "Native") native->setChecked(true);
                else if (defaultAspectRatio == "Always 4:3") always43->setChecked(true);
                else if (defaultAspectRatio == "Traditional 4:3") traditional43->setChecked(true);
            }

            menu->addMenu(aspectRatio);
            menu->addSeparator();
            menu->addMenu(upscaler);
            menu->addMenu(downscaler);
            menu->addSeparator();

            QAction *record = new QAction("Record...", this);
            record->setShortcut(QKeySequence("ctrl+r"));
            menu->addAction(record);
            connect(record, &QAction::triggered, this, [=]{this->open_record_dialog();});

            QAction *overlay = new QAction("Overlay...", this);
            overlay->setShortcut(QKeySequence("ctrl+l"));
            menu->addAction(overlay);
            connect(overlay, &QAction::triggered, this, [=]{this->open_overlay_dialog();});

            QAction *antiTear = new QAction("Anti-tear...", this);
            antiTear->setShortcut(QKeySequence("ctrl+a"));
            menu->addAction(antiTear);
            connect(antiTear, &QAction::triggered, this, [=]{this->open_antitear_dialog();});

            QAction *resolution = new QAction("Resolution...", this);
            resolution->setShortcut(QKeySequence("ctrl+o"));
            menu->addAction(resolution);
            connect(resolution, &QAction::triggered, this, [=]{this->open_output_resolution_dialog();});

            QAction *filter = new QAction("Filter graph...", this);
            filter->setShortcut(QKeySequence("ctrl+f"));
            menu->addAction(filter);
            connect(filter, &QAction::triggered, this, [=]{this->open_filter_graph_dialog();});
        }

        // Help...
        {
            QMenu *menu = new QMenu("Help", this);
            menus.push_back(menu);

            connect(menu->addAction("About..."), &QAction::triggered, this, [=]{this->open_about_dialog();});
        }

        for (QMenu *const menu: menus)
        {
            ui->menuBar->addMenu(menu);
            connect(menu, &QMenu::aboutToHide, this, [=]{ui->centralwidget->setFocus(); this->mouseActivityMonitor.report_activity();});

            // Ensure that action shortcuts can be used regardless of which dialog
            // has focus.
            this->addActions(menu->actions());
            for (auto dialog: this->dialogs)
            {
                dialog->addActions(menu->actions());
            }
        }

        connect(this->menuBar(), &QMenuBar::hovered, this, [=]{this->mouseActivityMonitor.report_activity();});
    }

    // We intend to repaint the entire window every time we update it, so ask for no automatic fill.
    this->setAttribute(Qt::WA_OpaquePaintEvent, true);

    update_window_size();
    update_window_title();
    set_keyboard_shortcuts();

#ifdef _WIN32
    /// Temp hack. On my system, need to toggle this or there'll be a 2-pixel
    /// gap between the frame and the bottom corner of the window.
    toggle_window_border();
    toggle_window_border();
#endif

    this->activateWindow();
    this->raise();

    // Check over the network whether there are updates available for VCS.
    {
        QNetworkAccessManager *network = new QNetworkAccessManager(this);
        connect(network, &QNetworkAccessManager::finished, [=](QNetworkReply *reply)
        {
            if (reply->error() == QNetworkReply::NoError)
            {
                bool isNewVersionAvailable = reply->readAll().trimmed().toInt();

                if (isNewVersionAvailable)
                {
                    INFO(("A newer version of VCS is available."));

                    if (this->aboutDlg)
                    {
                        this->aboutDlg->notify_of_new_program_version();
                    }
                }
            }
            else
            {
                /// TODO. Handle the error.
                return;
            }
        });

        // Make the request.
        if (!DEV_VERSION)
        {
            const QString url = QString("http://www.tarpeeksihyvaesoft.com/vcs/is_newer_version_available.php?uv=%1").arg(PROGRAM_VERSION_STRING);
            network->get(QNetworkRequest(QUrl(url)));
        }
    }

    connect(&this->mouseActivityMonitor, &mouse_activity_monitor_c::activated, this, [=]
    {
        this->menuBar()->setVisible(true);
    });

    connect(&this->mouseActivityMonitor, &mouse_activity_monitor_c::inactivated, this, [=]
    {
        // Hide the menu bar only if the mouse cursor isnät hovering over it at the moment.
        if (!this->menuBar()->hasFocus())
        {
            this->menuBar()->setVisible(false);
        }
    });

    return;
}

MainWindow::~MainWindow()
{
    // Save persistent settings.
    {
        const QString aspectMode = []()->QString
        {
            switch (ks_aspect_mode())
            {
                case aspect_mode_e::native: return "Native";
                case aspect_mode_e::always_4_3: return "Always 4:3";
                case aspect_mode_e::traditional_4_3: return "Traditional 4:3";
                default: return "(Unknown)";
            }
        }();

        kpers_set_value(INI_GROUP_OUTPUT, "aspect_mode", aspectMode);
        kpers_set_value(INI_GROUP_OUTPUT, "renderer", (OGL_SURFACE? "OpenGL" : "Software"));
        kpers_set_value(INI_GROUP_OUTPUT, "upscaler", QString::fromStdString(ks_upscaling_filter_name()));
        kpers_set_value(INI_GROUP_OUTPUT, "downscaler", QString::fromStdString(ks_downscaling_filter_name()));
    }

    delete ui;
    ui = nullptr;

    for (auto dialog: this->dialogs)
    {
        delete dialog;
        dialog = nullptr;
    }

    return;
}

bool MainWindow::load_font(const QString &filename)
{
    if (QFontDatabase::addApplicationFont(filename) == -1)
    {
        NBENE(("Failed to load the font '%s'.", filename.toStdString().c_str()));
        return false;
    }

    return true;
}

void MainWindow::open_filter_graph_dialog(void)
{
    k_assert(this->filterGraphDlg != nullptr, "");
    this->filterGraphDlg->show();
    this->filterGraphDlg->activateWindow();
    this->filterGraphDlg->raise();

    return;
}

void MainWindow::open_output_resolution_dialog(void)
{
    k_assert(this->outputResolutionDlg != nullptr, "");
    this->outputResolutionDlg->show();
    this->outputResolutionDlg->activateWindow();
    this->outputResolutionDlg->raise();

    return;
}

void MainWindow::open_input_resolution_dialog(void)
{
    k_assert(this->inputResolutionDlg != nullptr, "");
    this->inputResolutionDlg->show();
    this->inputResolutionDlg->activateWindow();
    this->inputResolutionDlg->raise();

    return;
}

void MainWindow::open_video_parameter_graph_dialog(void)
{
    k_assert(this->videoParamDlg != nullptr, "");
    this->videoParamDlg->show();
    this->videoParamDlg->activateWindow();
    this->videoParamDlg->raise();

    return;
}

void MainWindow::open_video_dialog(void)
{
    k_assert(this->signalDlg != nullptr, "");
    this->signalDlg->show();
    this->signalDlg->activateWindow();
    this->signalDlg->raise();

    return;
}

void MainWindow::open_overlay_dialog(void)
{
    k_assert(this->overlayDlg != nullptr, "");
    this->overlayDlg->show();
    this->overlayDlg->activateWindow();
    this->overlayDlg->raise();

    return;
}

void MainWindow::open_record_dialog(void)
{
    k_assert(this->recordDlg != nullptr, "");
    this->recordDlg->show();
    this->recordDlg->activateWindow();
    this->recordDlg->raise();

    return;
}

void MainWindow::open_about_dialog(void)
{
    k_assert(this->aboutDlg != nullptr, "");
    this->aboutDlg->show();
    this->aboutDlg->activateWindow();
    this->aboutDlg->raise();

    return;
}

void MainWindow::open_alias_dialog(void)
{
    k_assert(this->aliasDlg != nullptr, "");
    this->aliasDlg->show();
    this->aliasDlg->activateWindow();
    this->aliasDlg->raise();

    return;
}

void MainWindow::open_antitear_dialog(void)
{
    k_assert(this->antitearDlg != nullptr, "");
    this->antitearDlg->show();
    this->antitearDlg->activateWindow();
    this->antitearDlg->raise();

    return;
}

void MainWindow::refresh(void)
{
    if (OGL_SURFACE != nullptr)
    {
        OGL_SURFACE->repaint();
    }
    else this->repaint();

    return;
}

void MainWindow::set_opengl_enabled(const bool enabled)
{
    if (enabled)
    {
        INFO(("Renderer: OpenGL."));

        k_assert((OGL_SURFACE == nullptr), "Can't doubly enable OpenGL.");

        OGL_SURFACE = new OGLWidget(std::bind(&MainWindow::overlay_image, this), this);
        OGL_SURFACE->show();
        OGL_SURFACE->raise();

        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setVersion(1, 2);
        format.setSwapInterval(0); // Vsync.
        format.setSamples(0);
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
        format.setRenderableType(QSurfaceFormat::OpenGL);
        QSurfaceFormat::setDefaultFormat(format);

        ui->centralwidget->layout()->addWidget(OGL_SURFACE);
    }
    else
    {
        INFO(("Renderer: Software."));

        ui->centralwidget->layout()->removeWidget(OGL_SURFACE);
        delete OGL_SURFACE;
        OGL_SURFACE = nullptr;
    }

    return;
}

void MainWindow::closeEvent(QCloseEvent*)
{
    PROGRAM_EXIT_REQUESTED = 1;

    return;
}

void MainWindow::enterEvent(QEvent *event)
{
    if (ui->centralwidget->hasFocus())
    {
        this->mouseActivityMonitor.report_activity();
    }

    (void)event;

    return;
}

void MainWindow::leaveEvent(QEvent *event)
{
    (void)event;

    return;
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        toggle_window_border();
    }

    return;
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        // Hide the window's cursor when in full-screen mode, and restore it when not.
        /// TODO: Ideally, this would only get set when we enter full-screen
        /// mode and back, not every time the window's state changes.
        if (this->windowState() & Qt::WindowFullScreen)
        {
            emit this->entered_fullscreen();
            this->setCursor(QCursor(Qt::BlankCursor));
        }
        else
        {
            emit this->left_fullscreen();
            this->unsetCursor();
        }
    }

    QWidget::changeEvent(event);
}

static QPoint PREV_MOUSE_POS; /// Temp. Used to track mouse movement delta across frames.
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (ui->centralwidget->hasFocus())
    {
        this->mouseActivityMonitor.report_activity();
    }

    // If the cursor is over the capture window and the left mouse button is being
    // held down, drag the window. Note: We disable dragging in fullscreen mode,
    // since really you shouldn't be able to drag a fullscreen window.
    if ((QApplication::mouseButtons() & Qt::LeftButton) &&
        !this->isFullScreen())
    {
        QPoint delta = (event->globalPos() - PREV_MOUSE_POS);
        this->move(pos() + delta);
    }

    PREV_MOUSE_POS = event->globalPos();

    return;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    this->mouseActivityMonitor.report_activity();

    ui->centralwidget->setFocus();

    if (event->button() == Qt::LeftButton)
    {
        PREV_MOUSE_POS = event->globalPos();
    }

    return;
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    this->mouseActivityMonitor.report_activity();

    if (event->button() == Qt::MidButton)
    {
        // If the capture window is pressed with the middle mouse button, show the overlay dialog.
        open_overlay_dialog();
    }

    return;
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if ((this->outputResolutionDlg != nullptr) &&
        this->is_mouse_wheel_scaling_allowed())
    {
        // Adjust the size of the capture window with the mouse scroll wheel.
        const int dir = (event->angleDelta().y() < 0)? 1 : -1;
        this->outputResolutionDlg->adjust_output_scaling(dir);
    }

    return;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Escape) &&
        this->isFullScreen())
    {
        this->showNormal();
    }

    return;
}

bool MainWindow::is_mouse_wheel_scaling_allowed(void)
{
    return (!kd_is_fullscreen() && // On my virtual machine, at least, wheel scaling while in full-screen messes up the full-screen mode.
            !krecord_is_recording());
}

QImage MainWindow::overlay_image(void)
{
    if (!kc_capture_api().has_no_signal() &&
        overlayDlg != nullptr &&
        overlayDlg->is_overlay_enabled())
    {
        return overlayDlg->overlay_as_qimage();
    }
    else return QImage();
}

void MainWindow::paintEvent(QPaintEvent *)
{
    // If OpenGL is enabled, its own paintGL() should be getting called instead of paintEvent().
    if (OGL_SURFACE != nullptr) return;

    // Convert the output buffer into a QImage frame.
    const QImage frameImage = ([]()->QImage
    {
        const resolution_s r = ks_output_resolution();
        const u8 *const fb = ks_scaler_output_as_raw_ptr();

        if (fb == nullptr)
        {
            DEBUG(("Requested the scaler output as a QImage while the scaler's output buffer was uninitialized."));
            return QImage();
        }
        else return QImage(fb, r.w, r.h, QImage::Format_RGB32);
    })();

    QPainter painter(this);

    // Draw the frame.
    if (!frameImage.isNull())
    {
        painter.drawImage(0, 0, frameImage);
    }

    // Draw the overlay.
    const QImage overlayImg = overlay_image();
    if (!overlayImg.isNull())
    {
        painter.drawImage(0, 0, overlayImg);
    }

    // Show a magnifying glass effect which blows up part of the captured image.
    static QLabel *magnifyingGlass = nullptr;
    if (!kc_capture_api().has_no_signal() &&
        this->isActiveWindow() &&
        this->rect().contains(this->mapFromGlobal(QCursor::pos())) &&
        (QGuiApplication::mouseButtons() & Qt::RightButton))
    {
        if (magnifyingGlass == nullptr)
        {
            magnifyingGlass = new QLabel(this);
            magnifyingGlass->setStyleSheet("background-color: rgba(0, 0, 0, 255);"
                                           "border-color: blue;"
                                           "border-style: solid;"
                                           "border-width: 3px;"
                                           "border-radius: 0px;");
        }

        const QSize magnifiedRegionSize = QSize(40, 30);
        const QSize glassSize = QSize(280, 210);
        const QPoint cursorPos = this->mapFromGlobal(QCursor::pos());
        QPoint regionTopLeft = QPoint((cursorPos.x() - (magnifiedRegionSize.width() / 2)),
                                      (cursorPos.y() - (magnifiedRegionSize.height() / 2)));

        // Don't let the magnification overflow the image buffer.
        if (regionTopLeft.x() < 0)
        {
            regionTopLeft.setX(0);
        }
        else if (regionTopLeft.x() > (frameImage.width() - magnifiedRegionSize.width()))
        {
            regionTopLeft.setX(frameImage.width() - magnifiedRegionSize.width());
        }
        if (regionTopLeft.y() < 0)
        {
            regionTopLeft.setY(0);
        }
        else if (regionTopLeft.y() > (frameImage.height() - magnifiedRegionSize.height()))
        {
            regionTopLeft.setY(frameImage.height() - magnifiedRegionSize.height());
        }

        // Grab the magnified region's pixel data.
        const u32 startIdx = (regionTopLeft.x() + regionTopLeft.y() * frameImage.width()) * (frameImage.depth() / 8);
        QImage magn(frameImage.bits() + startIdx, magnifiedRegionSize.width(), magnifiedRegionSize.height(),
                    frameImage.bytesPerLine(), frameImage.format());

        magnifyingGlass->resize(glassSize);
        magnifyingGlass->setPixmap(QPixmap::fromImage(magn.scaled(glassSize.width(), glassSize.height(),
                                                                  Qt::IgnoreAspectRatio, Qt::FastTransformation)));
        magnifyingGlass->move(std::min((this->width() - glassSize.width()), std::max(0, cursorPos.x() - (glassSize.width() / 2))),
                              std::max(0, std::min((this->height() - glassSize.height()), cursorPos.y() - (glassSize.height() / 2))));
        magnifyingGlass->show();
    }
    else
    {
        if (magnifyingGlass != nullptr &&
            magnifyingGlass->isVisible())
        {
            magnifyingGlass->setVisible(false);
        }
    }

    return;
}

void MainWindow::measure_framerate()
{
    static qint64 elapsed = 0, prevElapsed = 0;
    static u32 peakProcessTime = 0, avgProcessTime = 0;
    static u32 numFramesDrawn = 0;

    static QElapsedTimer fpsTimer;
    if (!fpsTimer.isValid())
    {
        fpsTimer.start();
    }

    elapsed = fpsTimer.elapsed();

    if ((elapsed - prevElapsed) > peakProcessTime)
    {
        peakProcessTime = (elapsed - prevElapsed);
    }

    avgProcessTime += (elapsed - prevElapsed);

    numFramesDrawn++;

    // Once per second or so update the GUI on capture output performance.
    if (elapsed >= 1000)
    {
        const int fps = round(1000 / (real(elapsed) / numFramesDrawn));

        UPDATE_LATENCY_AVG = (avgProcessTime / numFramesDrawn);
        UPDATE_LATENCY_PEAK = peakProcessTime;

        this->update_output_framerate(fps, kc_capture_api().get_missed_frames_count());
        kc_capture_api().reset_missed_frames_count();

        numFramesDrawn = 0;
        avgProcessTime = 0;
        peakProcessTime = 0;
        prevElapsed = 0;
        fpsTimer.restart();
    }
    else
    {
        prevElapsed = elapsed;
    }

    return;
}

void MainWindow::set_keyboard_shortcuts(void)
{
    // Creates a new QShortcut instance assigned to the given QKeySequence-
    // compatible sequence string (e.g. "F1" for the F1 key).
    auto keyboardShortcut = [this](const std::string keySequence)->QShortcut*
    {
        QShortcut *shortcut = new QShortcut(QKeySequence(keySequence.c_str()), this);
        shortcut->setContext(Qt::ApplicationShortcut);

        return shortcut;
    };

    // Assign ctrl + numeral key 1-9 to the input resolution force buttons.
    k_assert(this->inputResolutionDlg != nullptr, "");
    for (uint i = 1; i <= 9; i++)
    {
        connect(keyboardShortcut(QString("ctrl+%1").arg(QString::number(i)).toStdString()),
                &QShortcut::activated, [=]{this->inputResolutionDlg->activate_capture_res_button(i);});
    }

    // Assign alt + arrow keys to move the capture input alignment horizontally and vertically.
    connect(keyboardShortcut("alt+shift+left"), &QShortcut::activated,  []{kc_capture_api().adjust_horizontal_offset(1); });
    connect(keyboardShortcut("alt+shift+right"), &QShortcut::activated, []{kc_capture_api().adjust_horizontal_offset(-1);});
    connect(keyboardShortcut("alt+shift+up"), &QShortcut::activated,    []{kc_capture_api().adjust_vertical_offset(1);   });
    connect(keyboardShortcut("alt+shift+down"), &QShortcut::activated,  []{kc_capture_api().adjust_vertical_offset(-1);  });

    connect(keyboardShortcut("f5"), &QShortcut::activated, []{if (!kc_capture_api().has_no_signal()) ALIGN_CAPTURE = true;});

    // Make Ctrl + Shift + <x> toggle the various dialogs' functionality on/off.
    connect(keyboardShortcut("ctrl+shift+f"), &QShortcut::activated, [=]{this->filterGraphDlg->set_filter_graph_enabled(!this->filterGraphDlg->is_filter_graph_enabled());});
    connect(keyboardShortcut("ctrl+shift+l"), &QShortcut::activated, [=]{this->overlayDlg->set_overlay_enabled(!this->overlayDlg->is_overlay_enabled());});
    connect(keyboardShortcut("ctrl+shift+a"), &QShortcut::activated, [=]{this->antitearDlg->set_anti_tear_enabled(!this->antitearDlg->is_anti_tear_enabled());});
    connect(keyboardShortcut("ctrl+shift+r"), &QShortcut::activated, [=]{this->recordDlg->set_recording_enabled(!this->recordDlg->is_recording_enabled());});

    return;
}

void MainWindow::signal_new_known_alias(const mode_alias_s a)
{
    k_assert(this->aliasDlg != nullptr, "");
    aliasDlg->receive_new_alias(a);

    return;
}

void MainWindow::clear_filter_graph(void)
{
    k_assert(this->filterGraphDlg != nullptr, "");
    filterGraphDlg->clear_filter_graph();

    return;
}

void MainWindow::recalculate_filter_graph_chains(void)
{
    k_assert(this->filterGraphDlg != nullptr, "");
    filterGraphDlg->recalculate_filter_chains();
}

void MainWindow::disable_output_size_controls(const bool areDisabled)
{
    k_assert(this->outputResolutionDlg != nullptr, "");
    outputResolutionDlg->disable_output_size_controls(areDisabled);

    return;
}

FilterGraphNode* MainWindow::add_filter_graph_node(const filter_type_enum_e &filterType,
                                                   const u8 *const initialParameterValues)
{
    k_assert(this->filterGraphDlg != nullptr, "");
    return this->filterGraphDlg->add_filter_graph_node(filterType, initialParameterValues);
}

void MainWindow::signal_new_mode_settings_source_file(const std::string &filename)
{
    k_assert(this->signalDlg != nullptr, "");
    //this->signalDlg->receive_new_mode_settings_filename(QString::fromStdString(filename));

    return;
}

void MainWindow::set_recording_is_active(const bool isActive)
{
    k_assert(this->recordDlg != nullptr, "");
    this->recordDlg->set_recording_controls_enabled(isActive);

    return;
}

void MainWindow::update_window_title()
{
    QString title = PROGRAM_NAME;

    if (kc_capture_api().has_no_signal())
    {
        title = QString("%1 - No signal").arg(this->windowTitleOverride.isEmpty()? PROGRAM_NAME : this->windowTitleOverride);
    }
    else if (kc_capture_api().has_invalid_signal())
    {
        title = QString("%1 - Invalid signal").arg(this->windowTitleOverride.isEmpty()? PROGRAM_NAME : this->windowTitleOverride);
    }
    else
    {
        // A symbol shown in the title if VCS is currently dropping frames.
        const QString missedFramesMarker = "{!}";

        const resolution_s inRes = kc_capture_api().get_resolution();
        const resolution_s outRes = ks_output_resolution();
        const int relativeScale = round((outRes.h / (real)inRes.h) * 100); /// FIXME: Doesn't notice if only the width is modified.

        QStringList programStatus;
        if (krecord_is_recording()) programStatus << "R";
        if (kf_is_filtering_enabled()) programStatus << "F";
        if (overlayDlg->is_overlay_enabled()) programStatus << "O";
        if (kat_is_anti_tear_enabled()) programStatus << "A";

        if (!this->windowTitleOverride.isEmpty())
        {
            title = QString("%1%2")
                    .arg((kc_capture_api().get_missed_frames_count() > 0)? (missedFramesMarker + " ") : "")
                    .arg(this->windowTitleOverride);
        }
        else
        {
            title = QString("%1%2 - %3%4 x %5 scaled to %6 x %7 (~%8%)")
                    .arg((kc_capture_api().get_missed_frames_count() > 0)? (missedFramesMarker + " ") : "")
                    .arg(PROGRAM_NAME)
                    .arg(programStatus.count()? QString("%1 - ").arg(programStatus.join("")) : "")
                    .arg(inRes.w)
                    .arg(inRes.h)
                    .arg(outRes.w)
                    .arg(outRes.h)
                    .arg(relativeScale);
        }
    }

    this->setWindowTitle(title);

    return;
}

void MainWindow::set_capture_info_as_no_signal()
{
    update_window_title();

    k_assert(this->signalDlg != nullptr, "");
    this->signalDlg->set_controls_enabled(false);

    return;
}

void MainWindow::set_capture_info_as_receiving_signal()
{
    k_assert(this->signalDlg != nullptr, "");
    this->signalDlg->set_controls_enabled(true);

    return;
}

uint MainWindow::output_framerate(void)
{
    return CURRENT_OUTPUT_FRAMERATE;
}

void MainWindow::update_output_framerate(const u32 fps,
                                         const bool missedFrames)
{
    CURRENT_OUTPUT_FRAMERATE = fps;

    // We assume that the window title contains information about
    // the current frame rate, so we'll want to keep it up-to-date.
    this->update_window_title();

    (void)missedFrames;

    return;
}

void MainWindow::update_video_mode_params(void)
{
    k_assert(this->signalDlg != nullptr, "");
    //this->signalDlg->update_controls();

    return;
}

void MainWindow::update_capture_signal_info(void)
{
    if (kc_capture_api().has_no_signal())
    {
        DEBUG(("Was asked to update GUI input info while there was no signal."));
    }
    else
    {
        k_assert(this->signalDlg != nullptr, "");
        this->signalDlg->notify_of_new_capture_signal();

        k_assert(this->outputResolutionDlg != nullptr, "");
        this->outputResolutionDlg->notify_of_new_capture_signal();

        this->update_window_title();
    }

    return;
}

void MainWindow::set_filter_graph_options(const std::vector<filter_graph_option_s> &graphOptions)
{
    k_assert(this->filterGraphDlg != nullptr, "");
    this->filterGraphDlg->set_filter_graph_options(graphOptions);

    return;
}

void MainWindow::set_filter_graph_source_filename(const std::string &sourceFilename)
{
    k_assert(this->filterGraphDlg != nullptr, "");
    this->filterGraphDlg->set_filter_graph_source_filename(sourceFilename);

    return;
}

void MainWindow::update_recording_metainfo(void)
{
    k_assert(this->recordDlg != nullptr, "");
    this->recordDlg->update_recording_metainfo();

    return;
}

void MainWindow::update_gui_state()
{
    // Manually spin the event loop.
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents();

    return;
}

void MainWindow::clear_known_aliases()
{
    k_assert(this->aliasDlg != nullptr, "");
    this->aliasDlg->clear_known_aliases();

    return;
}

void MainWindow::update_window_size()
{
    const resolution_s r = ks_output_resolution();

    this->setFixedSize(r.w, r.h);
    overlayDlg->set_overlay_max_width(r.w);

    update_window_title();

    return;
}

bool MainWindow::window_has_border()
{
    return !bool(windowFlags() & Qt::FramelessWindowHint);
}

void MainWindow::toggle_window_border()
{
    if (this->isFullScreen())
    {
        return;
    }

    const Qt::WindowFlags borderlessFlags = (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    // Show the border.
    if (!window_has_border())
    {
        this->setWindowFlags(this->defaultWindowFlags & ~borderlessFlags);
        this->show();

        update_window_size();

        emit this->border_revealed();
    }
    // Hide the border.
    else
    {
        this->setWindowFlags(Qt::FramelessWindowHint | borderlessFlags);
        this->show();

        update_window_size();

        emit this->border_hidden();
    }
}

void MainWindow::add_gui_log_entry(const log_entry_s e)
{
    /// GUI logging is currently not implemented.

    (void)e;

    return;
}
