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
#include <QTreeWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QShortcut>
#include <QPainter>
#include <QImage>
#include <QLabel>
#include "d_resolution_dialog.h"
#include "d_overlay_dialog.h"
#include "d_control_panel.h"
#include "ui_d_window.h"
#include "../capture.h"
#include "../common.h"
#include "../scaler.h"
#include "d_window.h"
#include "w_opengl.h"
#include "../main.h"

/// Temp. Stores the number of milliseconds passed for each frame update. This
/// number includes everything done to the frame - capture, scaling, and display.
int UPDATE_LATENCY_PEAK = 0;
int UPDATE_LATENCY_AVG = 0;

// A text document object drawn over the capture window to get an on-screen
// display effect.
static QTextDocument OVERLAY;

#ifdef USE_OPENGL
    static OGLWidget *OGL_SURFACE = nullptr;
#endif

 #include <QVBoxLayout>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Initialize OpenGL.
    #ifdef USE_OPENGL
        OGL_SURFACE = new OGLWidget(this);
        OGL_SURFACE->show();
        OGL_SURFACE->raise();

        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setVersion(1, 2);
        format.setSwapInterval(0); // Vsync.
        format.setSamples(0);
        format.setProfile(QSurfaceFormat::CoreProfile);
        QSurfaceFormat::setDefaultFormat(format);

        QVBoxLayout *const mainLayout = new QVBoxLayout(ui->centralwidget);
        mainLayout->setMargin(0);
        mainLayout->setSpacing(0);

        mainLayout->addWidget(OGL_SURFACE);
    #endif

    OVERLAY.setDocumentMargin(0);
    OVERLAY.setTextWidth(width());    // Fix HTML align not working.

    controlPanel = new ControlPanel(this);
    overlayDlg = new OverlayDialog(this);

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
    controlPanel->move(this->width() + (this->frameSize() - this->size()).width(), 0);
    controlPanel->show();

    this->move(0, 0);
    this->activateWindow();
    this->raise();

    return;
}

MainWindow::~MainWindow()
{
    delete ui; ui = nullptr;
    delete controlPanel; controlPanel = nullptr;
    delete overlayDlg; overlayDlg = nullptr;

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

void MainWindow::paintEvent(QPaintEvent *)
{
    static QLabel *magnifyingGlass = nullptr;

    const QImage capturedImg = ks_scaler_output_buffer_as_qimage();
    // Draw the captured and scaled frame to screen.
    #if !USE_OPENGL
        QPainter painter(this);

        if (!capturedImg.isNull())
        {
            int posX = 0;
            int posY = 0;

            if (ks_is_output_padding_enabled())
            {
                const auto sourceRes = ks_output_resolution();
                const auto targetRes = ks_padded_output_resolution();

                posX = (((int)targetRes.w - (int)sourceRes.w) / 2);
                posY = (((int)targetRes.h - (int)sourceRes.h) / 2);

                // Clear the window, since we likely have automatic clearing off
                // and padding may leave areas which aren't painted over by the
                // captured image.
                painter.fillRect(0, 0, this->width(), this->height(), QBrush("black"));
            }

            painter.drawImage(posX, posY, capturedImg);
        }

        /// FIXME: Overlay won't get drawn when using OpenGL.
        // Draw the overlay, if enabled.
        if (//!kc_no_signal() &&
            overlayDlg != nullptr &&
            overlayDlg->is_overlay_enabled())
        {
            OVERLAY.setHtml(overlayDlg->overlay_html_string());
            OVERLAY.drawContents(&painter, this->rect());
        }
    #endif

    // If the user pressed the right mouse button inside the main capture window,
    // show a magnifying glass effect which blows up part of the captured image.
    if (!kc_is_no_signal() &&
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
                                           "border-width: 2px;"
                                           "border-radius: 1px;");
        }

        const QSize subRegionSize = QSize(40, 30);
        const QSize glassSize = QSize(280, 210);
        const QPoint cursorPos = this->mapFromGlobal(QCursor::pos());
        QPoint adjPos = QPoint((cursorPos.x() - (subRegionSize.width() / 2)),
                               (cursorPos.y() - (subRegionSize.height() / 2)));

        // Don't let the magnification overflow the image buffer.
        if (adjPos.x() < 0)
        {
            adjPos.setX(0);
        }
        else if (adjPos.x() > (capturedImg.width() - subRegionSize.width()))
        {
            adjPos.setX(capturedImg.width() - subRegionSize.width());
        }
        if (adjPos.y() < 0)
        {
            adjPos.setY(0);
        }
        else if (adjPos.y() > (capturedImg.height() - subRegionSize.height()))
        {
            adjPos.setY(capturedImg.height() - subRegionSize.height());
        }

        // Get a small subregion of the capture image to magnify.
        const u32 startIdx = (adjPos.x() + adjPos.y() * capturedImg.width()) * (capturedImg.depth() / 8);
        QImage magn(capturedImg.bits() + startIdx, subRegionSize.width(), subRegionSize.height(),
                    capturedImg.bytesPerLine(), capturedImg.format());

        magnifyingGlass->resize(glassSize);
        magnifyingGlass->setPixmap(QPixmap::fromImage(magn.scaled(glassSize.width(), glassSize.height(),
                                                                  Qt::IgnoreAspectRatio, Qt::FastTransformation)));
        magnifyingGlass->move(cursorPos.x() - (glassSize.width() / 2),
                              cursorPos.y() - (glassSize.height() / 2));
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

        kd_update_gui_output_framerate_info(fps, kc_capturer_has_missed_frames());
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
    QShortcut *shortcutInputRes640x400 = new QShortcut(QKeySequence(Qt::Key_F1), this);
    shortcutInputRes640x400->setContext(Qt::ApplicationShortcut);
    connect(shortcutInputRes640x400, SIGNAL(activated()),
            this, SLOT(force_input_resolution_640x400()));

    QShortcut *shortcutInputRes720x400 = new QShortcut(QKeySequence(Qt::Key_F2), this);
    shortcutInputRes720x400->setContext(Qt::ApplicationShortcut);
    connect(shortcutInputRes720x400, SIGNAL(activated()),
            this, SLOT(force_input_resolution_720x400()));

    return;
}

