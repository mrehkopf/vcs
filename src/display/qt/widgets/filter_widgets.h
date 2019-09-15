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
#include "filter/filter.h"
#include "common/types.h"

struct filter_widget_s : public QObject
{
    Q_OBJECT

public:
    filter_widget_s(const filter_type_enum_e filterType, u8 *const parameterData, const unsigned minWidth = 220);
    virtual ~filter_widget_s();

    QWidget *widget = nullptr;

    // Fills the given byte array with the filter's default parameters.
    virtual void reset_parameter_data(void) = 0;

    // Initializes the filter's Qt widget, which contains all the filter's user-
    // accessible controls. The widget might contain, for instance, a spin box
    // for adjusting the radius of a blur filter.
    virtual void create_widget(void) = 0;

    // The user-facing name of this filter - e.g. "Blur" for a blur filter.
    const QString title;

    // The string shown to the user in the widget if this filter has no
    // user-configurable parameters.
    const QString noParamsMsg = "This filter has no parameters.";

    u8 *const parameterData;

    // The default width of the widget.
    const unsigned minWidth;
};



struct filter_widget_input_gate_s : public filter_widget_s
{
    // Width and height reserve two bytes each.
    enum data_offset_e { OFFS_WIDTH = 0, OFFS_HEIGHT = 2};

    filter_widget_input_gate_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::input_gate, parameterData, 180)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_output_gate_s : public filter_widget_s
{
    // Width and height reserve two bytes each.
    enum data_offset_e { OFFS_WIDTH = 0, OFFS_HEIGHT = 2};

    filter_widget_output_gate_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::output_gate, parameterData, 180)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_blur_s : public filter_widget_s
{
    enum data_offset_e { OFFS_TYPE = 0, OFFS_KERNEL_SIZE = 1 };
    enum filter_type_e { FILTER_TYPE_BOX = 0, FILTER_TYPE_GAUSSIAN = 1 };

    filter_widget_blur_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::blur, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_rotate_s : public filter_widget_s
{
    // Note: the rotation angle and scale reserve two bytes.
    enum data_offset_e { OFFS_ROT = 0, OFFS_SCALE = 2 };

    filter_widget_rotate_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::rotate, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_crop_s : public filter_widget_s
{
    // Note: x, y, width, and height reserve two bytes each.
    enum data_offset_e { OFFS_X = 0, OFFS_Y = 2, OFFS_WIDTH = 4, OFFS_HEIGHT = 6, OFFS_SCALER = 8 };

    filter_widget_crop_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::crop, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_flip_s : public filter_widget_s
{
    enum data_offset_e { OFFS_AXIS = 0 };

    filter_widget_flip_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::flip, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_median_s : public filter_widget_s
{
    enum data_offset_e { OFFS_KERNEL_SIZE = 0 };

    filter_widget_median_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::median, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_denoise_temporal_s : public filter_widget_s
{
    enum data_offset_e { OFFS_THRESHOLD = 0};
    enum filter_type_e { FILTER_TYPE_TEMPORAL = 0, FILTER_TYPE_SPATIAL = 1 };

    filter_widget_denoise_temporal_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::denoise_temporal, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_denoise_nonlocal_means_s : public filter_widget_s
{
    // Offsets in the paramData array of the various parameters' values.
    enum data_offset_e { OFFS_H = 0, OFFS_H_COLOR = 1, OFFS_TEMPLATE_WINDOW_SIZE = 2, OFFS_SEARCH_WINDOW_SIZE = 3};

    filter_widget_denoise_nonlocal_means_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::denoise_nonlocal_means, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_sharpen_s : public filter_widget_s
{
    filter_widget_sharpen_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::sharpen, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_unsharp_mask_s : public filter_widget_s
{
    enum data_offset_e { OFFS_STRENGTH = 0, OFFS_RADIUS = 1 };

    filter_widget_unsharp_mask_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::unsharp_mask, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_decimate_s : public filter_widget_s
{
    enum data_offset_e { OFFS_TYPE = 0, OFFS_FACTOR = 1 };
    enum filter_type_e { FILTER_TYPE_NEAREST = 0, FILTER_TYPE_AVERAGE = 1 };

    filter_widget_decimate_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::decimate, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_delta_histogram_s : public filter_widget_s
{
    filter_widget_delta_histogram_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::delta_histogram, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};



struct filter_widget_unique_count_s : public filter_widget_s
{
    enum data_offset_e { OFFS_THRESHOLD = 0, OFFS_CORNER = 1 };

    filter_widget_unique_count_s(u8 *const parameterData) :
        filter_widget_s(filter_type_enum_e::unique_count, parameterData)
    {
        create_widget();
        return;
    }

    void reset_parameter_data(void) override;

private:
    void create_widget(void) override;
};

#endif
