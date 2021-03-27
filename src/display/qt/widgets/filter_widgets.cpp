#include <QDebug>
#include <cmath>
#include "display/qt/widgets/filter_widgets.h"

filter_widget_s::filter_widget_s(const filter_type_enum_e filterType,
                                 u8 *const parameterArray,
                                 const u8 *const initialParameterValues,
                                 const std::vector<filter_widget_s::parameter_s> &parameters,
                                 const unsigned minWidth) :
    title(QString::fromStdString(kf_filter_name_for_type(filterType))),
    parameterArray(parameterArray),
    minWidth(minWidth)
{
    if (initialParameterValues)
    {
        memcpy(this->parameterArray, initialParameterValues, FILTER_PARAMETER_ARRAY_LENGTH);
    }
    else
    {
        memset(this->parameterArray, 0, FILTER_PARAMETER_ARRAY_LENGTH);
    }

    for (const auto &parameter: parameters)
    {
        k_assert((this->parameters.find(parameter.offset) == this->parameters.end()),
                 "Duplicate parameter offset.");

        this->parameters[parameter.offset] = parameter;

        if (!initialParameterValues)
        {
             this->set_parameter(parameter.offset, parameter.defaultValue, true);
        }
    }

    return;
}

filter_widget_s::~filter_widget_s(void)
{
    return;
}

