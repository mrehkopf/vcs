/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QWidget>
#include <QFrame>
#include <QLabel>
#include "display/qt/subclasses/QFrame_filtergui_for_qt.h"
#include "filter/filter.h"

FilterGUIForQt::FilterGUIForQt(const filter_c *const filter,
                               QWidget *parent) :
    QFrame(parent)
{
    const auto &guiDescription = filter->gui_description();
    auto *const widgetLayout = new QFormLayout(this);

    if (guiDescription.empty())
    {
        auto *const emptyLabel = new QLabel("No parameters", this);

        emptyLabel->setStyleSheet("font-style: italic;");
        emptyLabel->setAlignment(Qt::AlignCenter);

        widgetLayout->addWidget(emptyLabel);
    }
    else
    {
        for (const auto &field: guiDescription)
        {
            auto *const rowContainer = new QFrame(this);
            auto *const containerLayout = new QHBoxLayout(rowContainer);

            containerLayout->setContentsMargins(0, 0, 0, 0);
            rowContainer->setFrameShape(QFrame::Shape::NoFrame);

            for (auto *component: field.components)
            {
                switch (component->type())
                {
                    case filtergui_component_e::label:
                    {
                        auto *const c = ((filtergui_label_s*)component);
                        auto *const label = new QLabel(QString::fromStdString(c->text), this);

                        label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
                        label->setAlignment(Qt::AlignCenter);
                        label->setStyleSheet("margin-bottom: .25em;");

                        containerLayout->addWidget(label);

                        break;
                    }
                    case filtergui_component_e::combobox:
                    {
                        auto *const c = ((filtergui_combobox_s*)component);
                        auto *const combobox = new QComboBox(this);

                        for (const auto &item: c->items)
                        {
                            combobox->addItem(QString::fromStdString(item));
                        }

                        combobox->setCurrentIndex(c->get_value());

                        connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](const int currentIdx)
                        {
                            c->set_value(currentIdx);
                            emit this->parameter_changed();
                        });

                        containerLayout->addWidget(combobox);

                        break;
                    }
                    case filtergui_component_e::spinbox:
                    {
                        auto *const c = ((filtergui_spinbox_s*)component);
                        auto *const spinbox = new QSpinBox(this);

                        spinbox->setRange(c->minValue, c->maxValue);
                        spinbox->setValue(c->get_value());

                        connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), [=](const double newValue)
                        {
                            c->set_value(newValue);
                            emit this->parameter_changed();
                        });

                        containerLayout->addWidget(spinbox);

                        break;
                    }
                    case filtergui_component_e::doublespinbox:
                    {
                        auto *const c = ((filtergui_doublespinbox_s*)component);
                        auto *const doublespinbox = new QDoubleSpinBox(this);

                        doublespinbox->setRange(c->minValue, c->maxValue);
                        doublespinbox->setDecimals(c->numDecimals);
                        doublespinbox->setValue(c->get_value());
                        doublespinbox->setSingleStep(c->stepSize);

                        connect(doublespinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](const double newValue)
                        {
                            c->set_value(newValue);
                            emit this->parameter_changed();
                        });

                        containerLayout->addWidget(doublespinbox);

                        break;
                    }
                    default: k_assert(0, "Unrecognized filter GUI component."); break;
                }
            }

            auto *const label = (field.label.empty()? nullptr : new QLabel(QString::fromStdString(field.label), this));

            widgetLayout->addRow(label, rowContainer);
        }
    }

    this->setMinimumWidth(200);
    this->adjustSize();

    return;
}
