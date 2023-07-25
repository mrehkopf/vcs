/*
 * 2018-2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 *
 * VCS's main window. Displays captured frames and provides the user access to child dialogs
 * for adjusting various program settings.
 *
 */

#include <QElapsedTimer>
#include <QTextDocument>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QDateTime>
#include <QShortcut>
#include <QPainter>
#include <QRegExp>
#include <QScreen>
#include <QImage>
#include <QMenu>
#include <QLabel>
#include <QDir>
#include <cmath>
#include "display/qt/subclasses/QLabel_magnifying_glass.h"
#include "display/qt/subclasses/QOpenGLWidget_opengl_renderer.h"
#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"
#include "display/qt/dialogs/components/window_options_dialog/window_size.h"
#include "display/qt/dialogs/components/capture_dialog/input_resolution.h"
#include "display/qt/windows/control_panel_window.h"
#include "display/qt/dialogs/window_options_dialog.h"
#include "display/qt/dialogs/capture_dialog.h"
#include "display/qt/dialogs/filter_graph_dialog.h"
#include "display/qt/dialogs/overlay_dialog.h"
#include "display/qt/windows/output_window.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/keyboard_shortcuts.h"
#include "anti_tear/anti_tear.h"
#include "common/vcs_event/vcs_event.h"
#include "capture/video_presets.h"
#include "capture/capture.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "main.h"
#include "ui_output_window.h"

// For an optional OpenGL render surface.
static OGLWidget *OGL_SURFACE = nullptr;

