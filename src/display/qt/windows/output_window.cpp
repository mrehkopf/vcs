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
#include "../../../display/qt/subclasses/QOpenGLWidget_opengl_widget.h"
#include "../../../display/qt/windows/control_panel_window.h"
#include "../../../display/qt/dialogs/resolution_dialog.h"
#include "../../../display/qt/dialogs/overlay_dialog.h"
#include "../../../display/qt/windows/output_window.h"
#include "../../../capture/capture.h"
#include "../../../capture/alias.h"
#include "../../../common/globals.h"
#include "../../../record/record.h"
#include "../../../scaler/scaler.h"
#include "ui_output_window.h"

/// Temp. Stores the number of milliseconds passed for each frame update. This
/// number includes everything done to the frame - capture, scaling, and display.
int UPDATE_LATENCY_PEAK = 0;
int UPDATE_LATENCY_AVG = 0;

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

    // Set up a layout for the central widget now, so we can add the OpenGL
    // render surface to it when OpenGL is enabled.
    QVBoxLayout *const mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);

    controlPanel = new ControlPanel(this);
    overlayDlg = new OverlayDialog(this);

    if (controlPanel->custom_program_styling_enabled())
    {
        apply_programwide_styling(":/res/stylesheets/appstyle-gray.qss");
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

    // Locate the control panel in a convenient position on the screen.
    controlPanel->move(std::min(QGuiApplication::primaryScreen()->availableSize().width() - controlPanel->width(),
                                this->width() + (this->frameSize() - this->size()).width()), 0);
    controlPanel->show();

    this->move(0, 0);
    this->activateWindow();
    this->raise();

    return;
}

MainWindow::~MainWindow()
{
    delete ui;
    ui = nullptr;

    delete controlPanel;
    controlPanel = nullptr;

    delete overlayDlg;
    overlayDlg = nullptr;

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

// Loads a QSS stylesheet from the given file, and assigns it to the entire
// program. Returns true if the file was successfully opened; false otherwise;
// will not signal whether actually assigning the stylesheet succeeded or not.
bool MainWindow::apply_programwide_styling(const QString &filename)
{
    // Take an empty filename to mean that all custom stylings should be removed.
    if (filename.isEmpty())
    {
        qApp->setStyleSheet("");

        return true;
    }

    // Apply the given style, prepended by an OS-specific font style.
    QFile styleFile(filename);
    #if _WIN32
        QFile defaultFontStyleFile(":/res/stylesheets/font-windows.qss");
    #else
        QFile defaultFontStyleFile(":/res/stylesheets/font-linux.qss");
    #endif
    if (styleFile.open(QIODevice::ReadOnly) &&
        defaultFontStyleFile.open(QIODevice::ReadOnly))
    {
        qApp->setStyleSheet(QString("%1 %2").arg(QString(defaultFontStyleFile.readAll()))
                                            .arg(QString(styleFile.readAll())));

        return true;
    }

    return false;
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
        ui->centralwidget->layout()->removeWidget(OGL_SURFACE);
        delete OGL_SURFACE;
        OGL_SURFACE = nullptr;
    }

    return;
}

void MainWindow::closeEvent(QCloseEvent*)
{
    PROGRAM_EXIT_REQUESTED = 1;

    k_assert(controlPanel != nullptr, "");
    controlPanel->close();

    k_assert(overlayDlg != nullptr, "");
    overlayDlg->close();

    return;
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Toggle the window border on/off.
    if (event->button() == Qt::LeftButton )
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

    return;
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MidButton)
    {
        // If the capture window is pressed with the middle mouse button, show the overlay dialog.
        show_overlay_dialog();
    }

    return;
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if ((controlPanel != nullptr) &&
        controlPanel->is_mouse_wheel_scaling_allowed())
    {
        // Adjust the size of the capture window with the mouse scroll wheel.
        const int dir = (event->angleDelta().y() < 0)? 1 : -1;
        controlPanel->adjust_output_scaling(dir);
    }

    return;
}

