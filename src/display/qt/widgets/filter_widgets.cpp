#include <QDebug>
#include <cmath>
#include "display/qt/widgets/filter_widgets.h"

filter_widget_s::filter_widget_s(const filter_type_enum_e filterType,
                                 u8 *const parameterArray,
                                 const u8 *const initialParameterValues,
                                 const unsigned minWidth) :
    title(QString::fromStdString(kf_filter_name_for_type(filterType))),
    parameterArray(parameterArray),
    minWidth(minWidth)
{
    if (initialParameterValues)
    {
        memcpy(parameterArray, initialParameterValues, FILTER_PARAMETER_ARRAY_LENGTH);
    }

    return;
}

filter_widget_s::~filter_widget_s()
{
    return;
}

void filter_widget_blur_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    this->parameterArray[OFFS_KERNEL_SIZE] = 10;
    this->parameterArray[OFFS_TYPE] = FILTER_TYPE_GAUSSIAN;

    return;
}

void filter_widget_blur_s::create_widget(void)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Blur type.
    QLabel *typeLabel = new QLabel("Type:", frame);
    QComboBox *typeList = new QComboBox(frame);
    typeList->addItem("Box");
    typeList->addItem("Gaussian");
    typeList->setCurrentIndex(this->parameterArray[OFFS_TYPE]);

    // Blur radius.
    QLabel *radiusLabel = new QLabel("Radius:", frame);
    QDoubleSpinBox *radiusSpin = new QDoubleSpinBox(frame);
    radiusSpin->setRange(0.1, 25);
    radiusSpin->setDecimals(1);
    radiusSpin->setValue(this->parameterArray[OFFS_KERNEL_SIZE] / 10.0);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(typeLabel, typeList);
    l->addRow(radiusLabel, radiusSpin);

    connect(radiusSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_KERNEL_SIZE] = round(newValue * 10.0);
    });

    connect(typeList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int currentIdx)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_TYPE] = ((currentIdx == -1)? 0 : currentIdx);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

void filter_widget_rotate_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    // The scale value gets divided by 100 when used.
    *(i16*)&(this->parameterArray[OFFS_SCALE]) = 100;

    // The rotation value gets divided by 10 when used.
    *(i16*)&(this->parameterArray[OFFS_ROT]) = 0;

    return;
}

void filter_widget_rotate_s::create_widget(void)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *rotLabel = new QLabel("Angle:", frame);
    QDoubleSpinBox *rotSpin = new QDoubleSpinBox(frame);
    rotSpin->setDecimals(1);
    rotSpin->setRange(-360, 360);
    rotSpin->setValue(*(i16*)&(this->parameterArray[OFFS_ROT]) / 10.0);

    QLabel *scaleLabel = new QLabel("Scale:", frame);
    QDoubleSpinBox *scaleSpin = new QDoubleSpinBox(frame);
    scaleSpin->setDecimals(2);
    scaleSpin->setRange(0.01, 20);
    scaleSpin->setSingleStep(0.1);
    scaleSpin->setValue((*(i16*)&(this->parameterArray[OFFS_SCALE])) / 100.0);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(rotLabel, rotSpin);
    l->addRow(scaleLabel, scaleSpin);

    connect(rotSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
         *(i16*)&(this->parameterArray[OFFS_ROT]) = (newValue * 10);
    });

    connect(scaleSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        *(i16*)&(this->parameterArray[OFFS_SCALE]) = (newValue * 100);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

void filter_widget_input_gate_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    *(u16*)&(this->parameterArray[OFFS_WIDTH]) = 640;
    *(u16*)&(this->parameterArray[OFFS_HEIGHT]) = 480;
}

