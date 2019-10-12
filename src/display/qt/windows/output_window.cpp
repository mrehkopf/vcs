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
#include "display/qt/dialogs/input_resolution_dialog.h"
#include "display/qt/dialogs/video_and_color_dialog.h"
#include "display/qt/dialogs/filter_graph_dialog.h"
#include "display/qt/dialogs/resolution_dialog.h"
#include "display/qt/dialogs/anti_tear_dialog.h"
#include "display/qt/dialogs/overlay_dialog.h"
#include "display/qt/dialogs/record_dialog.h"
#include "display/qt/windows/output_window.h"
#include "display/qt/dialogs/alias_dialog.h"
#include "display/qt/dialogs/about_dialog.h"
#include "display/qt/persistent_settings.h"
#include "common/propagate.h"
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

    // Set up a layout for the central widget, so we can add the OpenGL
    // render surface to it when OpenGL is enabled.
    QVBoxLayout *const mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);

    ui->menuBar->setVisible(false);

    // Set up dialogs.
    {
        outputResolutionDlg = new OutputResolutionDialog;
        inputResolutionDlg = new InputResolutionDialog;
        filterGraphDlg = new FilterGraphDialog;
        antitearDlg = new AntiTearDialog;
        overlayDlg = new OverlayDialog;
        videoDlg = new VideoAndColorDialog;
        aliasDlg = new AliasDialog;
        aboutDlg = new AboutDialog;
        recordDlg = new RecordDialog;
    }

    // Create the window's menu bar.
    {
        // File...
        {
            QMenu *menu = new QMenu("File", this);

            connect(menu->addAction("Exit"), &QAction::triggered, this, [=]{this->close();});

            ui->menuBar->addMenu(menu);
            this->addActions(menu->actions());
        }

        // Input...
        {
            QMenu *menu = new QMenu("Input", this);

            QMenu *channel = new QMenu("Channel", this);
            {
                QActionGroup *group = new QActionGroup(this);

                for (int i = 0; i < kc_hardware().meta.num_capture_inputs(); i++)
                {
                    QAction *inputChannel = new QAction(QString::number(i+1), this);
                    inputChannel->setActionGroup(group);
                    inputChannel->setCheckable(true);
                    channel->addAction(inputChannel);

                    if (i == int(INPUT_CHANNEL_IDX))
                    {
                        inputChannel->setChecked(true);
                    }

                    connect(inputChannel, &QAction::triggered, this, [=]{kc_set_input_channel(i);});
                }
            }

            QMenu *colorDepth = new QMenu("Color depth", this);
            {
                QActionGroup *group = new QActionGroup(this);

                QAction *c24 = new QAction("24-bit (888)", this);
                c24->setActionGroup(group);
                c24->setCheckable(true);
                c24->setChecked(true);
                colorDepth->addAction(c24);

                QAction *c16 = new QAction("16-bit (565)", this);
                c16->setActionGroup(group);
                c16->setCheckable(true);
                colorDepth->addAction(c16);

                QAction *c15 = new QAction("15-bit (555)", this);
                c15->setActionGroup(group);
                c15->setCheckable(true);
                colorDepth->addAction(c15);

                connect(c24, &QAction::triggered, this, [=]{kc_set_input_color_depth(24);});
                connect(c16, &QAction::triggered, this, [=]{kc_set_input_color_depth(16);});
                connect(c15, &QAction::triggered, this, [=]{kc_set_input_color_depth(15);});
            }

            menu->addMenu(channel);
            menu->addSeparator();
            menu->addMenu(colorDepth);
            menu->addSeparator();

            QAction *video = new QAction("Video...", this);
            video->setShortcut(QKeySequence("ctrl+v"));
            menu->addAction(video);

            connect(menu->addAction("Aliases..."), &QAction::triggered, this, [=]{this->open_alias_dialog();});

            QAction *resolution = new QAction("Resolution...", this);
            resolution->setShortcut(QKeySequence("ctrl+i"));
            menu->addAction(resolution);

            connect(video, &QAction::triggered, this, [=]{this->open_video_dialog();});
            connect(resolution, &QAction::triggered, this, [=]{this->open_input_resolution_dialog();});

            ui->menuBar->addMenu(menu);
            this->addActions(menu->actions());
        }

        // Output...
        {
            QMenu *menu = new QMenu("Output", this);

            const std::vector<std::string> scalerNames = ks_list_of_scaling_filter_names();
            k_assert(!scalerNames.empty(), "Expected to receive a list of scalers, but got an empty list.");

            QMenu *renderer = new QMenu("Renderer", this);
            {
                QActionGroup *group = new QActionGroup(this);

                QAction *opengl = new QAction("OpenGL", this);
                opengl->setActionGroup(group);
                opengl->setCheckable(true);
                renderer->addAction(opengl);

                QAction *software = new QAction("Software", this);
                software->setActionGroup(group);
                software->setCheckable(true);
                renderer->addAction(software);

                connect(opengl, &QAction::toggled, this, [=](const bool checked){if (checked) this->set_opengl_enabled(true);});
                connect(software, &QAction::toggled, this, [=](const bool checked){if (checked) this->set_opengl_enabled(false);});

                if (kpers_value_of(INI_GROUP_OUTPUT, "renderer", "Software").toString() == "Software")
                {
                    software->setChecked(true);
                }
                else
                {
                    opengl->setChecked(true);
                }
            }

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

            menu->addMenu(renderer);
            menu->addSeparator();
            menu->addMenu(aspectRatio);
            menu->addSeparator();
            menu->addMenu(upscaler);
            menu->addMenu(downscaler);
            menu->addSeparator();

            connect(menu->addAction("Record..."), &QAction::triggered, this, [=]{this->open_record_dialog();});
            connect(menu->addAction("Overlay..."), &QAction::triggered, this, [=]{this->open_overlay_dialog();});

            QAction *resolution = new QAction("Resolution...", this);
            resolution->setShortcut(QKeySequence("ctrl+o"));
            menu->addAction(resolution);
            connect(resolution, &QAction::triggered, this, [=]{this->open_output_resolution_dialog();});

            QAction *filter = new QAction("Filter graph...", this);
            filter->setShortcut(QKeySequence("ctrl+f"));
            menu->addAction(filter);
            connect(filter, &QAction::triggered, this, [=]{this->open_filter_graph_dialog();});

            connect(menu->addAction("Anti-tearing..."), &QAction::triggered, this, [=]{this->open_antitear_dialog();});

            ui->menuBar->addMenu(menu);
            this->addActions(menu->actions());
        }

        // Help...
        {
            QMenu *menu = new QMenu("Help", this);

            connect(menu->addAction("About..."), &QAction::triggered, this, [=]{this->open_about_dialog();});

            ui->menuBar->addMenu(menu);
            this->addActions(menu->actions());
        }
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

    delete overlayDlg;
    overlayDlg = nullptr;

    delete filterGraphDlg;
    filterGraphDlg = nullptr;

    delete antitearDlg;
    antitearDlg = nullptr;

    delete videoDlg;
    videoDlg = nullptr;

    delete aliasDlg;
    aliasDlg = nullptr;

    delete aboutDlg;
    aboutDlg = nullptr;

    delete recordDlg;
    recordDlg = nullptr;

    delete outputResolutionDlg;
    outputResolutionDlg = nullptr;

    delete inputResolutionDlg;
    inputResolutionDlg = nullptr;

    return;
}

