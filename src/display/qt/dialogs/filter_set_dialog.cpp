/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter set dialog
 *
 * A GUI dialog for defining a filter set - a collection of filters that're to be
 * applied to captured frames.
 *
 */

#include <QPushButton>
#include "display/qt/dialogs/filter_set_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "ui_filter_set_dialog.h"

FilterSetDialog::FilterSetDialog(filter_set_s *const filterSet, QWidget *parent, const bool allowApplyButton) :
    QDialog(parent),
    ui(new Ui::FilterSetDialog),
    filterSet(filterSet)
{
    k_assert((filterSet != nullptr), "The filter set dialog expected a valid pointer to a filter set.");

    ui->setupUi(this);

    this->setWindowTitle("VCS - Filter Set");

    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    this->setAttribute(Qt::WA_DeleteOnClose);

    // Make a copy of the given filter set, so we can undo any changes made
    // afterward if the user so requests.
    this->originalFilterSet = *this->filterSet;

    // Set the GUI controls to their proper initial values.
    {
        ui->pushButton_apply->setVisible(allowApplyButton);

        // Append the filter set's description, if any, to the window title.
        if (!filterSet->description.empty())
        {
            this->setWindowTitle(QString("%1 - \"%2\"").arg(this->windowTitle())
                                                       .arg(QString::fromStdString(filterSet->description)));
        }

        // Populate the selection of scalers.
        {
            const std::vector<std::string> scalerNames = ks_list_of_scaling_filter_names();

            k_assert(!scalerNames.empty(),
                     "Expected to receive a list of scalers, but got an empty list.");

            ui->comboBox_scaler->clear();
            for (const auto &name: scalerNames)
            {
                ui->comboBox_scaler->addItem(QString::fromStdString(name));
            }
        }

        // Populate the selection of filters.
        {
            const QStringList filterNames = ([]
            {
                QStringList qStringList;

                const std::vector<std::string> filterNames = kf_filter_name_list();
                for (auto filterName: filterNames)
                {
                    qStringList << QString::fromStdString(filterName);
                }

                return qStringList;
            })();

            k_assert(!filterNames.isEmpty(),
                     "Expected to receive a list of filters, but got an empty list.");

            ui->listWidget_filterList->clear();
            for (const auto &filterName: filterNames)
            {
                QTreeWidgetItem *item = new QTreeWidgetItem;

                item->setText(0, filterName);

                ui->listWidget_filterList->addTopLevelItem(item);
            }
        }

        // Initialize the GUI controls to the data in the given filter set.
        {
            // Activation type.
            {
                if (filterSet->activation & filter_set_s::activation_e::all)
                {
                    ui->radioButton_conditionAlways->setChecked(true);
                }
                else if ((filterSet->activation & filter_set_s::activation_e::in) &&
                         (filterSet->activation & filter_set_s::activation_e::out))
                {
                    ui->radioButton_conditionInput->setChecked(true);
                    ui->checkBox_conditionOutput->setChecked(true);
                    ui->spinBox_inputX->setValue(filterSet->inRes.w);
                    ui->spinBox_inputY->setValue(filterSet->inRes.h);
                    ui->spinBox_outputX->setValue(filterSet->outRes.w);
                    ui->spinBox_outputY->setValue(filterSet->outRes.h);
                }
                else
                {
                    if (filterSet->activation & filter_set_s::activation_e::in)
                    {
                        ui->radioButton_conditionInput->setChecked(true);
                        ui->spinBox_inputX->setValue(filterSet->inRes.w);
                        ui->spinBox_inputY->setValue(filterSet->inRes.h);
                    }

                    if (filterSet->activation & filter_set_s::activation_e::out)
                    {
                        ui->checkBox_conditionOutput->setChecked(true);
                        ui->spinBox_inputX->setValue(filterSet->outRes.w);
                        ui->spinBox_inputY->setValue(filterSet->outRes.h);
                    }
                }
            }

            // Scaler.
            if (filterSet->scaler)
            {
                block_widget_signals_c b(ui->comboBox_scaler);

                set_qcombobox_idx_c(ui->comboBox_scaler).by_string(QString::fromStdString(filterSet->scaler->name));
            }

            ui->treeWidget_preFilters->set_filters(filterSet->preFilters);
            ui->treeWidget_postFilters->set_filters(filterSet->postFilters);
        }
    }

    // Connect GUI controls to consequences for operating them.
    {
        // Enable/disable the filter set activation input/output condition controls
        // if the user has specified that there should be such conditions.
        connect(ui->radioButton_conditionInput, &QRadioButton::toggled, this, [this](bool state)
        {
            ui->spinBox_inputX->setEnabled(state);
            ui->spinBox_inputY->setEnabled(state);

            ui->checkBox_conditionOutput->setEnabled(state);
            ui->spinBox_outputX->setEnabled(state && ui->checkBox_conditionOutput->isChecked());
            ui->spinBox_outputY->setEnabled(state && ui->checkBox_conditionOutput->isChecked());
        });

        connect(ui->checkBox_conditionOutput, &QCheckBox::toggled, this, [this](bool state)
        {
            ui->spinBox_outputX->setEnabled(state);
            ui->spinBox_outputY->setEnabled(state);
        });

        // Returns a filter set whose attributes are set based on the dialog's
        // user-definable GUI state at the time of this call.
        auto current_filter_set = [=]
        {
            filter_set_s c;

            c.isEnabled = filterSet->isEnabled;
            c.description = filterSet->description;
            c.scaler = ks_scaler_for_name_string(ui->comboBox_scaler->currentText().toStdString());
            c.activation = filter_set_s::activation_e::none;
            c.preFilters = ui->treeWidget_preFilters->filters();
            c.postFilters = ui->treeWidget_postFilters->filters();

            if (ui->radioButton_conditionAlways->isChecked())
            {
                c.activation |= filter_set_s::activation_e::all;
            }
            else
            {
                if (ui->radioButton_conditionInput->isChecked()) c.activation |= filter_set_s::activation_e::in;
                if (ui->checkBox_conditionOutput->isChecked()) c.activation |= filter_set_s::activation_e::out;
            }

            c.inRes.w = ui->spinBox_inputX->value();
            c.inRes.h = ui->spinBox_inputY->value();
            c.outRes.w = ui->spinBox_outputX->value();
            c.outRes.h = ui->spinBox_outputY->value();

            return c;
        };

        // Copy the current settings from the GUI into the target filter set, but
        // don't exit the dialog.
        connect(ui->pushButton_apply, &QPushButton::clicked, this,
                [=]{ *filterSet = current_filter_set(); });

        // Copy the current settings from the GUI into the target filter set, and
        // exit the dialog.
        connect(ui->pushButton_ok, &QPushButton::clicked, this,
                [=]{ *filterSet = current_filter_set(); this->done(1); });

        // Undo any changes, and exit the dialog.
        connect(ui->pushButton_cancel, &QPushButton::clicked, this,
                [=]{ *filterSet = originalFilterSet; this->done(0); });
    }

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "filter_set", size()).toSize());
    }

    return;
}

FilterSetDialog::~FilterSetDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "filter_set", size());
    }

    delete ui;

    return;
}