OutputWindow::OutputWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::OutputWindow)
{
    ui->setupUi(this);

    this->setWindowFlags(this->defaultWindowFlags);

    ui->centralwidget->setMouseTracking(true);
    ui->centralwidget->setFocusPolicy(Qt::StrongFocus);

    // Set up a layout for the central widget, so we can add the OpenGL
    // render surface to it when/if OpenGL is enabled.
    QVBoxLayout *const mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);

    this->magnifyingGlass = new MagnifyingGlass(this);
    this->controlPanelWindow = new ControlPanelWindow(this);

    // Restore persistent settings.
    {
        k_set_eco_mode_enabled(kpers_value_of(INI_GROUP_APP, "EcoMode", k_is_eco_mode_enabled()).toBool());
    }

    // Create the window's context menu.
    {
        this->contextMenu = new QMenu(this);

        QAction *controlPanel = new QAction("Control panel...", this);
        {
            controlPanel->setShortcut(kd_get_key_sequence("output-window: open-control-panel-dialog"));
            connect(controlPanel, &QAction::triggered, this, [this]{this->controlPanelWindow->show();});
            this->addAction(controlPanel);
        }

        QAction *showBorder = new QAction("Border", this);
        {

            showBorder->setCheckable(true);
            showBorder->setChecked(this->window_has_border());
            showBorder->setShortcut(kd_get_key_sequence("output-window: toggle-window-border"));

            connect(this, &OutputWindow::border_hidden, this, [showBorder]
            {
                showBorder->setChecked(false);
            });

            connect(this, &OutputWindow::border_shown, this, [showBorder]
            {
                showBorder->setChecked(true);
            });

            connect(this, &OutputWindow::fullscreen_mode_enabled, this, [showBorder]
            {
                showBorder->setEnabled(false);
            });

            connect(this, &OutputWindow::fullscreen_mode_disabled, this, [showBorder]
            {
                showBorder->setEnabled(true);
            });

            connect(showBorder, &QAction::triggered, this, [this]
            {
                this->toggle_window_border();
            });
        }

        QAction *fullscreen = new QAction("Fullscreen", this);
        {

            fullscreen->setCheckable(true);
            fullscreen->setChecked(this->isFullScreen());
            fullscreen->setShortcut(kd_get_key_sequence("output-window: toggle-fullscreen-mode"));

            connect(this, &OutputWindow::fullscreen_mode_enabled, this, [=]
            {
                fullscreen->setChecked(true);
            });

            connect(this, &OutputWindow::fullscreen_mode_disabled, this, [=]
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
        }

        QAction *ecoMode = new QAction("Eco mode", this);
        {
            ecoMode->setCheckable(true);
            ecoMode->setChecked(k_is_eco_mode_enabled());

            connect(ecoMode, &QAction::toggled, this, [](bool checked)
            {
                k_set_eco_mode_enabled(checked);
                kpers_set_value(INI_GROUP_APP, "EcoMode", checked);
            });
        }

        connect(this->contextMenu->addAction("Screenshot"), &QAction::triggered, this, [this]{this->save_screenshot();});
        this->contextMenu->addSeparator();
        this->contextMenu->addAction(controlPanel);
        this->contextMenu->addSeparator();
        this->contextMenu->addAction(showBorder);
        this->contextMenu->addAction(fullscreen);
        this->contextMenu->addSeparator();
        this->contextMenu->addAction(ecoMode);
    }

    // We intend to repaint the entire window every time we update it, so ask for no automatic fill.
    this->setAttribute(Qt::WA_OpaquePaintEvent, true);

    this->update_window_size();
    this->update_window_title();

    // Set keyboard shortcuts.
    {
        auto makeAppwideShortcut = [this](const std::string label)->QShortcut*
        {
            QShortcut *shortcut = kd_make_key_shortcut(label, this);

            shortcut->setContext(Qt::ApplicationShortcut);
            shortcut->setAutoRepeat(false);

            return shortcut;
        };

        // Numkeys 1 through 9.
        for (uint i = 1; i <= 9; i++)
        {
            const std::string shortcutString = ("input-resolution-dialog: resolution-activator-" + std::to_string(i));

            connect(makeAppwideShortcut(shortcutString), &QShortcut::activated, [i, this]
            {
                this->control_panel()->capture()->input_resolution()->activate_resolution_button(i);
            });
        }

        connect(makeAppwideShortcut("filter-graph-dialog: toggle-enabled"), &QShortcut::activated, [this]
        {
            this->control_panel()->filter_graph()->set_enabled(!this->control_panel()->filter_graph()->is_enabled());
        });

        connect(makeAppwideShortcut("overlay-dialog: toggle-enabled"), &QShortcut::activated, [this]
        {
            this->control_panel()->overlay()->set_enabled(!this->control_panel()->overlay()->is_enabled());
        });

        // F1 through F12.
        for (uint i = 1; i <= 12; i++)
        {
            const std::string shortcutString = ("video-preset-dialog: preset-activator-" + std::to_string(i));

            connect(makeAppwideShortcut(shortcutString), &QShortcut::activated, this, [=]
            {
                kvideopreset_activate_keyboard_shortcut(shortcutString);
            });
        }

        connect(makeAppwideShortcut("output-window: set-input-channel-1"), &QShortcut::activated, []{
            kc_set_device_property("input channel index", 0);
        });

        connect(makeAppwideShortcut("output-window: set-input-channel-2"), &QShortcut::activated, []{
            kc_set_device_property("input channel index", 1);
        });

        connect(kd_make_key_shortcut("output-window: exit-fullscreen-mode", this), &QShortcut::activated, this, [this]
        {
            if (this->isFullScreen())
            {
                this->showNormal();
            }
        });
    }

    this->activateWindow();
    this->raise();

    // Apply any custom app-wide styling styling.
    {
        qApp->setWindowIcon(QIcon(":/res/images/icons/appicon.ico"));
        this->apply_global_stylesheet(":/res/stylesheets/appstyle-newie.qss");
        this->set_global_font_size(this->appwideFontSize);
    }

    // Listen for app events.
    {
        ev_dirty_output_window.listen([this]
        {
            this->redraw();
        });

        ev_new_output_image.listen([this]
        {
            this->redraw();
        });

        ev_eco_mode_enabled.listen([this]
        {
            this->update_window_title();
        });

        ev_eco_mode_disabled.listen([this]
        {
            this->update_window_title();
        });

        ev_new_output_resolution.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
        });

        ev_capture_signal_lost.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
            this->redraw();
        });

        ev_invalid_capture_device.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
            this->redraw();
        });

        ev_invalid_capture_signal.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
            this->redraw();
        });

        ev_new_video_mode.listen([this](video_mode_s)
        {
            this->update_window_title();
        });
    }

    return;
}

