/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter set dialog
 *
 * A GUI dialog for defining a filter set - a collection of filters that're to be
 * applied to captured frames.
 *
 */

#include <QPushButton>
#include <map>
#include "../../display/qt/d_filter_set_dialog.h"
#include "../../common/persistent_settings.h"
#include "../../display/qt/df_filters.h"
#include "../../display/qt/d_util.h"
#include "../../common/globals.h"
#include "../../scaler/scaler.h"
#include "../../common/memory.h"
#include "ui_d_filter_set_dialog.h"

// Takes in the filter set for which we'll set the parameters.
//
FilterSetDialog::FilterSetDialog(filter_set_s *const filterSet, QWidget *parent, const bool allowApply) :
    QDialog(parent),
    ui(new Ui::FilterSetDialog),
    filterSet(filterSet)
{
    k_assert((filterSet != nullptr), "The filter set dialog expected a valid pointer to a filter set struct.");

    this->backupFilterSet = *this->filterSet;

    ui->setupUi(this);

    // The CSV parser preserves {} for its own use, so discourage the user from
    // entering those characters into the description string.
    /// TODO: When saving data, you should pass it through the CSV parser for
    /// validation, instead of validating on the spot like this.
    ui->lineEdit_description->setValidator(new QRegExpValidator(QRegExp("[^{}]*"), this));

    if (!allowApply)
    {
        ui->pushButton_apply->setEnabled(false);
    }

    this->setWindowTitle("VCS - Define a filter set");
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    this->setAttribute(Qt::WA_DeleteOnClose);

    // Populate the selection of filters.
    {
        const QStringList filterNames = []()->QStringList
        {
            QStringList list;
            const std::vector<std::string> stringList = kf_filter_name_list();
            for (auto string: stringList) list << QString::fromStdString(string);
            return list;
        }();
        k_assert(!filterNames.isEmpty(),
                 "Expected to receive a list of filters, but got an empty list.");

        ui->listWidget_filterList->clear();
        ui->listWidget_filterList->addItems(filterNames);
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

    // Initialize the GUI controls to the data in the given filter set.
    {
        ui->lineEdit_description->setText(QString::fromStdString(filterSet->description));

        // Activation type.
        {
            if (filterSet->activation & filter_set_s::activation_e::all)
            {
                ui->radioButton_activeAlways->setChecked(true);
            }
            else if ((filterSet->activation & filter_set_s::activation_e::in) &&
                     (filterSet->activation & filter_set_s::activation_e::out))
            {
                ui->radioButton_activeIn->setChecked(true);
                ui->checkBox_activeOut->setChecked(true);

                ui->spinBox_inputX->setValue(filterSet->inRes.w);
                ui->spinBox_inputY->setValue(filterSet->inRes.h);
                ui->spinBox_outputX->setValue(filterSet->outRes.w);
                ui->spinBox_outputY->setValue(filterSet->outRes.h);
            }
            else
            {
                if (filterSet->activation & filter_set_s::activation_e::in)
                {
                    ui->radioButton_activeIn->setChecked(true);

                    ui->spinBox_inputX->setValue(filterSet->inRes.w);
                    ui->spinBox_inputY->setValue(filterSet->inRes.h);
                }

                if (filterSet->activation & filter_set_s::activation_e::out)
                {
                    ui->checkBox_activeOut->setChecked(true);

                    ui->spinBox_inputX->setValue(filterSet->outRes.w);
                    ui->spinBox_inputY->setValue(filterSet->outRes.h);
                }
            }
        }

        // Scaler.
        if (filterSet->scaler != nullptr)
        {
            const int itemIdx = ui->comboBox_scaler->findText(QString::fromStdString(filterSet->scaler->name));
            if (itemIdx >= 0)
            {
                block_widget_signals_c b(ui->comboBox_scaler);
                ui->comboBox_scaler->setCurrentIndex(itemIdx);
            }
        }

        // Pre and post-filters.
        {
            ui->listWidget_preFilters->clear();
            ui->listWidget_postFilters->clear();

            auto add_filters_to_gui = [=](const std::vector<filter_s> filters, QListWidget *const filterList)
            {
                for (auto &filter: filters)
                {
                    block_widget_signals_c b(filterList);

                    QListWidgetItem *itm = new QListWidgetItem(filterList);

                    itm->setText(QString::fromStdString(filter.name));

                    filterData[itm] = (u8*)kmem_allocate(FILTER_DATA_LENGTH, "Filter set data");
                    memcpy(filterData[itm], filter.data, FILTER_DATA_LENGTH);

                    filterList->addItem(itm);
                }
            };

            add_filters_to_gui(filterSet->preFilters, ui->listWidget_preFilters);
            add_filters_to_gui(filterSet->postFilters, ui->listWidget_postFilters);
        }
    }

    resize(kpers_value_of(INI_GROUP_GEOMETRY, "filter_set", size()).toSize());

    return;
}

FilterSetDialog::~FilterSetDialog()
{
    kpers_set_value(INI_GROUP_GEOMETRY, "filter_set", size());

    delete ui;

    for (auto item: filterData)
    {
        kmem_release((void**)&item.second);
    }

    return;
}

// Constructs a filter set out of the current user-definable GUI state.
//
filter_set_s FilterSetDialog::get_filter_set_from_current_state(void)
{
    filter_set_s c;

    // Default to all new sets being enabled.
    c.isEnabled = true;

    // Add the pre-filters.
    /// TODO. Code duplication.
    for (int i = 0; i < ui->listWidget_preFilters->count(); i++)
    {
        const QListWidgetItem *const item = ui->listWidget_preFilters->item(i);
        const QString filterName = item->text();

        filter_s f;

        f.name = filterName.toStdString();
        if (!kf_named_filter_exists(f.name))
        {
            NBENE(("Was asked to access a filter that doesn't exist. Ignoring this."));
            continue;
        }

        memcpy(f.data, filterData[item], FILTER_DATA_LENGTH);

        c.preFilters.push_back(f);
    }

    // Add the post-filters.
    for (int i = 0; i < ui->listWidget_postFilters->count(); i++)
    {
        const QListWidgetItem *const item = ui->listWidget_postFilters->item(i);
        const QString filterName = ui->listWidget_postFilters->item(i)->text();

        filter_s f;

        f.name = filterName.toStdString();
        if (!kf_named_filter_exists(f.name))
        {
            NBENE(("Was asked to access a filter that doesn't exist. Ignoring this."));
            continue;
        }

        memcpy(f.data, filterData[item], FILTER_DATA_LENGTH);

        c.postFilters.push_back(f);
    }

    c.description = ui->lineEdit_description->text().toStdString();

    c.scaler = ks_scaler_for_name_string(ui->comboBox_scaler->currentText().toStdString());

    c.activation = filter_set_s::activation_e::none;
    if (ui->radioButton_activeAlways->isChecked()) c.activation |= filter_set_s::activation_e::all;
    else
    {
        if (ui->radioButton_activeIn->isChecked()) c.activation |= filter_set_s::activation_e::in;
        if (ui->checkBox_activeOut->isChecked()) c.activation |= filter_set_s::activation_e::out;
    }

    c.inRes.w = ui->spinBox_inputX->value();
    c.inRes.h = ui->spinBox_inputY->value();
    c.outRes.w = ui->spinBox_outputX->value();
    c.outRes.h = ui->spinBox_outputY->value();

    return c;
}

void FilterSetDialog::on_listWidget_preFilters_itemDoubleClicked(QListWidgetItem *item)
{
    // Pop up a dialog letting the user set the filter's parameters.
    kf_filter_dialog_for_name(item->text().toStdString())->poll_user_for_params(filterData[item], this);

    return;
}

void FilterSetDialog::on_listWidget_postFilters_itemDoubleClicked(QListWidgetItem *item)
{
    // Pop up a dialog letting the user set the filter's parameters.
    kf_filter_dialog_for_name(item->text().toStdString())->poll_user_for_params(filterData[item], this);

    return;
}

void FilterSetDialog::on_listWidget_preFilters_itemChanged(QListWidgetItem *item)
{
    filterData[item] = (u8*)kmem_allocate(FILTER_DATA_LENGTH, "Pre-filter data");
    kf_filter_dialog_for_name(item->text().toStdString())->insert_default_params(filterData[item]);

    return;
}

void FilterSetDialog::on_listWidget_postFilters_itemChanged(QListWidgetItem *item)
{
    filterData[item] = (u8*)kmem_allocate(FILTER_DATA_LENGTH, "Post-filter data");
    kf_filter_dialog_for_name(item->text().toStdString())->insert_default_params(filterData[item]);

    return;
}

// Toggle controls related to the activation selection.
//
void FilterSetDialog::on_radioButton_activeIn_toggled(bool checked)
{
    // Toggle the "in" controls.
    ui->spinBox_inputX->setEnabled(checked);
    ui->spinBox_inputY->setEnabled(checked);
    ui->label_inputSeparator->setEnabled(checked);

    // Toggle the "out" controls.
    ui->checkBox_activeOut->setEnabled(checked);
    ui->spinBox_outputX->setEnabled(checked && ui->checkBox_activeOut->isChecked());
    ui->spinBox_outputY->setEnabled(checked && ui->checkBox_activeOut->isChecked());
    ui->label_outputSparator->setEnabled(checked && ui->checkBox_activeOut->isChecked());

    return;
}

// Toggle the activation output controls.
//
void FilterSetDialog::on_checkBox_activeOut_toggled(bool checked)
{
    ui->spinBox_outputX->setEnabled(checked);
    ui->spinBox_outputY->setEnabled(checked);
    ui->label_outputSparator->setEnabled(checked);

    return;
}

void FilterSetDialog::apply_current_settings(void)
{
    *filterSet = get_filter_set_from_current_state();
    return;
}

void FilterSetDialog::on_pushButton_ok_clicked()
{
    apply_current_settings();
    done(1);
}

void FilterSetDialog::on_pushButton_cancel_clicked()
{
    *filterSet = backupFilterSet;
    done(0);
}

void FilterSetDialog::on_pushButton_apply_clicked()
{
    apply_current_settings();
    return;
}
