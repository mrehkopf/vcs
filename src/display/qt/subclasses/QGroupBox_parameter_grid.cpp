/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QGridLayout>
#include <QPushButton>
#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <QScrollBar>
#include "display/qt/subclasses/QGroupBox_parameter_grid.h"
#include "common/globals.h"

ParameterGrid::ParameterGrid(QWidget *parent) : QGroupBox(parent)
{
    auto *const layout = new QGridLayout();
    layout->setContentsMargins(0, 9, 9, 9);

    this->setLayout(layout);

    return;
}

ParameterGrid::~ParameterGrid()
{
    for (const auto *parameter: this->parameters)
    {
        delete parameter;
    }

    return;
}

void ParameterGrid::add_spacer(void)
{
    auto *const line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    auto *layout = qobject_cast<QGridLayout*>(this->layout());
    const auto rowIdx = layout->rowCount();
    layout->addWidget(line, rowIdx, 0, 1, 0);

    return;
}

void ParameterGrid::add_combobox(const QString name,
                                 const std::list<QString> items)
{
    auto *const newParam = new ParameterGrid::parameter_meta_s;
    newParam->name = name;
    newParam->type = ParameterGrid::parameter_type_e::combobox;
    newParam->currentValue = 0;
    newParam->defaultValue = 0;
    newParam->minimumValue = 0;
    newParam->maximumValue = 0;

    // Set up the UI.
    {
        auto *const label = new QLabel(name);
        label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        label->setMinimumWidth(100);

        auto *const comboBox = new QComboBox();
        comboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        for (const auto &item: items)
        {
            comboBox->addItem(item);
        }

        auto resetButtonIcon = QIcon(":/res/images/icons/newie/reset.png");
        resetButtonIcon.addPixmap(QPixmap(":/res/images/icons/newie/reset_disabled.png"), QIcon::Disabled);

        auto *const resetButton = new QPushButton();
        resetButton->setIcon(resetButtonIcon);
        resetButton->setFixedWidth(24);
        resetButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        resetButton->setEnabled(false);
        resetButton->setToolTip("Reset to default");

        // Insert the components.
        {
            auto *layout = qobject_cast<QGridLayout*>(this->layout());
            const auto rowIdx = layout->rowCount();

            layout->addWidget(label, rowIdx, 0);
            layout->addWidget(comboBox, rowIdx, 1, 1, 2);
            layout->addWidget(resetButton, rowIdx, 3);

            newParam->guiComboBox = comboBox;
            newParam->guiResetButton = resetButton;
        }

        // Create component reactivity.
        {
            connect(resetButton, &QPushButton::clicked, this, [=]
            {
                comboBox->setCurrentIndex(newParam->defaultValue);
                resetButton->setEnabled(false);
            });

            connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](const int newIndex)
            {
                newParam->currentValue = newIndex;
                resetButton->setEnabled((newIndex != newParam->defaultValue));

                emit this->parameter_value_changed(newParam->name);
            });
        }
    }

    this->parameters.push_back(newParam);

    return;
}