filter_widget_blur_s::filter_widget_blur_s(u8 *const parameterArray,
                                           const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::blur,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_KERNEL_SIZE, u8>(10),
                     filter_widget_s::make_parameter<PARAM_TYPE, u8>(FILTER_TYPE_GAUSSIAN)})
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Blur type.
    QLabel *typeLabel = new QLabel("Type", frame);
    QComboBox *typeList = new QComboBox(frame);
    typeList->addItem("Box");
    typeList->addItem("Gaussian");
    typeList->setCurrentIndex(this->parameter(PARAM_TYPE));

    // Blur radius.
    QLabel *radiusLabel = new QLabel("Radius", frame);
    QDoubleSpinBox *radiusSpin = new QDoubleSpinBox(frame);
    radiusSpin->setRange(0.1, 25);
    radiusSpin->setDecimals(1);
    radiusSpin->setValue(this->parameter(PARAM_KERNEL_SIZE) / 10.0);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(typeLabel, typeList);
    l->addRow(radiusLabel, radiusSpin);

    connect(radiusSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        this->set_parameter(PARAM_KERNEL_SIZE, round(newValue * 10.0));
    });

    connect(typeList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int currentIdx)
    {
        this->set_parameter(PARAM_TYPE, ((currentIdx < 0)? 0 : currentIdx));
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_rotate_s::filter_widget_rotate_s(u8 *const parameterArray,
                                               const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::rotate,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_SCALE, u16>(100),
                     filter_widget_s::make_parameter<PARAM_ROT, u16>(0)})
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *rotLabel = new QLabel("Angle", frame);
    QDoubleSpinBox *rotSpin = new QDoubleSpinBox(frame);
    rotSpin->setDecimals(1);
    rotSpin->setRange(-360, 360);
    rotSpin->setValue(this->parameter(PARAM_ROT) / 10.0);

    QLabel *scaleLabel = new QLabel("Scale", frame);
    QDoubleSpinBox *scaleSpin = new QDoubleSpinBox(frame);
    scaleSpin->setDecimals(2);
    scaleSpin->setRange(0.01, 20);
    scaleSpin->setSingleStep(0.1);
    scaleSpin->setValue(this->parameter(PARAM_SCALE) / 100.0);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(rotLabel, rotSpin);
    l->addRow(scaleLabel, scaleSpin);

    connect(rotSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        this->set_parameter(PARAM_ROT, (newValue * 10));
    });

    connect(scaleSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        this->set_parameter(PARAM_SCALE, (newValue * 100));
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_input_gate_s::filter_widget_input_gate_s(u8 *const parameterArray,
                                                       const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::input_gate,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_WIDTH, u16>(640),
                     filter_widget_s::make_parameter<PARAM_HEIGHT, u16>(480)},
                    180)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *widthLabel = new QLabel("Width", frame);
    QSpinBox *widthSpin = new QSpinBox(frame);
    widthSpin->setRange(0, MAX_CAPTURE_WIDTH);
    widthSpin->setValue(this->parameter(PARAM_WIDTH));

    QLabel *heightLabel = new QLabel("Height", frame);
    QSpinBox *heightSpin = new QSpinBox(frame);
    heightSpin->setRange(0, MAX_CAPTURE_WIDTH);
    heightSpin->setValue(this->parameter(PARAM_HEIGHT));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(widthLabel, widthSpin);
    l->addRow(heightLabel, heightSpin);

    connect(widthSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_WIDTH, newValue);
    });

    connect(heightSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_HEIGHT, newValue);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_output_gate_s::filter_widget_output_gate_s(u8 *const parameterArray,
                                                         const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::output_gate,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_WIDTH, u16>(640),
                     filter_widget_s::make_parameter<PARAM_HEIGHT, u16>(480)},
                    180)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *widthLabel = new QLabel("Width", frame);
    QSpinBox *widthSpin = new QSpinBox(frame);
    widthSpin->setRange(0, MAX_OUTPUT_WIDTH);
    widthSpin->setValue(this->parameter(PARAM_WIDTH));

    QLabel *heightLabel = new QLabel("Height", frame);
    QSpinBox *heightSpin = new QSpinBox(frame);
    heightSpin->setRange(0, MAX_OUTPUT_HEIGHT);
    heightSpin->setValue(this->parameter(PARAM_HEIGHT));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(widthLabel, widthSpin);
    l->addRow(heightLabel, heightSpin);

    connect(widthSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_WIDTH, newValue);
    });

    connect(heightSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_HEIGHT, newValue);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_crop_s::filter_widget_crop_s(u8 *const parameterArray,
                                           const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::crop,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_X, u16>(0),
                     filter_widget_s::make_parameter<PARAM_Y, u16>(0),
                     filter_widget_s::make_parameter<PARAM_WIDTH, u16>(640),
                     filter_widget_s::make_parameter<PARAM_HEIGHT, u16>(480),
                     filter_widget_s::make_parameter<PARAM_SCALER, u8>(0)})
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *xLabel = new QLabel("X", frame);
    QSpinBox *xSpin = new QSpinBox(frame);
    xSpin->setRange(0, 65535);
    xSpin->setValue(this->parameter(PARAM_X));

    QLabel *yLabel = new QLabel("Y", frame);
    QSpinBox *ySpin = new QSpinBox(frame);
    ySpin->setRange(0, 65535);
    ySpin->setValue(this->parameter(PARAM_Y));

    QLabel *widthLabel = new QLabel("Width", frame);
    QSpinBox *widthSpin = new QSpinBox(frame);
    widthSpin->setRange(0, 65535);
    widthSpin->setValue(this->parameter(PARAM_WIDTH));

    QLabel *heightLabel = new QLabel("Height", frame);
    QSpinBox *heightSpin = new QSpinBox(frame);
    heightSpin->setRange(0, 65535);
    heightSpin->setValue(this->parameter(PARAM_HEIGHT));

    QLabel *scalerLabel = new QLabel("Scaler", frame);
    QComboBox *scalerList = new QComboBox(frame);
    scalerList->addItem("Linear");
    scalerList->addItem("Nearest");
    scalerList->addItem("(Don't scale)");
    scalerList->setCurrentIndex(this->parameter(PARAM_SCALER));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(xLabel, xSpin);
    l->addRow(yLabel, ySpin);
    l->addRow(widthLabel, widthSpin);
    l->addRow(heightLabel, heightSpin);
    l->addRow(scalerLabel, scalerList);

    connect(xSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_X, newValue);
    });

    connect(ySpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_Y, newValue);
    });

    connect(widthSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_WIDTH, newValue);
    });

    connect(heightSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_HEIGHT, newValue);
    });

    connect(scalerList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        this->set_parameter(PARAM_SCALER, newIndex);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_flip_s::filter_widget_flip_s(u8 *const parameterArray,
                                           const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::flip,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_AXIS, u8>(0)})
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *axisLabel = new QLabel("Axis", frame);
    QComboBox *axisList = new QComboBox(frame);
    axisList->addItem("Vertical");
    axisList->addItem("Horizontal");
    axisList->addItem("Both");
    axisList->setCurrentIndex(this->parameter(PARAM_AXIS));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(axisLabel, axisList);

    connect(axisList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        this->set_parameter(PARAM_AXIS, newIndex);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_median_s::filter_widget_median_s(u8 *const parameterArray,
                                               const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::median,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_KERNEL_SIZE, u8>(3)})
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Median radius.
    QLabel *radiusLabel = new QLabel("Radius", frame);
    QSpinBox *radiusSpin = new QSpinBox(frame);
    radiusSpin->setRange(0, 99);
    radiusSpin->setValue(this->parameter(PARAM_KERNEL_SIZE) / 2);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(radiusLabel, radiusSpin);

    connect(radiusSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_KERNEL_SIZE, ((newValue * 2) + 1));
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_denoise_temporal_s::filter_widget_denoise_temporal_s(u8 *const parameterArray,
                                                                   const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::denoise_temporal,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_THRESHOLD, u8>(5)})
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Denoising threshold.
    QLabel *thresholdLabel = new QLabel("Threshold", frame);
    QSpinBox *thresholdSpin = new QSpinBox(frame);
    thresholdSpin->setRange(0, 255);
    thresholdSpin->setValue(this->parameter(PARAM_THRESHOLD));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(thresholdLabel, thresholdSpin);

    connect(thresholdSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_THRESHOLD, newValue);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_denoise_nonlocal_means_s::filter_widget_denoise_nonlocal_means_s(u8 *const parameterArray,
                                                                               const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::denoise_nonlocal_means,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_H, u8>(10),
                     filter_widget_s::make_parameter<PARAM_H_COLOR, u8>(10),
                     filter_widget_s::make_parameter<PARAM_TEMPLATE_WINDOW_SIZE, u8>(7),
                     filter_widget_s::make_parameter<PARAM_SEARCH_WINDOW_SIZE, u8>(21)})
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *hLabel = new QLabel("Luminance", frame);
    QLabel *hColorLabel = new QLabel("Color", frame);
    QLabel *templateWindowLabel = new QLabel("Template wnd.", frame);
    QLabel *searchWindowLabel = new QLabel("Search wnd.", frame);

    QSpinBox *hSpin = new QSpinBox(frame);
    hSpin->setRange(0, 255);
    hSpin->setValue(this->parameter(PARAM_H));

    QSpinBox *hColorSpin = new QSpinBox(frame);
    hColorSpin->setRange(0, 255);
    hColorSpin->setValue(this->parameter(PARAM_H_COLOR));

    QSpinBox *templateWindowSpin = new QSpinBox(frame);
    templateWindowSpin->setRange(0, 255);
    templateWindowSpin->setValue(this->parameter(PARAM_TEMPLATE_WINDOW_SIZE));

    QSpinBox *searchWindowSpin = new QSpinBox(frame);
    searchWindowSpin->setRange(0, 255);
    searchWindowSpin->setValue(this->parameter(PARAM_SEARCH_WINDOW_SIZE));

    QFormLayout *layout = new QFormLayout(frame);
    layout->addRow(hColorLabel, hColorSpin);
    layout->addRow(hLabel, hSpin);
    layout->addRow(searchWindowLabel, searchWindowSpin);
    layout->addRow(templateWindowLabel, templateWindowSpin);

    connect(hSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_H, newValue);
    });

    connect(hColorSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_H_COLOR, newValue);
    });

    connect(templateWindowSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_TEMPLATE_WINDOW_SIZE, newValue);
    });

    connect(searchWindowSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_SEARCH_WINDOW_SIZE, newValue);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_sharpen_s::filter_widget_sharpen_s(u8 *const parameterArray,
                                                 const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::sharpen,
                    parameterArray,
                    initialParameterValues)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *noneLabel = new QLabel(this->noParamsMsg);
    noneLabel->setAlignment(Qt::AlignHCenter);

    QHBoxLayout *l = new QHBoxLayout(frame);
    l->addWidget(noneLabel);

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_unsharp_mask_s::filter_widget_unsharp_mask_s(u8 *const parameterArray,
                                                           const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::unsharp_mask,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_STRENGTH, u8>(50),
                     filter_widget_s::make_parameter<PARAM_RADIUS, u8>(10)})
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Strength.
    QLabel *strLabel = new QLabel("Strength", frame);
    QSpinBox *strSpin = new QSpinBox(frame);
    strSpin->setRange(0, 255);
    strSpin->setValue(this->parameter(PARAM_STRENGTH));

    // Radius.
    QLabel *radiusLabel = new QLabel("Radius", frame);
    QSpinBox *radiusdSpin = new QSpinBox(frame);
    radiusdSpin->setRange(1, 255);
    radiusdSpin->setValue(this->parameter(PARAM_RADIUS) / 10);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(strLabel, strSpin);
    l->addRow(radiusLabel, radiusdSpin);

    connect(strSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_STRENGTH, newValue);
    });

    connect(radiusdSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_RADIUS, (newValue * 10));
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_decimate_s::filter_widget_decimate_s(u8 *const parameterArray,
                                                   const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::decimate,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_FACTOR, u8>(2),
                     filter_widget_s::make_parameter<PARAM_TYPE, u8>(FILTER_TYPE_AVERAGE)})
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Factor.
    QLabel *factorLabel = new QLabel("Factor", frame);
    QComboBox *factorList = new QComboBox(frame);
    factorList->addItem("2");
    factorList->addItem("4");
    factorList->addItem("8");
    factorList->addItem("16");
    factorList->setCurrentIndex((round(sqrt(this->parameter(PARAM_FACTOR))) - 1));

    // Sampling.
    QLabel *radiusLabel = new QLabel("Sampling", frame);
    QComboBox *samplingList = new QComboBox(frame);
    samplingList->addItem("Nearest");
    samplingList->addItem("Average");
    samplingList->setCurrentIndex(this->parameter(PARAM_TYPE));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(factorLabel, factorList);
    l->addRow(radiusLabel, samplingList);

    connect(factorList, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), [this](const QString &newIndexText)
    {
        this->set_parameter(PARAM_FACTOR, newIndexText.toUInt());
    });

    connect(samplingList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        this->set_parameter(PARAM_TYPE, newIndex);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_delta_histogram_s::filter_widget_delta_histogram_s(u8 *const parameterArray,
                                                                 const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::delta_histogram,
                    parameterArray,
                    initialParameterValues)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *noneLabel = new QLabel(this->noParamsMsg);
    noneLabel->setAlignment(Qt::AlignHCenter);

    QHBoxLayout *l = new QHBoxLayout(frame);
    l->addWidget(noneLabel);

    frame->adjustSize();
    this->widget = frame;

    return;
}