bool OutputWindow::apply_global_stylesheet(const QString &qssFilename)
{
    QFile styleFile(qssFilename);

    if (styleFile.open(QIODevice::ReadOnly))
    {
        const QString styleSheet = styleFile.readAll();
        this->appwideStyleSheet = styleSheet;
        qApp->setStyleSheet(styleSheet);
        return true;
    }

    return false;
}

bool OutputWindow::set_global_font_size(const unsigned fontSize)
{
    #if __linux__
        QFile fontStyleFile(":/res/stylesheets/font-linux.qss");
    #else
        #error "Unknown platform."
    #endif

    if (fontStyleFile.open(QIODevice::ReadOnly))
    {
        QString fontStyleSheet = fontStyleFile.readAll();
        fontStyleSheet.replace("%FONT_SIZE%", QString("%1px").arg(fontSize));

        qApp->setStyleSheet(this->appwideStyleSheet + "\n" + fontStyleSheet);
        this->appwideFontSize = fontSize;

        return true;
    }

    return false;
}

void OutputWindow::override_window_title(const std::string &newTitle)
{
    this->windowTitleOverride = QString::fromStdString(newTitle);
    this->update_window_title();

    return;
}

OutputWindow::~OutputWindow()
{
    delete ui;
    ui = nullptr;

    return;
}

bool OutputWindow::load_font(const QString &filename)
{
    if (QFontDatabase::addApplicationFont(filename) == -1)
    {
        NBENE(("Failed to load the font '%s'.", filename.toStdString().c_str()));
        return false;
    }

    return true;
}

void OutputWindow::redraw(void)
{
    if (OGL_SURFACE != nullptr)
    {
        OGL_SURFACE->repaint();
    }
    else this->repaint();

    return;
}

void OutputWindow::set_opengl_enabled(const bool enabled)
{
    if (enabled)
    {
        k_assert((OGL_SURFACE == nullptr), "Can't doubly enable OpenGL.");

        OGL_SURFACE = new OGLWidget(std::bind(&OutputWindow::overlay_image, this), this);
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
        ui->centralwidget->layout()->removeWidget(OGL_SURFACE);
        delete OGL_SURFACE;
        OGL_SURFACE = nullptr;
    }

    return;
}

void OutputWindow::closeEvent(QCloseEvent *event)
{
    // The main VCS loop will close the window if it finds PROGRAM_EXIT_REQUESTED true.
    event->ignore();

    /// TODO: Since previously standalone dialogs were infused into a single control
    /// panel, we now need to reorganize this unsaved changes detector as well.
#if 0
    // If there are unsaved changes, ask the user to confirm to exit.
    {
        QStringList dialogsWithUnsavedChanges;

        for (const VCSBaseDialog *dialog: this->dialogs)
        {
            if (dialog->has_unsaved_changes())
            {
                dialogsWithUnsavedChanges << QString("* %1").arg(dialog->name());
            }
        }

        if (dialogsWithUnsavedChanges.count() &&
            (QMessageBox::question(this, "Confirm exit",
                                   "The following dialogs have unsaved changes:"
                                   "<b><br><br>" + dialogsWithUnsavedChanges.join("<br>") +
                                   "</b><br><br>Exit and lose unsaved changes?",
                                   (QMessageBox::No | QMessageBox::Yes)) == QMessageBox::No))
        {
            return;
        }
    }
#else
    DEBUG(("Unimplemented OutputWindow::closeEvent() functionality."));
#endif

    PROGRAM_EXIT_REQUESTED = 1;

    return;
}

void OutputWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        toggle_window_border();
    }

    return;
}

void OutputWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        // Hide the window's cursor when in full-screen mode, and restore it when not.
        /// TODO: Ideally, this would only get set when we enter full-screen
        /// mode and back, not every time the window's state changes.
        if (this->windowState() & Qt::WindowFullScreen)
        {
            emit this->fullscreen_mode_enabled();
            this->setCursor(QCursor(Qt::BlankCursor));
        }
        else
        {
            emit this->fullscreen_mode_disabled();
            this->unsetCursor();
        }
    }

    QWidget::changeEvent(event);
}

/// Temp. Used to track mouse movement delta across frames.
static bool LEFT_MOUSE_BUTTON_DOWN = false;
static QPoint PREV_MOUSE_POS;

void OutputWindow::mouseMoveEvent(QMouseEvent *event)
{
    // If the cursor is over the capture window and the left mouse button is being
    // held down, drag the window. Note: We disable dragging in fullscreen mode,
    // since really you shouldn't be able to drag a fullscreen window.
    if (LEFT_MOUSE_BUTTON_DOWN &&
        !this->isFullScreen())
    {
        QPoint delta = (event->globalPos() - PREV_MOUSE_POS);
        this->move(pos() + delta);
    }

    PREV_MOUSE_POS = event->globalPos();

    return;
}

void OutputWindow::mousePressEvent(QMouseEvent *event)
{
    ui->centralwidget->setFocus();

    if (event->button() == Qt::LeftButton)
    {
        LEFT_MOUSE_BUTTON_DOWN = true;
        PREV_MOUSE_POS = event->globalPos();
    }

    return;
}

void OutputWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        LEFT_MOUSE_BUTTON_DOWN = false;
    }

    return;
}


void OutputWindow::wheelEvent(QWheelEvent *event)
{
    // Adjust the size of the capture window with the mouse scroll wheel.
    const int dir = (event->angleDelta().y() < 0)? 1 : -1;
    this->control_panel()->window_options()->window_size()->adjust_output_scaling(dir);

    return;
}

QImage OutputWindow::overlay_image(void)
{
    if (kc_has_signal() && this->control_panel()->overlay()->is_enabled())
    {
        return this->control_panel()->overlay()->rendered();
    }
    else
    {
        return QImage();
    }
}