void filter_widget_input_gate_s::create_widget(void)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *widthLabel = new QLabel("Width:", frame);
    QSpinBox *widthSpin = new QSpinBox(frame);
    widthSpin->setRange(0, u16(~0u));
    widthSpin->setValue(*(u16*)&(this->parameterArray[OFFS_WIDTH]));

    QLabel *heightLabel = new QLabel("Height:", frame);
    QSpinBox *heightSpin = new QSpinBox(frame);
    heightSpin->setRange(0, u16(~0u));
    heightSpin->setValue(*(i16*)&(this->parameterArray[OFFS_HEIGHT]));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(widthLabel, widthSpin);
    l->addRow(heightLabel, heightSpin);

    connect(widthSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
         *(u16*)&(this->parameterArray[OFFS_WIDTH]) = newValue;
    });

    connect(heightSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
         *(u16*)&(this->parameterArray[OFFS_HEIGHT]) = newValue;
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

void filter_widget_output_gate_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    *(u16*)&(this->parameterArray[OFFS_WIDTH]) = 1920;
    *(u16*)&(this->parameterArray[OFFS_HEIGHT]) = 1080;

    return;
}

void filter_widget_output_gate_s::create_widget(void)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *widthLabel = new QLabel("Width:", frame);
    QSpinBox *widthSpin = new QSpinBox(frame);
    widthSpin->setRange(0, u16(~0u));
    widthSpin->setValue(*(u16*)&(this->parameterArray[OFFS_WIDTH]));

    QLabel *heightLabel = new QLabel("Height:", frame);
    QSpinBox *heightSpin = new QSpinBox(frame);
    heightSpin->setRange(0, u16(~0u));
    heightSpin->setValue(*(i16*)&(this->parameterArray[OFFS_HEIGHT]));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(widthLabel, widthSpin);
    l->addRow(heightLabel, heightSpin);

    connect(widthSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
         *(u16*)&(this->parameterArray[OFFS_WIDTH]) = newValue;
    });

    connect(heightSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
         *(u16*)&(this->parameterArray[OFFS_HEIGHT]) = newValue;
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

void filter_widget_crop_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    *(u16*)&(this->parameterArray[OFFS_X]) = 0;
    *(u16*)&(this->parameterArray[OFFS_Y]) = 0;
    *(u16*)&(this->parameterArray[OFFS_WIDTH]) = 640;
    *(u16*)&(this->parameterArray[OFFS_HEIGHT]) = 480;
    this->parameterArray[OFFS_SCALER] = 0;

    return;
}

void filter_widget_crop_s::create_widget(void)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *xLabel = new QLabel("X:", frame);
    QSpinBox *xSpin = new QSpinBox(frame);
    xSpin->setRange(0, 65535);
    xSpin->setValue(*(u16*)&(this->parameterArray[OFFS_X]));

    QLabel *yLabel = new QLabel("Y:", frame);
    QSpinBox *ySpin = new QSpinBox(frame);
    ySpin->setRange(0, 65535);
    ySpin->setValue(*(u16*)&(this->parameterArray[OFFS_Y]));

    QLabel *widthLabel = new QLabel("Width:", frame);
    QSpinBox *widthSpin = new QSpinBox(frame);
    widthSpin->setRange(0, 65535);
    widthSpin->setValue(*(u16*)&(this->parameterArray[OFFS_WIDTH]));

    QLabel *heightLabel = new QLabel("Height:", frame);
    QSpinBox *heightSpin = new QSpinBox(frame);
    heightSpin->setRange(0, 65535);
    heightSpin->setValue(*(u16*)&(this->parameterArray[OFFS_HEIGHT]));

    QLabel *scalerLabel = new QLabel("Scaler:", frame);
    QComboBox *scalerList = new QComboBox(frame);
    scalerList->addItem("Linear");
    scalerList->addItem("Nearest");
    scalerList->addItem("(Don't scale)");
    scalerList->setCurrentIndex(this->parameterArray[OFFS_SCALER]);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(xLabel, xSpin);
    l->addRow(yLabel, ySpin);
    l->addRow(widthLabel, widthSpin);
    l->addRow(heightLabel, heightSpin);
    l->addRow(scalerLabel, scalerList);

    connect(xSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
         *(u16*)&(this->parameterArray[OFFS_X]) = newValue;
    });

    connect(ySpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
         *(u16*)&(this->parameterArray[OFFS_Y]) = newValue;
    });

    connect(widthSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
         *(u16*)&(this->parameterArray[OFFS_WIDTH]) = newValue;
    });

    connect(heightSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
         *(u16*)&(this->parameterArray[OFFS_HEIGHT]) = newValue;
    });

    connect(scalerList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_SCALER] = newIndex;
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