// Load the font pointed to by the given filename, and make it available to the program.
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

void MainWindow::open_video_dialog(void)
{
    k_assert(this->videoDlg != nullptr, "");
    this->videoDlg->show();
    this->videoDlg->activateWindow();
    this->videoDlg->raise();

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

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Toggle the window border on/off.
    if (event->button() == Qt::LeftButton)
    {
        toggle_window_border();
    }

    return;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Tab) && !event->isAutoRepeat())
    {
        ui->menuBar->setVisible(!ui->menuBar->isVisible());

        return;
    }

    QWidget::keyPressEvent(event);

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
            this->setCursor(QCursor(Qt::BlankCursor));
        }
        else this->unsetCursor();
    }

    QWidget::changeEvent(event);
}

static QPoint PREV_MOUSE_POS; /// Temp. Used to track mouse movement delta across frames.
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // If the cursor is over the capture window and the left mouse button is being
    // held down, drag the window.
    if (QApplication::mouseButtons() & Qt::LeftButton)
    {
        QPoint delta = (event->globalPos() - PREV_MOUSE_POS);
        this->move(pos() + delta);
    }

    PREV_MOUSE_POS = event->globalPos();

    return;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        PREV_MOUSE_POS = event->globalPos();
    }

    ui->menuBar->setVisible(false);

    return;
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
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

// Returns true if the user is allowed to scale the output resolution by using the
// mouse wheel over the capture window.
//
bool MainWindow::is_mouse_wheel_scaling_allowed(void)
{
    return (!kd_is_fullscreen() && // On my virtual machine, at least, wheel scaling while in full-screen messes up the full-screen mode.
            !krecord_is_recording());
}

