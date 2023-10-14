/*
 * 2021-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_ABSTRACT_GUI_H
#define VCS_COMMON_ABSTRACT_GUI_H

#include <functional>
#include <string>
#include <vector>

struct abstract_gui_widget_s
{
    virtual ~abstract_gui_widget_s(void){}
    std::function<void(const bool)> set_enabled;
    bool isInitiallyEnabled = true;
};

namespace abstract_gui
{
    enum class horizontal_align
    {
        left,
        right,
        center
    };

    struct label : abstract_gui_widget_s
    {
        std::function<void(const std::string&)> set_text;
        std::function<std::string(void)> get_text;

        std::string text;
    };

    struct button_get_open_filename : abstract_gui_widget_s
    {
        std::function<void(const std::string&)> on_success;

        std::string label;
        std::string filenameFilter;
    };

    struct text_edit : abstract_gui_widget_s
    {
        std::function<void(const std::string&)> set_text;
        std::function<std::string(void)> get_text;

        std::string text;
        std::size_t maxLength = 255;
    };

    struct checkbox : abstract_gui_widget_s
    {
        std::function<void(const bool)> set_value;
        std::function<bool(void)> get_value;

        std::string label;
    };

    struct spinner : abstract_gui_widget_s
    {
        std::function<void(const int)> set_value;
        std::function<int(void)> get_value;

        int maxValue = 0;
        int minValue = 0;
        std::string prefix = "";
        std::string suffix = "";
        horizontal_align alignment = horizontal_align::left;
    };

    struct double_spinner : abstract_gui_widget_s
    {
        std::function<void(const double)> set_value;
        std::function<double(void)> get_value;

        double maxValue = 0;
        double minValue = 0;
        int numDecimals = 1;
        double stepSize = 1;
        std::string prefix = "";
        std::string suffix = "";
        horizontal_align alignment = horizontal_align::left;
    };

    struct combo_box : abstract_gui_widget_s
    {
        std::function<void(const int)> set_value;
        std::function<int(void)> get_value = []{return 0;};

        std::vector<std::string> items;
    };
}

// A GUI field is a combination of a label and one or more widgets. For instance,
// a field for the settings of a blur filter might be "Radius [x]", where "Radius"
// is the label and [x] is a spinner widget.
struct abstract_gui_field_s
{
    const std::string label = "";
    const std::vector<abstract_gui_widget_s*> components;

    abstract_gui_field_s(const std::string label, const std::vector<abstract_gui_widget_s*> components) :
        label(label),
        components(components){}
};

// A framework-independent description of a GUI; to be converted into an actual
// user-facing GUI by whatever GUI framework VCS happens to be using.
struct abstract_gui_s
{
    abstract_gui_s(void) {}

    abstract_gui_s(std::function<void(abstract_gui_s *const)> initialize)
    {
        initialize(this);
    }

    ~abstract_gui_s(void)
    {
        for (auto &row: this->fields)
        {
            for (auto *const component: row.components)
            {
                delete component;
            }
        }

        return;
    }

    std::vector<abstract_gui_field_s> fields;

    enum class layout_e
    {
        form,
        vertical_box,
    } layout = layout_e::form;
};

#endif