filter_widget_unique_count_s::filter_widget_unique_count_s(u8 *const parameterArray,
                                                           const u8 *const initialParameterValues) :
    filter_widget_s(filter_type_enum_e::unique_count,
                    parameterArray,
                    initialParameterValues,
                    {filter_widget_s::make_parameter<PARAM_THRESHOLD, u8>(20),
                     filter_widget_s::make_parameter<PARAM_CORNER, u8>(0),
                     filter_widget_s::make_parameter<PARAM_BG_COLOR, u8>(0),
                     filter_widget_s::make_parameter<PARAM_TEXT_COLOR, u8>(0)})
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Denoising threshold.
    QLabel *thresholdLabel = new QLabel("Threshold", frame);
    QSpinBox *thresholdSpin = new QSpinBox(frame);
    thresholdSpin->setRange(0, 255);
    thresholdSpin->setValue(this->parameter(PARAM_THRESHOLD));

    // In which corner to show the counter.
    QLabel *cornerLabel = new QLabel("Show in", frame);
    QComboBox *cornerList = new QComboBox(frame);
    cornerList->addItem("Top left");
    cornerList->addItem("Top right");
    cornerList->addItem("Bottom right");
    cornerList->addItem("Bottom left");
    cornerList->setCurrentIndex(this->parameter(PARAM_CORNER));

    // Text color.
    QLabel *textColorLabel = new QLabel("Text", frame);
    QComboBox *textColorList = new QComboBox(frame);
    textColorList->addItem("Yellow");
    textColorList->addItem("Purple");
    textColorList->addItem("Black");
    textColorList->addItem("White");
    textColorList->setCurrentIndex(this->parameter(PARAM_TEXT_COLOR));

    // Background color.
    QLabel *bgColorLabel = new QLabel("Background", frame);
    QComboBox *bgColorList = new QComboBox(frame);
    bgColorList->addItem("Transparent");
    bgColorList->addItem("Black");
    bgColorList->addItem("White");
    bgColorList->setCurrentIndex(this->parameter(PARAM_BG_COLOR));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(cornerLabel, cornerList);
    l->addRow(textColorLabel, textColorList);
    l->addRow(bgColorLabel, bgColorList);
    l->addRow(thresholdLabel, thresholdSpin);

    connect(thresholdSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        this->set_parameter(PARAM_THRESHOLD, newValue);
    });

    connect(cornerList, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        this->set_parameter(PARAM_CORNER, newIndex);
    });

    connect(bgColorList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        this->set_parameter(PARAM_BG_COLOR, newIndex);
    });

    connect(textColorList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        this->set_parameter(PARAM_TEXT_COLOR, newIndex);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}
