/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_WINDOWS_OUTPUT_WINDOW_H
#define VCS_DISPLAY_QT_WINDOWS_OUTPUT_WINDOW_H

#include <QMainWindow>
#include "display/display.h"
#include "common/globals.h"

class OutputResolutionDialog;
class InputResolutionDialog;
class VideoParameterDialog;
class FilterGraphDialog;
class AntiTearDialog;
class OverlayDialog;
class RecordDialog;
class SignalDialog;
class AliasDialog;
class AboutDialog;
class VCSBaseDialog;
class QAction;

struct signal_info_s;
struct mode_alias_s;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    // Gets user input in the GUI, etc.
    void update_gui_state(void);

    // Returns true if the window has a border.
    bool window_has_border(void);

    void add_gui_log_entry(const log_entry_s e);

    void update_window_size(void);

    // Updates the window title with the current status of capture and that of the program
    // in general.
    void update_window_title(void);

    void set_opengl_enabled(const bool enabled);

    void redraw(void);

    // Returns the current overlay as a QImage, or a null QImage if the overlay
    // should not be shown at this time.
    QImage overlay_image(void);

    // Returns true if the user is allowed to scale the output resolution by using the
    // mouse wheel over the capture window.
    bool is_mouse_wheel_scaling_allowed(void);

    bool apply_programwide_styling(const QString &filename);

    OutputResolutionDialog* output_resolution_dialog(void) const { return this->outputResolutionDlg; }
    InputResolutionDialog* input_resolution_dialog(void)    const { return this->inputResolutionDlg; }
    VideoParameterDialog* video_presets_dialog(void)        const { return this->videoParamDlg; }
    FilterGraphDialog* filter_graph_dialog(void)            const { return this->filterGraphDlg; }
    AntiTearDialog* anti_tear_dialog(void)                  const { return this->antitearDlg; }
    OverlayDialog* overlay_dialog(void)                     const { return this->overlayDlg; }
    RecordDialog* recordDialog(void)                        const { return this->recordDlg; }
    SignalDialog* signal_info_dialog(void)                  const { return this->signalDlg; }
    AliasDialog* alias_resolutions_dialog(void)             const { return this->aliasDlg; }
    AboutDialog* about_dialog(void)                         const { return this->aboutDlg; }

signals:
    void entered_fullscreen(void);
    void left_fullscreen(void);

    // Emitted when the window border is toggled.
    void border_hidden(void);
    void border_revealed(void);

private slots:
    void toggle_window_border(void);

private:
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void changeEvent(QEvent *event);
    void paintEvent(QPaintEvent *);
    void closeEvent(QCloseEvent *event);
    void wheelEvent(QWheelEvent *event);

    void set_keyboard_shortcuts(void);

    // Load the font pointed to by the given filename, and make it available to the program.
    bool load_font(const QString &filename);

    void update_context_menu_eyedropper(const QPoint &scalerOutputPos);

    Ui::MainWindow *ui = nullptr;

    // The menu items shown when the user right-clicks this window.
    std::vector<QMenu*> contextMenuSubmenus;
    QMenu *contextMenu;

    OutputResolutionDialog *outputResolutionDlg = nullptr;
    InputResolutionDialog *inputResolutionDlg = nullptr;
    VideoParameterDialog *videoParamDlg = nullptr;
    FilterGraphDialog *filterGraphDlg = nullptr;
    AntiTearDialog *antitearDlg = nullptr;
    OverlayDialog *overlayDlg = nullptr;
    RecordDialog *recordDlg = nullptr;
    SignalDialog *signalDlg = nullptr;
    AliasDialog *aliasDlg = nullptr;
    AboutDialog *aboutDlg = nullptr;

    // A master list of all the dialogs that this window spawns. Will be filled in
    // when we create the dialog instances.
    QVector<VCSBaseDialog*> dialogs;

    // Set to true when the user has selected to close the
    // main window.
    bool userExiting = false;

    // Set to true if the program failed to initialize the capture card.
    bool captureInitFail = false;

    // A user-definable custom title for the output window.
    QString windowTitleOverride = "";

    // Set to true if the capture subsystem is currently reporting that captured frames
    // are being dropped; false otherwise. The value gets updated at some interval,
    // e.g. 1 second.
    bool areFramesBeingDropped = false;

    // Displayed in the window's context menu; shows the color values of the
    // pixel under the cursor where the context menu was spawned.
    QAction *contextMenuEyedropper;

    const Qt::WindowFlags defaultWindowFlags = (Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
};

#endif
