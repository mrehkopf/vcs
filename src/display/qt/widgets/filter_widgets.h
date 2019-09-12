#ifndef FILTER_WIDGETS_H
#define FILTER_WIDGETS_H


#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QComboBox>
#include <QWidget>
#include <QObject>
#include <QFrame>
#include <QLabel>
#include <string>
#include "common/globals.h"
#include "common/types.h"

struct filter_widget_s : public QObject
{
    Q_OBJECT

public:
    filter_widget_s(u8 *const filterData, const char *const name);
    ~filter_widget_s();

    QWidget *widget = nullptr;

    // Fills the given byte array with the filter's default parameters.
    virtual void reset_data(void) = 0;

    // Initializes the filter's Qt widget, which contains all the filter's user-
    // accessible controls. The widget might contain, for instance, a spin box
    // for adjusting the radius of a blur filter.
    virtual void create_widget(void) = 0;

    // The user-facing name of this filter - e.g. "Blur" for a blur filter.
    const std::string name;

    // The string shown to the user in the widget if this filter has no
    // user-configurable parameters.
    const std::string noParamsMsg = "This filter takes no parameters.";

    // The default width of the widget.
    const unsigned minWidth = 240;

    u8 *const filterData;
};

struct filter_widget_blur_s : public filter_widget_s
{
    enum data_offset_e { OFF_TYPE = 0, OFF_KERNEL_SIZE = 1 };
    enum filter_type_e { FILTER_TYPE_BOX = 0, FILTER_TYPE_GAUSSIAN = 1 };

    filter_widget_blur_s(u8 *const filterData) :
        filter_widget_s(filterData, "Blur")
    {
        create_widget();
        return;
    }

    void reset_data(void) override;

private:
    void create_widget() override;
};

struct filter_widget_rotate_s : public filter_widget_s
{
    // Note: the rotation angle and scale reserve two bytes.
    enum data_offset_e { OFFS_ROT = 0, OFFS_SCALE = 2 };

    filter_widget_rotate_s(u8 *const filterData) :
        filter_widget_s(filterData, "Rotate")
    {
        create_widget();
        return;
    }

    void reset_data(void) override;

private:
    void create_widget() override;
};

#endif
