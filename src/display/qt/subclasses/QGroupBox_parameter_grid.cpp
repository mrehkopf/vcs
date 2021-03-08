/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <QGridLayout>
#include <QPushButton>
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

void ParameterGrid::add_parameter(const QString name,
                                  const int valueInitial,
                                  const int valueMin,
                                  const int valueMax)
{
    auto *const newParam = new ParameterGrid::parameter_meta;
    newParam->name = name;
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

        auto resetButtonIcon = QIcon(":/res/images/icons/reset.png");
        resetButtonIcon.addPixmap(QPixmap(":/res/images/icons/reset_disabled.png"), QIcon::Disabled);

        auto *const resetButton = new QPushButton();
        resetButton->setIcon(resetButtonIcon);
        resetButton->setFixedWidth(22);
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

ParameterGrid::parameter_meta* ParameterGrid::parameter(const QString &parameterName) const
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
    this->parameter(parameterName)->guiSpinBox->setValue(newValue);

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

    parameter->maximumValue = newMax;
    parameter->guiSpinBox->setMaximum(newMax);
    parameter->guiScrollBar->setMaximum(newMax);

    return;
}

void ParameterGrid::set_minimum_value(const QString &parameterName, const int newMin)
{
    auto *parameter = this->parameter(parameterName);

    parameter->minimumValue = newMin;
    parameter->guiSpinBox->setMinimum(newMin);
    parameter->guiScrollBar->setMinimum(newMin);

    return;
}
