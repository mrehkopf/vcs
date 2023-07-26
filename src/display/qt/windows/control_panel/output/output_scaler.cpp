#include "scaler/scaler.h"
#include "display/qt/persistent_settings.h"
#include "output_scaler.h"
#include "ui_output_scaler.h"

WindowScaler::WindowScaler(QWidget *parent) :
    VCSDialogFragment(parent),
    ui(new Ui::WindowScaler)
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

WindowScaler::~WindowScaler()
{
    delete ui;
}