// Returns the current overlay as a QImage, or a null QImage if the overlay
// should not be shown at this time.
//
QImage MainWindow::overlay_image(void)
{
    if (!kc_no_signal() &&
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
    if (!kc_no_signal() &&
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

// Call this once per frame and it'll derive the current frame rate.
//
void MainWindow::measure_framerate()
{
    static qint64 elapsed = 0, prevElapsed = 0;
    static u32 peakProcessTime = 0, avgProcessTime;
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

        UPDATE_LATENCY_AVG = avgProcessTime / numFramesDrawn;
        UPDATE_LATENCY_PEAK = peakProcessTime;

        this->update_output_framerate(fps, kc_are_frames_being_missed());
        kc_reset_missed_frames_count();

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

void MainWindow::set_keyboard_shortcuts()
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
    connect(keyboardShortcut("alt+shift+left"), &QShortcut::activated, []{kc_adjust_video_horizontal_offset(1);});
    connect(keyboardShortcut("alt+shift+right"), &QShortcut::activated, []{kc_adjust_video_horizontal_offset(-1);});
    connect(keyboardShortcut("alt+shift+up"), &QShortcut::activated, []{kc_adjust_video_vertical_offset(1);});
    connect(keyboardShortcut("alt+shift+down"), &QShortcut::activated, []{kc_adjust_video_vertical_offset(-1);});

    /// NOTE: Qt's full-screen mode might not work correctly under Linux, depending
    /// on the distro etc.
    connect(keyboardShortcut("f11"), &QShortcut::activated, [this]
    {
        if (this->isFullScreen())
        {
            this->showNormal();
        }
        else this->showFullScreen();
    });

    connect(keyboardShortcut("f5"), &QShortcut::activated, []{if (!kc_no_signal()) ALIGN_CAPTURE = true;});

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

FilterGraphNode* MainWindow::add_filter_graph_node(const filter_type_enum_e &filterType,
                                                   const u8 *const initialParameterValues)
{
    k_assert(this->filterGraphDlg != nullptr, "");
    return this->filterGraphDlg->add_filter_graph_node(filterType, initialParameterValues);
}

void MainWindow::signal_new_mode_settings_source_file(const std::string &filename)
{
    k_assert(this->videoDlg != nullptr, "");
    this->videoDlg->receive_new_mode_settings_filename(QString::fromStdString(filename));

    return;
}

// Updates the window title with the capture's current status.
//
void MainWindow::update_window_title()
{
    QString scaleString, latencyString, inResString, recordingString;

    if (kc_no_signal())
    {
        latencyString = " - No signal";
    }
    else if (kc_is_invalid_signal())
    {
        latencyString = " - Invalid signal";
    }
    else
    {
        const resolution_s inRes = kc_hardware().status.capture_resolution();
        const resolution_s outRes = ks_output_resolution();
        const int relativeScale = round((outRes.h / (real)inRes.h) * 100);

        scaleString = QString(" scaled to %1 x %2 (~%3%)").arg(outRes.w).arg(outRes.h).arg(relativeScale);

        if (kc_are_frames_being_missed())
        {
            latencyString = " (dropping frames)";
        }

        if (krecord_is_recording())
        {
            recordingString = "[rec] ";
        }

        inResString = " - " + QString("%1 x %2").arg(inRes.w).arg(inRes.h);
    }

    this->setWindowTitle(recordingString + QString(PROGRAM_NAME) + inResString + scaleString + latencyString);

    return;
}

void MainWindow::set_capture_info_as_no_signal()
{
    update_window_title();

    k_assert(this->videoDlg != nullptr, "");
    this->videoDlg->set_controls_enabled(false);

    return;
}

void MainWindow::set_capture_info_as_receiving_signal()
{
    k_assert(this->videoDlg != nullptr, "");
    this->videoDlg->set_controls_enabled(true);

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

    update_window_title();

    (void)missedFrames;

    return;
}

void MainWindow::update_video_mode_params(void)
{
    k_assert(this->videoDlg != nullptr, "");
    this->videoDlg->update_controls();

    return;
}

void MainWindow::update_capture_signal_info(void)
{
    if (kc_no_signal())
    {
        DEBUG(("Was asked to update GUI input info while there was no signal."));
    }
    else
    {
        k_assert(this->videoDlg != nullptr, "");
        this->videoDlg->notify_of_new_capture_signal();
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
    if (!window_has_border()) // Show the border.
    {
        this->setWindowFlags(windowFlags() & ~(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint));
        this->show();

        update_window_size();
    }
    else // Hide the border.
    {
        this->move(0, 0);

        this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        this->show();

        update_window_size();
    }
}

// Displays to the user a message box that asks them to either Ok or Cancel the
// given proposal. The return value will give the Qt code for the button button
// the user pressed.
//
u32 MainWindow::ShowMessageBox_Query(const QString title, const QString msg)
{
    QMessageBox mb(this);
    mb.setWindowTitle(title);
    mb.setText(msg);
    mb.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    mb.setIcon(QMessageBox::Information);
    mb.setDefaultButton(QMessageBox::Ok);

    u32 ret = mb.exec();

    return ret;
}

// Displays to the user a message box that relays to them the given informational
// message. For errors, use the error version of this function.
//
void MainWindow::ShowMessageBox_Information(const QString msg)
{
    QMessageBox mb(this);
    mb.setWindowTitle("VCS had this to say");
    mb.setText(msg);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.setIcon(QMessageBox::Information);
    mb.setDefaultButton(QMessageBox::Ok);

    mb.exec();

    return;
}

// Displays to the user a message box that relays to them the given error
// message.
//
void MainWindow::ShowMessageBox_Error(const QString msg)
{
    QMessageBox mb(this);
    mb.setWindowTitle("VCS says nope");
    mb.setText(msg);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.setIcon(QMessageBox::Critical);
    mb.setDefaultButton(QMessageBox::Ok);

    mb.exec();

    return;
}

void MainWindow::add_gui_log_entry(const log_entry_s e)
{
    /// GUI logging is currently not implemented.

    (void)e;

    return;
}
