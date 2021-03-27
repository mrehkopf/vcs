#ifndef VCS_DISPLAY_QT_WIGETS_FILTER_WIDGETS_H
#define VCS_DISPLAY_QT_WIGETS_FILTER_WIDGETS_H

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QComboBox>
#include <QWidget>
#include <QObject>
#include <QFrame>
#include <QLabel>
#include <unordered_map>
#include <cstring>
#include <string>
#include "common/globals.h"
#include "filter/filter.h"
#include "common/types.h"

struct filter_widget_s : public QObject
{
    struct parameter_s
    {
        // Byte offset in the filter's parameter array where this parameter starts.
        unsigned offset = 0;

        // Number of bytes occupied by this parameter in the filter's parameter array.
        unsigned byteSize = 1;

        uint32_t defaultValue = 0;
    };

    filter_widget_s(const filter_type_enum_e filterType,
                    u8 *const parameterArray,
                    const u8 *const initialParameterValues = nullptr,
                    const std::vector<filter_widget_s::parameter_s> &parameters = {},
                    const unsigned minWidth = 220);

    virtual ~filter_widget_s(void);

    QWidget *widget = nullptr;

    uint32_t parameter(const unsigned offset)
    {
        k_assert(this->parameterArray, "Expected a non-null parameter array.");

        const auto &parameter = this->parameters.at(offset);

        uint32_t value = 0;

        for (unsigned i = 0; i < parameter.byteSize; i++)
        {
            value |= (this->parameterArray[offset + i] << (i * 8));
        }

        return value;
    }

    void set_parameter(const unsigned offset,
                       const uint32_t value,
                       const bool silent = false)
    {
        k_assert(this->parameterArray, "Expected a non-null parameter array.");

        const auto &parameter = this->parameters.at(offset);

        k_assert(!(value >> (parameter.byteSize * 8)), "Parameter value overflows allocated byte size.");

        for (unsigned i = 0; i < parameter.byteSize; i++)
        {
            this->parameterArray[parameter.offset + i] = (value >> (i * 8));
        }

        if (!silent)
        {
            emit this->parameter_changed();
        }
    }

    // The user-facing name of this filter - e.g. "Blur" for a blur filter.
    const QString title;

    // The string shown to the user in the widget if this filter has no
    // user-configurable parameters.
    const QString noParamsMsg = "(No parameters.)";

    u8 *const parameterArray;

    // The default width of the widget.
    const unsigned minWidth;

signals:
    // Emitted when the widget's parameter data is changed.
    void parameter_changed(void);

protected:
    template <unsigned Offset, typename T>
    static filter_widget_s::parameter_s make_parameter(const unsigned defaultValue)
    {
        filter_widget_s::parameter_s param;

        param.offset = Offset;
        param.byteSize = sizeof(T);
        param.defaultValue = defaultValue;

        return param;
    }

    // The parameters recognized by this filter.
    std::unordered_map<unsigned /*offset*/, filter_widget_s::parameter_s> parameters;

private:
    Q_OBJECT
};



struct filter_widget_input_gate_s : public filter_widget_s
{
    // Width and height reserve two bytes each.
    enum parameter_offset_e { PARAM_WIDTH = 0, PARAM_HEIGHT = 2};

    filter_widget_input_gate_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_output_gate_s : public filter_widget_s
{
    // Width and height reserve two bytes each.
    enum parameter_offset_e { PARAM_WIDTH = 0, PARAM_HEIGHT = 2};

    filter_widget_output_gate_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_blur_s : public filter_widget_s
{
private:
    Q_OBJECT

public:
    enum parameter_offset_e { PARAM_TYPE = 0, PARAM_KERNEL_SIZE = 1 };
    enum filter_type_e { FILTER_TYPE_BOX = 0, FILTER_TYPE_GAUSSIAN = 1 };

    filter_widget_blur_s(u8 *const parameterArray, const u8 *const initialParameterValues);
};



struct filter_widget_rotate_s : public filter_widget_s
{
    // Note: the rotation angle and scale reserve two bytes.
    enum parameter_offset_e { PARAM_ROT = 0, PARAM_SCALE = 2 };

    filter_widget_rotate_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_crop_s : public filter_widget_s
{
    // Note: x, y, width, and height reserve two bytes each.
    enum parameter_offset_e { PARAM_X = 0, PARAM_Y = 2, PARAM_WIDTH = 4, PARAM_HEIGHT = 6, PARAM_SCALER = 8 };

    filter_widget_crop_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_flip_s : public filter_widget_s
{
    enum parameter_offset_e { PARAM_AXIS = 0 };

    filter_widget_flip_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_median_s : public filter_widget_s
{
    enum parameter_offset_e { PARAM_KERNEL_SIZE = 0 };

    filter_widget_median_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_denoise_temporal_s : public filter_widget_s
{
    enum parameter_offset_e { PARAM_THRESHOLD = 0};
    enum filter_type_e { FILTER_TYPE_TEMPORAL = 0, FILTER_TYPE_SPATIAL = 1 };

    filter_widget_denoise_temporal_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_denoise_nonlocal_means_s : public filter_widget_s
{
    // Offsets in the paramData array of the various parameters' values.
    enum parameter_offset_e { PARAM_H = 0, PARAM_H_COLOR = 1, PARAM_TEMPLATE_WINDOW_SIZE = 2, PARAM_SEARCH_WINDOW_SIZE = 3};

    filter_widget_denoise_nonlocal_means_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_sharpen_s : public filter_widget_s
{
    filter_widget_sharpen_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_unsharp_mask_s : public filter_widget_s
{
    enum parameter_offset_e { PARAM_STRENGTH = 0, PARAM_RADIUS = 1 };

    filter_widget_unsharp_mask_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_decimate_s : public filter_widget_s
{
    enum parameter_offset_e { PARAM_TYPE = 0, PARAM_FACTOR = 1 };
    enum filter_type_e { FILTER_TYPE_NEAREST = 0, FILTER_TYPE_AVERAGE = 1 };

    filter_widget_decimate_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_delta_histogram_s : public filter_widget_s
{
    filter_widget_delta_histogram_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};



struct filter_widget_unique_count_s : public filter_widget_s
{
    enum parameter_offset_e { PARAM_THRESHOLD = 0,
                              PARAM_CORNER = 1,
                              PARAM_BG_COLOR = 2,
                              PARAM_TEXT_COLOR = 3,};

    filter_widget_unique_count_s(u8 *const parameterArray, const u8 *const initialParameterValues);

private:
    Q_OBJECT
};

#endif