void ParameterGrid::add_parameter(const QString name,
                                  const int valueInitial,
                                  const int valueMin,
                                  const int valueMax)
{
    auto *const newParam = new ParameterGrid::parameter_meta_s;
    newParam->name = name;
    newParam->type = ParameterGrid::parameter_type_e::scroller;
    newParam->currentValue = valueInitial;
    newParam->defaultValue = valueInitial;
    newParam->minimumValue = valueMin;
    newParam->maximumValue = valueMax;

    // Set up the UI.
    {
        auto *const label = new QLabel(name);
        label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        label->setMinimumWidth(100);

        auto *const spinBox = new QSpinBox();
        spinBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        spinBox->setMaximum(newParam->maximumValue);
        spinBox->setMinimum(newParam->minimumValue);
        spinBox->setValue(newParam->currentValue);
        spinBox->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);

        auto resetButtonIcon = QIcon(":/res/images/icons/newie/reset.png");
        resetButtonIcon.addPixmap(QPixmap(":/res/images/icons/newie/reset_disabled.png"), QIcon::Disabled);

        auto *const resetButton = new QPushButton();
        resetButton->setIcon(resetButtonIcon);
        resetButton->setFixedWidth(24);
        resetButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        resetButton->setEnabled(false);
        resetButton->setToolTip("Reset to default");

        auto *const scrollBar = new QScrollBar();
        scrollBar->setOrientation(Qt::Orientation::Horizontal);
        scrollBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        scrollBar->setMaximum(newParam->maximumValue);
        scrollBar->setMinimum(newParam->minimumValue);
        scrollBar->setValue(newParam->currentValue);

        // Insert the components.
        {
            auto *layout = qobject_cast<QGridLayout*>(this->layout());
            const auto rowIdx = layout->rowCount();

            layout->addWidget(label, rowIdx, 0);
            layout->addWidget(spinBox, rowIdx, 1);
            layout->addWidget(scrollBar, rowIdx, 2);
            layout->addWidget(resetButton, rowIdx, 3);

            newParam->guiSpinBox = spinBox;
            newParam->guiScrollBar = scrollBar;
            newParam->guiResetButton = resetButton;
        }

        // Create component reactivity.
        {
            connect(resetButton, &QPushButton::clicked, this, [=]
            {
                spinBox->setValue(newParam->defaultValue);
            });

            connect(scrollBar, QOverload<int>::of(&QScrollBar::valueChanged), this, [=](const int newValue)
            {
                spinBox->setValue(newValue);
            });

            connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](const int newValue)
            {
                newParam->currentValue = newValue;

                scrollBar->setValue(newValue);
                resetButton->setEnabled((newValue != newParam->defaultValue));

                emit this->parameter_value_changed(newParam->name);
            });
        }
    }

    this->parameters.push_back(newParam);

    return;
}

ParameterGrid::parameter_meta_s* ParameterGrid::parameter(const QString &parameterName) const
{
    for (auto *parameter: this->parameters)
    {
        if (parameter->name == parameterName)
        {
            return parameter;
        }
    }

    k_assert(0, "Unknown parameter.");

    return this->parameters.at(0);
}


int ParameterGrid::value(const QString &parameterName) const
{
    return this->parameter(parameterName)->currentValue;
}

int ParameterGrid::default_value(const QString &parameterName) const
{
    return this->parameter(parameterName)->defaultValue;
}

int ParameterGrid::maximum_value(const QString &parameterName) const
{
    return this->parameter(parameterName)->maximumValue;
}

int ParameterGrid::minimum_value(const QString &parameterName) const
{
    return this->parameter(parameterName)->minimumValue;
}

void ParameterGrid::set_value(const QString &parameterName, const int newValue)
{
    auto *const parameter = this->parameter(parameterName);

    k_assert((parameter->type != ParameterGrid::parameter_type_e::unknown),
             "Detected a parameter with no known type.");

    switch (parameter->type)
    {
        case ParameterGrid::parameter_type_e::scroller:
        {
            this->parameter(parameterName)->guiSpinBox->setValue(newValue);
            break;
        }
        case ParameterGrid::parameter_type_e::combobox:
        {
            parameter->currentValue = newValue;
            emit this->parameter_value_changed(parameterName);
            break;
        }
        default: k_assert(0, "Unknown parameter type."); break;
    }

    return;
}

void ParameterGrid::set_default_value(const QString &parameterName, const int newDefault)
{
    auto *parameter = this->parameter(parameterName);

    parameter->defaultValue = newDefault;
    parameter->guiResetButton->setEnabled((parameter->currentValue != newDefault));

    return;
}

void ParameterGrid::set_maximum_value(const QString &parameterName, const int newMax)
{
    auto *parameter = this->parameter(parameterName);

    k_assert((parameter->type == ParameterGrid::parameter_type_e::scroller),
             "Only scroller type parameter can have a maximum value.");

    parameter->maximumValue = newMax;
    parameter->guiSpinBox->setMaximum(newMax);
    parameter->guiScrollBar->setMaximum(newMax);

    return;
}

void ParameterGrid::set_minimum_value(const QString &parameterName, const int newMin)
{
    auto *parameter = this->parameter(parameterName);

    k_assert((parameter->type == ParameterGrid::parameter_type_e::scroller),
             "Only scroller type parameter can have a minimum value.");

    parameter->minimumValue = newMin;
    parameter->guiSpinBox->setMinimum(newMin);
    parameter->guiScrollBar->setMinimum(newMin);

    return;
}