void filter_widget_flip_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    return;
}

void filter_widget_flip_s::create_widget(void)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *axisLabel = new QLabel("Axis:", frame);
    QComboBox *axisList = new QComboBox(frame);
    axisList->addItem("Vertical");
    axisList->addItem("Horizontal");
    axisList->addItem("Both");
    axisList->setCurrentIndex(this->parameterArray[OFFS_AXIS]);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(axisLabel, axisList);

    connect(axisList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
         *(u16*)&(this->parameterArray[OFFS_AXIS]) = newIndex;
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

void filter_widget_median_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    this->parameterArray[OFFS_KERNEL_SIZE] = 3;

    return;
}

void filter_widget_median_s::create_widget(void)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Median radius.
    QLabel *radiusLabel = new QLabel("Radius:", frame);
    QSpinBox *radiusSpin = new QSpinBox(frame);
    radiusSpin->setRange(0, 99);
    radiusSpin->setValue(this->parameterArray[OFFS_KERNEL_SIZE] / 2);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(radiusLabel, radiusSpin);

    connect(radiusSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_KERNEL_SIZE] = ((newValue * 2) + 1);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

void filter_widget_denoise_temporal_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    this->parameterArray[OFFS_THRESHOLD] = 5;

    return;
}

void filter_widget_denoise_temporal_s::create_widget(void)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Denoising threshold.
    QLabel *thresholdLabel = new QLabel("Threshold:", frame);
    QSpinBox *thresholdSpin = new QSpinBox(frame);
    thresholdSpin->setRange(0, 255);
    thresholdSpin->setValue(this->parameterArray[OFFS_THRESHOLD]);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(thresholdLabel, thresholdSpin);

    connect(thresholdSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_THRESHOLD] = newValue;
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

void filter_widget_denoise_nonlocal_means_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    this->parameterArray[OFFS_H] = 10;
    this->parameterArray[OFFS_H_COLOR] = 10;
    this->parameterArray[OFFS_TEMPLATE_WINDOW_SIZE] = 7;
    this->parameterArray[OFFS_SEARCH_WINDOW_SIZE] = 21;

    return;
}

void filter_widget_denoise_nonlocal_means_s::create_widget(void)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    QLabel *hLabel = new QLabel("Luminance:", frame);
    QLabel *hColorLabel = new QLabel("Color:", frame);
    QLabel *templateWindowLabel = new QLabel("Template wnd.:", frame);
    QLabel *searchWindowLabel = new QLabel("Search wnd.:", frame);

    QSpinBox *hSpin = new QSpinBox(frame);
    hSpin->setRange(0, 255);
    hSpin->setValue(this->parameterArray[OFFS_H]);

    QSpinBox *hColorSpin = new QSpinBox(frame);
    hColorSpin->setRange(0, 255);
    hColorSpin->setValue(this->parameterArray[OFFS_H_COLOR]);

    QSpinBox *templateWindowSpin = new QSpinBox(frame);
    templateWindowSpin->setRange(0, 255);
    templateWindowSpin->setValue(this->parameterArray[OFFS_TEMPLATE_WINDOW_SIZE]);

    QSpinBox *searchWindowSpin = new QSpinBox(frame);
    searchWindowSpin->setRange(0, 255);
    searchWindowSpin->setValue(this->parameterArray[OFFS_SEARCH_WINDOW_SIZE]);

    QFormLayout *layout = new QFormLayout(frame);
    layout->addRow(hColorLabel, hColorSpin);
    layout->addRow(hLabel, hSpin);
    layout->addRow(searchWindowLabel, searchWindowSpin);
    layout->addRow(templateWindowLabel, templateWindowSpin);

    connect(hSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_H] = newValue;
    });

    connect(hColorSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_H_COLOR] = newValue;
    });

    connect(templateWindowSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_TEMPLATE_WINDOW_SIZE] = newValue;
    });

    connect(searchWindowSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_SEARCH_WINDOW_SIZE] = newValue;
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