// Returns the current overlay as a QImage, or a null QImage if the overlay
// should not be shown at this time.
//
QImage MainWindow::overlay_image(void)
{
    if (!kc_no_signal() &&
        overlayDlg != nullptr &&
        controlPanel->is_overlay_enabled())
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
    // Assign ctrl + number key 1-9 to the input resolution force buttons.
    for (uint i = 1; i <= 9; i++)
    {
        QShortcut *s = new QShortcut(QKeySequence(QString("ctrl+%1").arg(QString::number(i))), this);
        s->setContext(Qt::ApplicationShortcut);
        connect(s, &QShortcut::activated, [this, i]{this->controlPanel->activate_capture_res_button(i);});
    }

    // Assign alt + arrow keys to move the capture input alignment horizontally and vertically.
    QShortcut *altShiftLeft = new QShortcut(QKeySequence("alt+shift+left"), this);
    altShiftLeft->setContext(Qt::ApplicationShortcut);
    connect(altShiftLeft, &QShortcut::activated, []{kc_adjust_video_horizontal_offset(1);});
    QShortcut *altShiftRight = new QShortcut(QKeySequence("alt+shift+right"), this);
    altShiftRight->setContext(Qt::ApplicationShortcut);
    connect(altShiftRight, &QShortcut::activated, []{kc_adjust_video_horizontal_offset(-1);});
    QShortcut *altShiftUp = new QShortcut(QKeySequence("alt+shift+up"), this);
    altShiftUp->setContext(Qt::ApplicationShortcut);
    connect(altShiftUp, &QShortcut::activated, []{kc_adjust_video_vertical_offset(1);});
    QShortcut *altShiftDown = new QShortcut(QKeySequence("alt+shift+down"), this);
    altShiftDown->setContext(Qt::ApplicationShortcut);
    connect(altShiftDown, &QShortcut::activated, []{kc_adjust_video_vertical_offset(-1);});

    /// NOTE: Qt's full-screen mode might not work correctly under Linux, depending
    /// on the distro etc.
    QShortcut *f11 = new QShortcut(QKeySequence(Qt::Key_F11), this);
    f11->setContext(Qt::ApplicationShortcut);
    connect(f11, &QShortcut::activated, [this]
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

    QShortcut *f5 = new QShortcut(QKeySequence(Qt::Key_F5), this);
    f5->setContext(Qt::ApplicationShortcut);
    connect(f5, &QShortcut::activated, []{if (!kc_no_signal()) ALIGN_CAPTURE = true;});

    QShortcut *ctrlV = new QShortcut(QKeySequence("ctrl+v"), this);
    ctrlV->setContext(Qt::ApplicationShortcut);
    connect(ctrlV, &QShortcut::activated, [this]{this->controlPanel->open_video_adjust_dialog();});

    QShortcut *ctrlA = new QShortcut(QKeySequence("ctrl+a"), this);
    ctrlA->setContext(Qt::ApplicationShortcut);
    connect(ctrlA, &QShortcut::activated, [this]{this->controlPanel->open_antitear_dialog();});

    QShortcut *ctrlF = new QShortcut(QKeySequence("ctrl+f"), this);
    ctrlF->setContext(Qt::ApplicationShortcut);
    connect(ctrlF, &QShortcut::activated, [this]{this->controlPanel->open_filter_sets_dialog();});

    QShortcut *ctrlO = new QShortcut(QKeySequence("ctrl+o"), this);
    ctrlO->setContext(Qt::ApplicationShortcut);
    connect(ctrlO, &QShortcut::activated, [this]{this->controlPanel->toggle_overlay();});

    return;
}

void MainWindow::signal_new_known_alias(const mode_alias_s a)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->notify_of_new_alias(a);

    return;
}

void MainWindow::signal_new_mode_settings_source_file(const std::string &filename)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->notify_of_new_mode_settings_source_file(QString::fromStdString(filename));

    return;
}

