/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <QDoubleSpinBox>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QWidget>
#include <QFrame>
#include <QLabel>
#include "display/qt/widgets/QtAbstractGUI.h"
#include "filter/filter.h"
#include "filter/abstract_filter.h"
#include "common/assert.h"

QtAbstractGUI::QtAbstractGUI(const abstract_gui_s &gui) : QFrame()
{
    // For referring to this type of widget in QSS.
    this->setObjectName("QtAbstractGUI");

    QLayout *widgetLayout;

    switch (gui.layout)
    {
        default:
        case abstract_gui_s::layout_e::form:
        {
            widgetLayout = new QFormLayout(this);
            static_cast<QFormLayout*>(widgetLayout)->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
            static_cast<QFormLayout*>(widgetLayout)->setRowWrapPolicy(QFormLayout::DontWrapRows);
            break;
        }
        case abstract_gui_s::layout_e::vertical_box:
        {
            widgetLayout = new QVBoxLayout(this);
            break;
        }
    }

    /// TODO: This is leftover code from when QtAbstractGUI only handled filter graph
    /// GUIs. Ideally, instead of creating this "No parameters" message here, each
    /// filter inserts the message as needed on their end.
    if (gui.fields.empty())
    {
        auto *const emptyLabel = new QLabel("No parameters", this);
        emptyLabel->setStyleSheet("font-style: italic;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        widgetLayout->addWidget(emptyLabel);
    }
    else
    {
        for (const auto &field: gui.fields)
        {
            auto *const rowContainer = new QFrame(this);
            auto *const containerLayout = new QHBoxLayout(rowContainer);

            containerLayout->setContentsMargins(0, 0, 0, 0);
            rowContainer->setFrameShape(QFrame::Shape::NoFrame);

            for (auto *abstractComponent: field.components)
            {
                QWidget *widget = nullptr;

                if (dynamic_cast<abstract_gui_widget::label*>(abstractComponent))
                {
                    auto *const c = ((abstract_gui_widget::label*)abstractComponent);
                    auto *const label = qobject_cast<QLabel*>(widget = new QLabel(QString::fromStdString(c->text), this));

                    label->setStyleSheet("margin: 0; padding: 0; min-height: 1.25em;");
                    label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
                    label->setAlignment(Qt::AlignLeft);

                    c->set_text = [=](const std::string &text)
                    {
                        label->setText(QString::fromStdString(text));
                        c->text = text;
                    };
                }
                else if (dynamic_cast<abstract_gui_widget::button*>(abstractComponent))
                {
                    auto *const c = ((abstract_gui_widget::button*)abstractComponent);
                    auto *const button = qobject_cast<QPushButton*>(widget = new QPushButton(QString::fromStdString(c->label), this));

                    button->setStyleSheet("padding: 0 0.75em;");
                    connect(button, &QPushButton::clicked, [=, this]{c->on_press();});
                }
                else if (dynamic_cast<abstract_gui_widget::horizontal_rule*>(abstractComponent))
                {
                    auto *const rule = qobject_cast<QFrame*>(widget = new QFrame(this));
                    rule->setFrameShape(QFrame::HLine);
                }
                else if (dynamic_cast<abstract_gui_widget::button_get_open_filename*>(abstractComponent))
                {
                    auto *const c = ((abstract_gui_widget::button_get_open_filename*)abstractComponent);
                    auto *const button = qobject_cast<QPushButton*>(widget = new QPushButton(QString::fromStdString(c->label), this));

                    if (!c->on_success)
                    {
                        return;
                    }

                    connect(button, &QPushButton::clicked, [=, this]
                    {
                        QString filename = QFileDialog::getOpenFileName(
                            this,
                            "Select a file containing the filter graph to be loaded",
                            "",
                            QString::fromStdString(c->filenameFilter)
                        );

                        if (!filename.isNull())
                        {
                            c->on_success(filename.toStdString());
                        }
                    });
                }
                else if (dynamic_cast<abstract_gui_widget::line_edit*>(abstractComponent))
                {
                    auto *const c = ((abstract_gui_widget::line_edit*)abstractComponent);
                    auto *const lineEdit = qobject_cast<QLineEdit*>(widget = new QLineEdit(QString::fromStdString(c->text), this));

                    c->set_text = [=](const std::string &text)
                    {
                        lineEdit->setText(QString::fromStdString(text));

                        c->text = text;
                        c->on_change(text);
                    };

                    connect(lineEdit, &QLineEdit::textChanged, [=, this]
                    {
                        c->on_change(lineEdit->text().toStdString());
                    });
                }
                else if (dynamic_cast<abstract_gui_widget::text_edit*>(abstractComponent))
                {
                    auto *const c = ((abstract_gui_widget::text_edit*)abstractComponent);
                    auto *const textEdit = qobject_cast<QPlainTextEdit*>(widget = new QPlainTextEdit(QString::fromStdString(c->text), this));

                    textEdit->setMinimumWidth(150);
                    textEdit->setMaximumHeight(70);
                    textEdit->setTabChangesFocus(true);

                    c->set_text = [=](const std::string &text)
                    {
                        textEdit->setPlainText(QString::fromStdString(text));

                        c->text = text;
                        c->on_change(text);
                    };

                    connect(textEdit, &QPlainTextEdit::textChanged, [=, this]
                    {
                        QString text = textEdit->toPlainText();

                        textEdit->setProperty("overLengthLimit", (std::size_t(text.length()) > c->maxLength)? "true" : "false");
                        this->style()->polish(textEdit);

                        text.resize(std::min(std::size_t(text.length()), c->maxLength));
                        c->on_change(text.toStdString());

                        emit this->mutated(textEdit);
                    });
                }
                else if (dynamic_cast<abstract_gui_widget::checkbox*>(abstractComponent))
                {
                    auto *const c = ((abstract_gui_widget::checkbox*)abstractComponent);
                    auto *const checkbox = qobject_cast<QCheckBox*>(widget = new QCheckBox(this));

                    checkbox->setChecked(c->state);
                    checkbox->setText(QString::fromStdString(c->label));
                    checkbox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

                    connect(checkbox, &QCheckBox::toggled, [=, this](const bool isChecked)
                    {
                        c->on_change(isChecked);
                        c->state = isChecked;
                        emit this->mutated(checkbox);
                    });
                }
                else if (dynamic_cast<abstract_gui_widget::combo_box*>(abstractComponent))
                {
                    auto *const c = ((abstract_gui_widget::combo_box*)abstractComponent);
                    auto *const combobox = qobject_cast<QComboBox*>(widget = new QComboBox(this));

                    for (const auto &item: c->items)
                    {
                        combobox->addItem(QString::fromStdString(item));
                    }

                    combobox->setCurrentIndex(c->index);

                    c->set_index = [=](const int newIdx)
                    {
                        c->on_change(newIdx);
                        combobox->setCurrentIndex(newIdx);
                    };

                    c->set_items = [=](const std::vector<std::string> &items)
                    {
                        combobox->clear();
                        for (const auto &item: items)
                        {
                            combobox->addItem(QString::fromStdString(item));
                        }
                    };

                    connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=, this](const int newIdx)
                    {
                        c->on_change(newIdx);
                        c->index = newIdx;
                        emit this->mutated(combobox);
                    });
                }
                else if (dynamic_cast<abstract_gui_widget::spinner*>(abstractComponent))
                {
                    auto *const c = ((abstract_gui_widget::spinner*)abstractComponent);
                    auto *const spinbox = qobject_cast<QSpinBox*>(widget = new QSpinBox(this));

                    spinbox->setRange(c->minValue, c->maxValue);
                    spinbox->setValue(c->value);
                    spinbox->setPrefix(QString::fromStdString(c->prefix));
                    spinbox->setSuffix(QString::fromStdString(c->suffix));
                    spinbox->setButtonSymbols(
                        (c->alignment == abstract_gui_widget::horizontal_align::left)
                        ? QAbstractSpinBox::UpDownArrows
                        : QAbstractSpinBox::NoButtons
                    );

                    switch (c->alignment)
                    {
                        case abstract_gui_widget::horizontal_align::left: spinbox->setAlignment(Qt::AlignLeft); break;
                        case abstract_gui_widget::horizontal_align::right: spinbox->setAlignment(Qt::AlignRight); break;
                        case abstract_gui_widget::horizontal_align::center: spinbox->setAlignment(Qt::AlignHCenter); break;
                        default: k_assert(0, "Unrecognized filter GUI alignment enumerator."); break;
                    }

                    connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), [=, this](const double newValue)
                    {
                        c->on_change(newValue);
                        c->value = newValue;
                        emit this->mutated(spinbox);
                    });
                }
                else if (dynamic_cast<abstract_gui_widget::double_spinner*>(abstractComponent))
                {
                    auto *const c = ((abstract_gui_widget::double_spinner*)abstractComponent);
                    auto *const doublespinbox = qobject_cast<QDoubleSpinBox*>(widget = new QDoubleSpinBox(this));

                    doublespinbox->setRange(c->minValue, c->maxValue);
                    doublespinbox->setDecimals(c->numDecimals);
                    doublespinbox->setValue(c->value);
                    doublespinbox->setSingleStep(c->stepSize);
                    doublespinbox->setPrefix(QString::fromStdString(c->prefix));
                    doublespinbox->setSuffix(QString::fromStdString(c->suffix));
                    doublespinbox->setButtonSymbols(
                        (c->alignment == abstract_gui_widget::horizontal_align::left)
                        ? QAbstractSpinBox::UpDownArrows
                        : QAbstractSpinBox::NoButtons
                    );

                    switch (c->alignment)
                    {
                        case abstract_gui_widget::horizontal_align::left: doublespinbox->setAlignment(Qt::AlignLeft); break;
                        case abstract_gui_widget::horizontal_align::right: doublespinbox->setAlignment(Qt::AlignRight); break;
                        case abstract_gui_widget::horizontal_align::center: doublespinbox->setAlignment(Qt::AlignHCenter); break;
                        default: k_assert(0, "Unrecognized filter GUI alignment enumerator."); break;
                    }

                    connect(doublespinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](const double newValue)
                    {
                        c->on_change(newValue);
                        c->value = newValue;
                        emit this->mutated(doublespinbox);
                    });
                }
                else
                {
                    k_assert(0, "Unrecognized filter GUI component.");
                }

                k_assert(widget, "Expected the filter GUI widget to have been initialized by now.");

                widget->setEnabled(abstractComponent->isEnabled);
                abstractComponent->set_enabled = [=](const bool isEnabled)
                {
                    widget->setEnabled(isEnabled);
                    abstractComponent->isEnabled = isEnabled;
                };

                widget->setVisible(abstractComponent->isVisible);
                abstractComponent->set_visible = [=](const bool isVisible)
                {
                    widget->setVisible(isVisible);
                    abstractComponent->isVisible = isVisible;
                };

                containerLayout->addWidget(widget);
            }

            switch (gui.layout)
            {
                default:
                case abstract_gui_s::layout_e::form:
                {
                    auto *const label = (field.label.empty()? nullptr : new QLabel(QString::fromStdString(field.label), this));
                    dynamic_cast<QFormLayout*>(widgetLayout)->addRow(label, rowContainer);
                    break;
                }
                case abstract_gui_s::layout_e::vertical_box:
                {
                    widgetLayout->addWidget(rowContainer);
                    break;
                }
            }
        }
    }

    this->wheelBlocker = new WheelBlocker(this);

    return;
}

QtAbstractGUI::~QtAbstractGUI()
{
    delete this->wheelBlocker;

    return;
}
