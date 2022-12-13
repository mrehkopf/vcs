#ifndef VCS_DISPLAY_QT_DIALOGS_VIDEO_PARAMETER_DIALOG_H
#define VCS_DISPLAY_QT_DIALOGS_VIDEO_PARAMETER_DIALOG_H

#include <deque>
#include "filter/filter.h"
#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"

struct video_preset_s;

namespace Ui {
class VideoPresetsDialog;
}

class VideoPresetsDialog : public VCSBaseDialog
{
    Q_OBJECT

public:
    explicit VideoPresetsDialog(QWidget *parent = 0);
    ~VideoPresetsDialog();

    // Called to inform the dialog of a new source file for video presets.
    void assign_presets(const std::vector<video_preset_s*> &presets);

    // Attempts to load the presets stored in the given file. Returns true on
    // success; false otherwise.
    bool load_presets_from_file(const QString &filename);

    bool save_video_presets_to_file(QString filename);

signals:
    // Emitted when the last item in the preset list is removed.
    void preset_list_became_empty(void);

    // Emitted when the user makes changest
    void preset_activation_rules_changed(const video_preset_s *const preset);

    // Emitted when an item is added to an empty preset list.
    void preset_list_no_longer_empty(void);

    // Emitted when the user has requested that a different preset be shown in
    // the GUI.
    void preset_change_requested(const unsigned newPresetId);

    // Emitted when presets are loaded from or saved to a file.
    void new_presets_source_file(const QString &filename);

private:
    void update_preset_control_ranges(void);

    void update_preset_controls_with_current_preset_data(void);

    QString make_preset_list_text(const video_preset_s *const preset);

    void broadcast_current_preset_parameters(void);

    void resort_preset_list(void);

    void update_active_preset_indicator(void);

    Ui::VideoPresetsDialog *ui;
};

#endif
