#include "scaler/scaler.h"
#include "display/qt/persistent_settings.h"
#include "Scaler.h"
#include "ui_Scaler.h"

control_panel::output::Scaler::Scaler(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::Scaler)
{
    ui->setupUi(this);

    connect(ui->comboBox_scalerSelect, &QComboBox::currentTextChanged, this, [](const QString &scalerName)
    {
        ks_set_default_scaler(scalerName.toStdString());
        kpers_set_value(INI_GROUP_OUTPUT_WINDOW, "DefaultScaler", scalerName);
    });

    // Restore the previous persistent setting.
    ui->comboBox_scalerSelect->setCurrentText(kpers_value_of(INI_GROUP_OUTPUT_WINDOW, "DefaultScaler", ui->comboBox_scalerSelect->itemText(0)).toString());
}

control_panel::output::Scaler::~Scaler()
{
    delete ui;
}