// Updates the window title with the capture's current status.
//
void MainWindow::update_window_title()
{
    QString scale, latency, inRes, recording;

    if (kc_no_signal())
    {
        latency = " - No signal";
    }
    else if (kc_is_invalid_signal())
    {
        latency = " - Invalid signal";
    }
    else
    {
        const resolution_s rin = kc_hardware().status.capture_resolution();
        const resolution_s rout = ks_output_resolution();
        const int size = round((rout.h / (real)rin.h) * 100);

        // Estimate the % by which the input image is scaled up/down.
        if ((rin.w == rout.w) &&
            (rin.h == rout.h))
        {
            scale = QString(" scaled to %1%").arg(size);
        }
        else
        {
            scale = QString(" scaled to ~%1%").arg(size);
        }

        if (kc_are_frames_being_missed())
        {
            latency = " (dropping frames)";
        }

        if (krecord_is_recording())
        {
            recording = "[REC] ";
        }

        inRes = " - " + controlPanel->GetString_InputResolution();
    }

    setWindowTitle(recording + QString(PROGRAM_NAME) + inRes + scale + latency);

    return;
}

void MainWindow::set_capture_info_as_no_signal()
{
    update_window_title();

    k_assert(controlPanel != nullptr, "");
    controlPanel->set_capture_info_as_no_signal();

    return;
}

void MainWindow::set_capture_info_as_receiving_signal()
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->set_capture_info_as_receiving_signal();

    return;
}

void MainWindow::update_output_framerate(const u32 fps,
                                         const bool missedFrames)
{
    update_window_title();

    k_assert(controlPanel != nullptr, "");
    controlPanel->update_output_framerate(fps, missedFrames);

    return;
}

void MainWindow::update_filter_set_idx(void)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->update_filter_set_idx();

    return;
}

void MainWindow::update_filter_sets_list(void)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->update_filter_sets_list();

    return;
}

void MainWindow::update_video_params(void)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->update_video_params();

    return;
}

void MainWindow::update_capture_signal_info(void)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->update_capture_signal_info();

    return;
}

void MainWindow::update_recording_metainfo(void)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->update_recording_metainfo();

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
    k_assert(controlPanel != nullptr, "");
    controlPanel->clear_known_aliases();

    return;
}

QString MainWindow::GetString_OutputResolution()
{
    k_assert(controlPanel != nullptr, "");
    return controlPanel->GetString_OutputResolution();
}

QString MainWindow::GetString_InputResolution()
{
    k_assert(controlPanel != nullptr, "");
    return controlPanel->GetString_InputResolution();
}

QString MainWindow::GetString_InputRefreshRate()
{
    k_assert(controlPanel != nullptr, "");
    return controlPanel->GetString_InputRefreshRate();
}

QString MainWindow::GetString_OutputLatency()
{
    k_assert(controlPanel != nullptr, "");
    return controlPanel->GetString_OutputLatency();
}

QString MainWindow::GetString_OutputFrameRate()
{
    k_assert(controlPanel != nullptr, "");
    return controlPanel->GetString_OutputFrameRate();
}

QString MainWindow::GetString_DroppingFrames()
{
    k_assert(controlPanel != nullptr, "");
    return controlPanel->GetString_DroppingFrames();
}

void MainWindow::signal_that_overlay_is_enabled(const bool enabled)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->set_overlay_indicator_checked(enabled);

    return;
}

void MainWindow::show_overlay_dialog()
{
    k_assert(overlayDlg != nullptr, "");
    overlayDlg->show();
    overlayDlg->activateWindow();
    overlayDlg->raise();

    return;
}

void MainWindow::update_window_size()
{
    const resolution_s r = ks_output_resolution();

    this->setFixedSize(r.w, r.h);
    overlayDlg->set_overlay_max_width(r.w);

    update_window_title();

    k_assert(controlPanel != nullptr, "");
    controlPanel->update_output_resolution_info();

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
    k_assert(controlPanel != nullptr, "");
    controlPanel->add_gui_log_entry(e);

    return;
}