void filter_widget_sharpen_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    return;
}

void filter_widget_sharpen_s::create_widget(void)
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

void filter_widget_unsharp_mask_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    this->parameterArray[OFFS_STRENGTH] = 50;
    this->parameterArray[OFFS_RADIUS] = 10;

    return;
}

void filter_widget_unsharp_mask_s::create_widget(void)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Strength.
    QLabel *strLabel = new QLabel("Strength:", frame);
    QSpinBox *strSpin = new QSpinBox(frame);
    strSpin->setRange(0, 255);
    strSpin->setValue(this->parameterArray[OFFS_STRENGTH]);

    // Radius.
    QLabel *radiusLabel = new QLabel("Radius:", frame);
    QSpinBox *radiusdSpin = new QSpinBox(frame);
    radiusdSpin->setRange(0, 255);
    radiusdSpin->setValue(this->parameterArray[OFFS_RADIUS] / 10);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(strLabel, strSpin);
    l->addRow(radiusLabel, radiusdSpin);

    connect(strSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_STRENGTH] = newValue;
    });

    connect(radiusdSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_RADIUS] = (newValue * 10);
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

void filter_widget_decimate_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    this->parameterArray[OFFS_FACTOR] = 2;
    this->parameterArray[OFFS_TYPE] = FILTER_TYPE_AVERAGE;

    return;
}

void filter_widget_decimate_s::create_widget(void)
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Factor.
    QLabel *factorLabel = new QLabel("Factor:", frame);
    QComboBox *factorList = new QComboBox(frame);
    factorList->addItem("2");
    factorList->addItem("4");
    factorList->addItem("8");
    factorList->addItem("16");
    factorList->setCurrentIndex((round(sqrt(this->parameterArray[OFFS_FACTOR])) - 1));

    // Sampling.
    QLabel *radiusLabel = new QLabel("Sampling:", frame);
    QComboBox *samplingList = new QComboBox(frame);
    samplingList->addItem("Nearest");
    samplingList->addItem("Average");
    samplingList->setCurrentIndex(this->parameterArray[OFFS_TYPE]);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(factorLabel, factorList);
    l->addRow(radiusLabel, samplingList);

    connect(factorList, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), [this](const QString &newIndexText)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_FACTOR] = newIndexText.toUInt();
    });

    connect(samplingList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_TYPE] = newIndex;
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}

void filter_widget_delta_histogram_s::reset_parameter_data(void)
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    return;
}

void filter_widget_delta_histogram_s::create_widget(void)
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

void filter_widget_unique_count_s::reset_parameter_data()
{
    k_assert(this->parameterArray, "Expected non-null pointer to filter data.");

    memset(this->parameterArray, 0, sizeof(u8) * FILTER_PARAMETER_ARRAY_LENGTH);

    this->parameterArray[OFFS_THRESHOLD] = 20;
    this->parameterArray[OFFS_CORNER] = 0;

    return;
}

void filter_widget_unique_count_s::create_widget()
{
    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);

    // Denoising threshold.
    QLabel *thresholdLabel = new QLabel("Threshold:", frame);
    QSpinBox *thresholdSpin = new QSpinBox(frame);
    thresholdSpin->setRange(0, 255);
    thresholdSpin->setValue(this->parameterArray[OFFS_THRESHOLD]);

    // In which corner to show the counter.
    QLabel *cornerLabel = new QLabel("Show in:", frame);
    QComboBox *cornerList = new QComboBox(frame);
    cornerList->addItem("Top left");
    cornerList->addItem("Top right");
    cornerList->addItem("Bottom right");
    cornerList->addItem("Bottom left");
    cornerList->setCurrentIndex(this->parameterArray[OFFS_CORNER]);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(cornerLabel, cornerList);
    l->addRow(thresholdLabel, thresholdSpin);

    connect(thresholdSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_THRESHOLD] = newValue;
    });

    connect(cornerList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int newIndex)
    {
        k_assert(this->parameterArray, "Expected non-null filter data.");
        this->parameterArray[OFFS_CORNER] = newIndex;
    });

    frame->adjustSize();
    this->widget = frame;

    return;
}
