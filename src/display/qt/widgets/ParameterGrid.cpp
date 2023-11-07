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
#include "display/qt/widgets/HorizontalSlider.h"
#include "display/qt/widgets/ParameterGrid.h"
#include "common/assert.h"

QList<QWidget*> CREATED_WIDGETS;

#define ADD_TO_LAYOUT(layout, widget, ...) \
    layout->addWidget(widget, __VA_ARGS__);\
    CREATED_WIDGETS.push_back(widget);

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

void ParameterGrid::add_separator(void)
{
    auto *const line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    auto *layout = qobject_cast<QGridLayout*>(this->layout());
    const auto rowIdx = layout->rowCount();
    layout->addWidget(line, rowIdx, 0, 1, 0);

    return;
}

void ParameterGrid::add_combobox(const QString name, const std::list<QString> items)
{
    auto *const newParam = new ParameterGrid::parameter_meta_s;
    newParam->name = name;
    newParam->type = ParameterGrid::parameter_type_e::combobox;
    newParam->currentValue = 0;
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

        // Insert the components.
        {
            auto *layout = qobject_cast<QGridLayout*>(this->layout());
            const auto rowIdx = layout->rowCount();

            ADD_TO_LAYOUT(layout, label, rowIdx, 0);
            ADD_TO_LAYOUT(layout, comboBox, rowIdx, 1, 1, 2);
        }

        // Create component reactivity.
        newParam->guiConnections = {
            connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [newParam, this](const int newIndex)
            {
                newParam->currentValue = newIndex;

                emit this->parameter_value_changed(newParam->name, newIndex);
                emit this->parameter_value_changed_by_user(newParam->name, newIndex);
            }),
            connect(this, &ParameterGrid::parameter_value_changed_programmatically, this, [newParam, comboBox, this](const QString &parameterName, const int newValue)
            {
                if (parameterName == newParam->name)
                {
                    const QSignalBlocker blocker(comboBox);

                    newParam->currentValue = newValue;
                    comboBox->setCurrentIndex(newValue);
                }

                emit this->parameter_value_changed(parameterName, newValue);
            }),
        };
    }

    this->parameters.push_back(newParam);

    return;
}

void ParameterGrid::clear(void)
{
    while (auto *item = this->layout()->takeAt(0))
    {
        this->layout()->removeItem(item);
        delete item;
    }

    for (auto *widget: CREATED_WIDGETS)
    {
        widget->blockSignals(true);
        widget->disconnect();
        delete widget;
    }
    CREATED_WIDGETS.clear();

    for (const auto *parameter: this->parameters)
    {
        for (const auto connection: parameter->guiConnections)
        {
            QObject::disconnect(connection);
        }

        delete parameter;
    }
    this->parameters.clear();

    return;
}

void ParameterGrid::add_scroller(
    const QString name,
    const int valueInitial,
    const int valueMin,
    const int valueMax
)
{
    auto *const newParam = new ParameterGrid::parameter_meta_s;
    newParam->name = name;
    newParam->type = ParameterGrid::parameter_type_e::scroller;
    newParam->currentValue = valueInitial;
    newParam->minimumValue = valueMin;
    newParam->maximumValue = valueMax;

    // Set up the UI.
    {
        auto *const label = new QLabel(name);
        label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

        auto *const spinBox = new QSpinBox();
        spinBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        spinBox->setMaximum(newParam->maximumValue);
        spinBox->setMinimum(newParam->minimumValue);
        spinBox->setValue(newParam->currentValue);

        auto *const scrollBar = new HorizontalSlider();
        scrollBar->setOrientation(Qt::Orientation::Horizontal);
        scrollBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        scrollBar->setMaximum(newParam->maximumValue);
        scrollBar->setMinimum(newParam->minimumValue);
        scrollBar->setValue(newParam->currentValue);

        // Insert the components.
        {
            auto *layout = qobject_cast<QGridLayout*>(this->layout());
            layout->setSpacing(6);

            const auto rowIdx = layout->rowCount();

            unsigned column = 0;
            ADD_TO_LAYOUT(layout, label, rowIdx, column++);
            ADD_TO_LAYOUT(layout, spinBox, rowIdx, column++);
            ADD_TO_LAYOUT(layout, scrollBar, rowIdx, column++);
        }

        // Create component reactivity.
        newParam->guiConnections = {
            connect(scrollBar, QOverload<int>::of(&QScrollBar::valueChanged), this, [=](const int newValue)
            {
                spinBox->setValue(newValue);
            }),
            connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=, this](const int newValue)
            {
                newParam->currentValue = newValue;
                scrollBar->setValue(newValue);

                emit this->parameter_value_changed(newParam->name, newValue);
                emit this->parameter_value_changed_by_user(newParam->name, newValue);
            }),
            connect(this, &ParameterGrid::parameter_value_changed_programmatically, this, [=, this](const QString &parameterName, const int newValue)
            {
                if (parameterName == newParam->name)
                {
                    const QSignalBlocker blockSpin(spinBox);
                    const QSignalBlocker blockScroll(scrollBar);

                    newParam->currentValue = newValue;
                    spinBox->setValue(newValue);
                    scrollBar->setValue(newValue);
                }

                emit this->parameter_value_changed(newParam->name, newValue);
            }),
            connect(this, &ParameterGrid::parameter_minimum_value_changed_programmatically, this, [=](const QString &parameterName, const int newMin)
            {
                if (parameterName == newParam->name)
                {
                    const QSignalBlocker blockSpin(spinBox);
                    const QSignalBlocker blockScroll(scrollBar);

                    newParam->minimumValue = newMin;
                    spinBox->setMinimum(newMin);
                    scrollBar->setMinimum(newMin);
                }
            }),
            connect(this, &ParameterGrid::parameter_maximum_value_changed_programmatically, this, [=](const QString &parameterName, const int newMax)
            {
                if (parameterName == newParam->name)
                {
                    const QSignalBlocker blockSpin(spinBox);
                    const QSignalBlocker blockScroll(scrollBar);

                    newParam->minimumValue = newMax;
                    spinBox->setMaximum(newMax);
                    scrollBar->setMaximum(newMax);
                }
            })
        };
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
    emit this->parameter_value_changed_programmatically(parameterName, newValue);

    return;
}

void ParameterGrid::set_maximum_value(const QString &parameterName, const int newMax)
{
    auto *parameter = this->parameter(parameterName);

    k_assert((parameter->type == ParameterGrid::parameter_type_e::scroller),
             "Only scroller type parameter can have a maximum value.");

    emit this->parameter_maximum_value_changed_programmatically(parameterName, newMax);

    return;
}

void ParameterGrid::set_minimum_value(const QString &parameterName, const int newMin)
{
    auto *parameter = this->parameter(parameterName);

    k_assert((parameter->type == ParameterGrid::parameter_type_e::scroller),
             "Only scroller type parameter can have a minimum value.");

    emit this->parameter_minimum_value_changed_programmatically(parameterName, newMin);

    return;
}
