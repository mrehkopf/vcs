/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QCOMBOBOX_VIDEO_PRESET_LIST_H
#define VCS_DISPLAY_QT_SUBCLASSES_QCOMBOBOX_VIDEO_PRESET_LIST_H

#include <deque>
#include <QComboBox>

struct video_preset_s;

class VideoPresetList : public QComboBox
{
    Q_OBJECT

public:
    explicit VideoPresetList(QWidget *parent = 0);

    ~VideoPresetList(void);

    void add_preset(const unsigned presetId,
                    const bool blockSignal = false);

    // Removes the given present from the list, and sets the list's index
    // to the one given. If the given preset doesn't exist in the list, no
    // action will be taken.
    void remove_preset(const unsigned presetId);

    void remove_all_presets(void);

    void sort_alphabetically(void);

    void update_preset_item_label(const unsigned presetId);

    // Returns a pointer to the currently-selected preset; or nullptr if
    // no preset is selected.
    video_preset_s* current_preset(void) const;

signals:
    void preset_added(const unsigned presetId);

    void preset_removed(const unsigned presetId);

    void list_became_empty(void);

    void list_became_populated(void);

    void preset_selected(const unsigned presetId);

    void preset_label_changed(const unsigned presetId,
                              const QString &newLabel);

private:
    // Returns the list index of the given preset; or -1 if the list doesn't
    // contain the preset.
    int find_preset_idx_in_list(const int presetId);

    // Creates and returns a string to be displayed as the list item label for
    // the given present should the preset be added.
    QString make_preset_item_label(const video_preset_s *const preset);

    struct
    {
    public:
        void push(const unsigned presetId)
        {
            this->_history.push_back(presetId);

            if (this->_history.size() > maxLength)
            {
                this->_history.pop_front();
            }
        }

        int back(void)
        {
            if (this->_history.empty())
            {
                return -1;
            }

            this->_history.pop_back();
            return this->_history.back();
        }

        bool empty(void)
        {
            return this->_history.empty();
        }

        void clear(void)
        {
            this->_history.clear();
        }

        const unsigned maxLength = 10;

    private:
        std::deque<unsigned> _history;
    } presetSelectionHistory;
};

#endif