void MainWindow::force_input_resolution_640x400()
{
    const resolution_s r = {640, 400, 0};
    DEBUG(("Received a keyboard shortcut request to set the input resolution to %u x %u.",
           r.w, r.h));

    kmain_change_capture_input_resolution(r);

    return;
}

void MainWindow::force_input_resolution_720x400()
{
    const resolution_s r = {720, 400, 0};
    DEBUG(("Received a keyboard shortcut request to set the input resolution to %u x %u.",
           r.w, r.h));

    kmain_change_capture_input_resolution(r);

    return;
}

void MainWindow::signal_new_known_alias(const mode_alias_s a)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->notify_of_new_alias(a);

    return;
}

void MainWindow::signal_new_known_mode(const resolution_s r)
{
    //k_assert(controlPanel != nullptr, "");
    //controlPanel->notify_of_new_known_mode(r);

    return;
}

void MainWindow::signal_new_mode_settings_source_file(const QString &filename)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->notify_of_new_mode_settings_source_file(filename);

    return;
}

// Updates the window title with the capture's current status.
//
void MainWindow::update_window_title()
{
    QString scale, latency, inRes;

    if (kc_is_no_signal())
    {
        latency = " - No signal";
    }
    else if (kc_is_invalid_signal())
    {
        latency = " - Invalid signal";
    }
    else
    {
        const resolution_s rin = kc_input_resolution();
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

        if (kc_capturer_has_missed_frames())
        {
            latency = " (dropping frames)";
        }

        inRes = " - " + controlPanel->GetString_InputResolution();
    }

    setWindowTitle(QString(PROGRAM_NAME) + inRes + scale + latency);

    return;
}

void MainWindow::set_input_info_as_no_signal()
{
    update_window_title();

    k_assert(controlPanel != nullptr, "");
    controlPanel->set_input_info_as_no_signal();

    return;
}

void MainWindow::set_input_info_as_receiving_signal()
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->set_input_info_as_receiving_signal();

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

void MainWindow::update_current_filter_set_idx(const int idx)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->update_current_filter_set_idx(idx);

    return;
}

void MainWindow::update_input_signal_info(const input_signal_s &s)
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->update_input_signal_info(s);

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

void MainWindow::clear_known_modes()
{
    k_assert(controlPanel != nullptr, "");
    controlPanel->clear_known_modes();

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

void MainWindow::set_overlay_enabled(const bool state)
{
    k_assert(overlayDlg != nullptr, "");
    overlayDlg->set_overlay_enabled(state);

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
    const resolution_s r = ks_padded_output_resolution();

    this->setFixedSize(r.w, r.h);

    update_window_title();

    k_assert(controlPanel != nullptr, "");
    controlPanel->update_output_resolution_info(r);

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
        this->setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
        this->show();

        update_window_size();
    }
    else                    // Hide the border.
    {
        this->move(0, 0);

        this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
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
