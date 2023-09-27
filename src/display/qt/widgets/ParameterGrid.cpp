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

        auto resetButtonIcon = QIcon(":/res/icons/newie/reset.png");
        resetButtonIcon.addPixmap(QPixmap(":/res/icons/newie/reset_disabled.png"), QIcon::Disabled);

        auto *const resetButton = new QPushButton();
        resetButton->setIcon(resetButtonIcon);
        resetButton->setFixedWidth(24);
        resetButton->setEnabled(false);
        resetButton->setToolTip("Reset to default");

        // Insert the components.
        {
            auto *layout = qobject_cast<QGridLayout*>(this->layout());
            const auto rowIdx = layout->rowCount();

            layout->addWidget(label, rowIdx, 0);
            layout->addWidget(comboBox, rowIdx, 1, 1, 2);
            layout->addWidget(resetButton, rowIdx, 3);
        }

        // Create component reactivity.
        {
            connect(resetButton, &QPushButton::clicked, this, [=]
            {
                comboBox->setCurrentIndex(newParam->defaultValue);
                resetButton->setEnabled(false);
            });

            connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [newParam, resetButton, this](const int newIndex)
            {
                newParam->currentValue = newIndex;
                resetButton->setEnabled((newIndex != newParam->defaultValue));

                emit this->parameter_value_changed(newParam->name, newIndex);
                emit this->parameter_value_changed_by_user(newParam->name, newIndex);
            });

            connect(this, &ParameterGrid::parameter_value_changed_programmatically, this, [newParam, comboBox, resetButton, this](const QString &parameterName, const int newValue)
            {
                if (parameterName == newParam->name)
                {
                    const QSignalBlocker blocker(comboBox);

                    newParam->currentValue = newValue;
                    comboBox->setCurrentIndex(newValue);
                    resetButton->setEnabled((newValue != newParam->defaultValue));
                }

                emit this->parameter_value_changed(parameterName, newValue);
            });

            connect(this, &ParameterGrid::parameter_default_value_changed_programmatically, this, [=](const QString &parameterName, const int newDefault)
            {
                if (parameterName == newParam->name)
                {
                    newParam->defaultValue = newDefault;
                    resetButton->setEnabled((newParam->currentValue != newDefault));
                }
            });
        }
    }

    this->parameters.push_back(newParam);

    return;
}

void ParameterGrid::add_scroller(
    const QString name,
    const QString iconName,
    const int valueInitial,
    const int valueMin,
    const int valueMax
)
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
        auto *const icon = new QWidget();
        icon->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        icon->setObjectName("icon-" + iconName);

        auto *const label = new QLabel(name);
        label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

        auto *const spinBox = new QSpinBox();
        spinBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        spinBox->setMaximum(newParam->maximumValue);
        spinBox->setMinimum(newParam->minimumValue);
        spinBox->setValue(newParam->currentValue);

        auto resetButtonIcon = QIcon(":/res/icons/newie/reset.png");
        resetButtonIcon.addPixmap(QPixmap(":/res/icons/newie/reset_disabled.png"), QIcon::Disabled);

        auto *const resetButton = new QPushButton();
        resetButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        resetButton->setIcon(resetButtonIcon);
        resetButton->setEnabled(false);
        resetButton->setToolTip("Reset to default");

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
            layout->addWidget(icon, rowIdx, column++);
            layout->addWidget(label, rowIdx, column++);
            layout->addWidget(spinBox, rowIdx, column++);
            layout->addWidget(scrollBar, rowIdx, column++);
            layout->addWidget(resetButton, rowIdx, column++);
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

            connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [=, this](const int newValue)
            {
                newParam->currentValue = newValue;
                scrollBar->setValue(newValue);
                resetButton->setEnabled((newValue != newParam->defaultValue));

                emit this->parameter_value_changed(newParam->name, newValue);
                emit this->parameter_value_changed_by_user(newParam->name, newValue);
            });

            connect(this, &ParameterGrid::parameter_value_changed_programmatically, this, [=, this](const QString &parameterName, const int newValue)
            {
                if (parameterName == newParam->name)
                {
                    const QSignalBlocker blockSpin(spinBox);
                    const QSignalBlocker blockScroll(scrollBar);

                    newParam->currentValue = newValue;
                    spinBox->setValue(newValue);
                    scrollBar->setValue(newValue);
                    resetButton->setEnabled((newValue != newParam->defaultValue));
                }

                emit this->parameter_value_changed(newParam->name, newValue);
            });

            connect(this, &ParameterGrid::parameter_default_value_changed_programmatically, this, [=](const QString &parameterName, const int newDefault)
            {
                if (parameterName == newParam->name)
                {
                    newParam->defaultValue = newDefault;
                    resetButton->setEnabled((newParam->currentValue != newDefault));
                }
            });

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
            });

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
    emit this->parameter_value_changed_programmatically(parameterName, newValue);

    return;
}

void ParameterGrid::set_default_value(const QString &parameterName, const int newDefault)
{
    emit this->parameter_default_value_changed_programmatically(parameterName, newDefault);

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