void OutputWindow::paintEvent(QPaintEvent *)
{
    // If OpenGL is enabled, its own paintGL() should be getting called instead of paintEvent().
    if (OGL_SURFACE != nullptr) return;

    // Convert the output buffer into a QImage frame.
    const QImage frameImage = ([]
    {
       const image_s image = ks_scaler_frame_buffer();
       k_assert((image.bitsPerPixel == 32), "Expected output image data to be 32-bit.");

       if (!image.pixels)
       {
           DEBUG(("Requested the scaler output as a QImage while the scaler's output buffer was uninitialized."));
           return QImage();
       }
       else
       {
           return QImage(image.pixels, image.resolution.w, image.resolution.h, QImage::Format_RGB32);
       }
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
    if (kc_has_signal() &&
        this->isActiveWindow() &&
        this->rect().contains(this->mapFromGlobal(QCursor::pos())) &&
        (QGuiApplication::mouseButtons() & Qt::MidButton))
    {
        this->magnifyingGlass->magnify(frameImage, this->mapFromGlobal(QCursor::pos()));
    }
    else
    {
        this->magnifyingGlass->hide();
    }

    return;
}

void OutputWindow::update_window_title(void)
{
    QString programName = QString("%1%2").arg(PROGRAM_NAME).arg(k_is_eco_mode_enabled()? " Eco" : "");
    QString title = programName;

    if (PROGRAM_EXIT_REQUESTED)
    {
        title = QString("%1 - Closing...").arg(programName);
    }
    else if (!kc_has_signal())
    {
        title = QString("%1 - No signal").arg(this->windowTitleOverride.isEmpty()? programName : this->windowTitleOverride);
    }
    else
    {
        const auto inRes = resolution_s::from_capture_device();
        const resolution_s outRes = ks_output_resolution();
        const auto refreshRate = refresh_rate_s::from_capture_device();

        if (!this->windowTitleOverride.isEmpty())
        {
            title = this->windowTitleOverride;
        }
        else
        {
            title = QString("%1 - %2 \u00d7 %3 (%4 Hz) shown in %5 \u00d7 %6")
                .arg(programName)
                .arg(inRes.w)
                .arg(inRes.h)
                .arg(QString::number(refreshRate.value<double>(), 'f', 3))
                .arg(outRes.w)
                .arg(outRes.h);
        }
    }

    this->setWindowTitle(title);

    return;
}

void OutputWindow::update_gui_state(void)
{
    // Manually spin the event loop.
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents();

    return;
}

void OutputWindow::update_window_size(void)
{
    const resolution_s r = ks_output_resolution();

    this->setFixedSize(r.w, r.h);
    this->control_panel()->overlay()->set_overlay_max_width(r.w);

    update_window_title();

    return;
}

bool OutputWindow::window_has_border(void)
{
    return !bool(windowFlags() & Qt::FramelessWindowHint);
}

void OutputWindow::toggle_window_border(void)
{
    if (this->isFullScreen())
    {
        return;
    }

    const Qt::WindowFlags borderlessFlags = (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    if (!window_has_border())
    {
        this->setWindowFlags(this->defaultWindowFlags & ~borderlessFlags);
        emit this->border_shown();
    }
    else
    {
        this->setWindowFlags(Qt::FramelessWindowHint | borderlessFlags);
        emit this->border_hidden();
    }

    this->show();
    update_window_size();
}

void OutputWindow::save_screenshot(void)
{
    const QString datestampedFilename = (
        QDir::currentPath() +
        "/" +
        ([]()->QString
        {
            QString filename = QString("vcs %1.png").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd 'at' hh.mm.ss"));

            if (QFile::exists(filename))
            {
                // Append the current time's milliseconds, for some protection against filename collisions.
                filename.replace(QRegExp(".png$"), QString(".%1.png").arg(QTime::currentTime().toString("z")));
            }

            return filename;
        })()
    );

    const QImage frameImage = ([]()->QImage
    {
        const image_s curOutputImage = ks_scaler_frame_buffer();

        if (!curOutputImage.pixels)
        {
            DEBUG(("Requested the scaler output as a QImage while the scaler's output buffer was uninitialized."));
            return QImage();
        }
        else
        {
            return QImage(curOutputImage.pixels, curOutputImage.resolution.w, curOutputImage.resolution.h, QImage::Format_RGB32);
        }
    })();

    if (frameImage.save(datestampedFilename))
    {
        INFO(("Output saved to \"%s\".", QDir::toNativeSeparators(datestampedFilename).toStdString().c_str()));
    }
    else
    {
        NBENE(("Failed to save the output image to \"%s\".", QDir::toNativeSeparators(datestampedFilename).toStdString().c_str()));

        QMessageBox::critical(
            this,
            "Error saving image",
            QString(
                "The following location could not be written to:<br><br><b>%1</b>"
            ).arg(QDir::toNativeSeparators(QFileInfo(datestampedFilename).absolutePath()))
        );
    }

    return;
}

void OutputWindow::contextMenuEvent(QContextMenuEvent *event)
{
    this->contextMenu->popup(event->globalPos());

    return;
}

void OutputWindow::add_gui_log_entry(const log_entry_s e)
{
    /// GUI logging is currently not implemented.

    (void)e;

    return;
}
