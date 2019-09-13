#include <QDebug>
#include "display/qt/widgets/filter_widgets.h"

filter_widget_s::filter_widget_s(u8 *const filterData, const char *const displayName, const unsigned minWidth) :
    title(displayName),
    parameterData(filterData),
    minWidth(minWidth)
{
    return;
}

filter_widget_s::~filter_widget_s()
{
    return;
}

void filter_widget_blur_s::reset_parameter_data()
{
    k_assert(this->parameterData, "Expected non-null pointer to filter data.");

    memset(this->parameterData, 0, sizeof(u8) * FILTER_DATA_LENGTH);

    this->parameterData[OFFS_KERNEL_SIZE] = 10;
    this->parameterData[OFFS_TYPE] = FILTER_TYPE_GAUSSIAN;

    return;
}

void filter_widget_blur_s::create_widget()
{
    this->reset_parameter_data();

    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);
    frame->setStyleSheet("QFrame{background-color: transparent;}");

    // Blur type.
    QLabel *typeLabel = new QLabel("Type:", frame);
    QComboBox *typeList = new QComboBox(frame);
    typeList->addItem("Box");
    typeList->addItem("Gaussian");
    typeList->setCurrentIndex(this->parameterData[OFFS_TYPE]);

    // Blur radius.
    QLabel *radiusLabel = new QLabel("Radius:", frame);
    QDoubleSpinBox *radiusSpin = new QDoubleSpinBox(frame);
    radiusSpin->setRange(0, 25);
    radiusSpin->setDecimals(1);
    radiusSpin->setValue(this->parameterData[OFFS_KERNEL_SIZE] / 10.0);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(typeLabel, typeList);
    l->addRow(radiusLabel, radiusSpin);

    QObject::connect(radiusSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        k_assert(this->parameterData, "Expected non-null filter data.");
        this->parameterData[OFFS_KERNEL_SIZE] = round(newValue * 10.0);
    });

    QObject::connect(typeList, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](const int currentIdx)
    {
        k_assert(this->parameterData, "Expected non-null filter data.");
        this->parameterData[OFFS_TYPE] = ((currentIdx == -1)? 0 : currentIdx);
    });

    this->widget = frame;

    return;
}

void filter_widget_rotate_s::reset_parameter_data()
{
    k_assert(this->parameterData, "Expected non-null pointer to filter data.");

    memset(this->parameterData, 0, sizeof(u8) * FILTER_DATA_LENGTH);

    // The scale value gets divided by 100 when used.
    *(i16*)&this->parameterData[OFFS_SCALE] = 100;

    // The rotation value gets divided by 10 when used.
    *(i16*)&this->parameterData[OFFS_ROT] = 0;

    return;
}

void filter_widget_rotate_s::create_widget()
{
    this->reset_parameter_data();

    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);
    frame->setStyleSheet("QFrame{background-color: transparent;}");

    QLabel *rotLabel = new QLabel("Angle:", frame);
    QDoubleSpinBox *rotSpin = new QDoubleSpinBox(frame);
    rotSpin->setDecimals(1);
    rotSpin->setRange(-360, 360);
    rotSpin->setValue(*(i16*)&(this->parameterData[OFFS_ROT]) / 10.0);

    QLabel *scaleLabel = new QLabel("Scale:", frame);
    QDoubleSpinBox *scaleSpin = new QDoubleSpinBox(frame);
    scaleSpin->setDecimals(2);
    scaleSpin->setRange(0, 20);
    scaleSpin->setValue((*(i16*)&(this->parameterData[OFFS_SCALE])) / 100.0);

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(rotLabel, rotSpin);
    l->addRow(scaleLabel, scaleSpin);

    QObject::connect(rotSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        k_assert(this->parameterData, "Expected non-null filter data.");
         *(i16*)&this->parameterData[OFFS_ROT] = (newValue * 10);
    });

    QObject::connect(scaleSpin, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](const double newValue)
    {
        k_assert(this->parameterData, "Expected non-null filter data.");
        *(i16*)&this->parameterData[OFFS_SCALE] = (newValue * 100);
    });

    this->widget = frame;

    return;
}

void filter_widget_input_gate_s::reset_parameter_data()
{
    k_assert(this->parameterData, "Expected non-null pointer to filter data.");

    memset(this->parameterData, 0, sizeof(u8) * FILTER_DATA_LENGTH);

    *(u16*)&this->parameterData[OFFS_WIDTH] = 640;
    *(u16*)&this->parameterData[OFFS_HEIGHT] = 480;
}

void filter_widget_input_gate_s::create_widget()
{
    this->reset_parameter_data();

    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);
    frame->setStyleSheet("QFrame{background-color: transparent;}");

    QLabel *widthLabel = new QLabel("Width:", frame);
    QSpinBox *widthSpin = new QSpinBox(frame);
    widthSpin->setRange(0, u16(~0u));
    widthSpin->setValue(*(u16*)&(this->parameterData[OFFS_WIDTH]));

    QLabel *heightLabel = new QLabel("Height:", frame);
    QSpinBox *heightSpin = new QSpinBox(frame);
    heightSpin->setRange(0, u16(~0u));
    heightSpin->setValue(*(i16*)&(this->parameterData[OFFS_HEIGHT]));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(widthLabel, widthSpin);
    l->addRow(heightLabel, heightSpin);

    QObject::connect(widthSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterData, "Expected non-null filter data.");
         *(u16*)&this->parameterData[OFFS_WIDTH] = newValue;
    });

    QObject::connect(heightSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterData, "Expected non-null filter data.");
         *(u16*)&this->parameterData[OFFS_WIDTH] = newValue;
    });

    this->widget = frame;

    return;
}

void filter_widget_output_gate_s::reset_parameter_data()
{
    k_assert(this->parameterData, "Expected non-null pointer to filter data.");

    memset(this->parameterData, 0, sizeof(u8) * FILTER_DATA_LENGTH);

    *(u16*)&this->parameterData[OFFS_WIDTH] = 1920;
    *(u16*)&this->parameterData[OFFS_HEIGHT] = 1080;
}

void filter_widget_output_gate_s::create_widget()
{
    this->reset_parameter_data();

    QFrame *frame = new QFrame();
    frame->setMinimumWidth(this->minWidth);
    frame->setStyleSheet("QFrame{background-color: transparent;}");

    QLabel *widthLabel = new QLabel("Width:", frame);
    QSpinBox *widthSpin = new QSpinBox(frame);
    widthSpin->setRange(0, u16(~0u));
    widthSpin->setValue(*(u16*)&(this->parameterData[OFFS_WIDTH]));

    QLabel *heightLabel = new QLabel("Height:", frame);
    QSpinBox *heightSpin = new QSpinBox(frame);
    heightSpin->setRange(0, u16(~0u));
    heightSpin->setValue(*(i16*)&(this->parameterData[OFFS_HEIGHT]));

    QFormLayout *l = new QFormLayout(frame);
    l->addRow(widthLabel, widthSpin);
    l->addRow(heightLabel, heightSpin);

    QObject::connect(widthSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterData, "Expected non-null filter data.");
         *(u16*)&this->parameterData[OFFS_WIDTH] = newValue;
    });

    QObject::connect(heightSpin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this](const int newValue)
    {
        k_assert(this->parameterData, "Expected non-null filter data.");
         *(u16*)&this->parameterData[OFFS_WIDTH] = newValue;
    });

    this->widget = frame;

    return;
}
