/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS overlay dialog
 *
 * The overlay allows the user to have custom text appear over the captured frame
 * stream. The overlay dialog, then, allows the user to edit that overlay's contents.
 *
 */

#include <QPlainTextEdit>
#include <QFileDialog>
#include <QStatusBar>
#include <QCheckBox>
#include <QDateTime>
#include <QPainter>
#include <QMenuBar>
#include <QDebug>
#include <QMenu>
#include "display/qt/persistent_settings.h"
#include "display/display.h"
#include "Overlay.h"
#include "ui_Overlay.h"

control_panel::output::Overlay::Overlay(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::Overlay)
{
    overlayDocument.setDefaultFont(QGuiApplication::font());
    overlayDocument.setDocumentMargin(0);

    ui->setupUi(this);

    this->set_name("Overlay editor");

    ui->plainTextEdit->setTabStopWidth(22);

    // Wire up the GUI controls to consequences for operating them.
    {
        connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, this, [this]
        {
            kpers_set_value(INI_GROUP_OVERLAY, "Content", this->ui->plainTextEdit->toPlainText());
            ev_dirty_output_window.fire();
        });

        connect(this, &DialogFragment::enabled_state_set, this, [this](const bool isEnabled)
        {
            kpers_set_value(INI_GROUP_OVERLAY, "Enabled", isEnabled);
            this->ui->groupBox->setChecked(isEnabled);
            ev_dirty_output_window.fire();
        });

        connect(this->ui->groupBox, &QGroupBox::toggled, this, [this](const bool isEnabled)
        {
            this->set_enabled(isEnabled);
        });
    }

    // Restore persistent settings.
    {
        ui->plainTextEdit->setPlainText(kpers_value_of(INI_GROUP_OVERLAY, "Content", "").toString());
        this->set_enabled(kpers_value_of(INI_GROUP_OVERLAY, "Enabled", false).toBool());
    }

    return;
}

control_panel::output::Overlay::~Overlay()
{
    delete ui;
    ui = nullptr;

    return;
}

void control_panel::output::Overlay::set_overlay_max_width(const uint width)
{
    overlayDocument.setTextWidth(width);

    return;
}

// Renders the overlay into a QImage, and returns the image.
QImage control_panel::output::Overlay::rendered(void)
{
    const auto outRes = ks_output_resolution();

    const QString overlaySource = ([this, &outRes]()->QString
    {
        const auto inRes = resolution_s::from_capture_device_properties();
        const auto inHz = refresh_rate_s::from_capture_device_properties();

        QString source = this->ui->plainTextEdit->toPlainText();

        source.replace("$inWidth", QString::number(inRes.w));
        source.replace("$inHeight", QString::number(inRes.h));
        source.replace("$inRate", QString::number(inHz.value<double>(), 'f', 3));
        source.replace("$inChannel", QString("/dev/video%1").arg(kc_device_property("channel")));
        source.replace("$outWidth", QString::number(outRes.w));
        source.replace("$outHeight", QString::number(outRes.h));
        source.replace("$time", QDateTime::currentDateTime().time().toString());
        source.replace("$date", QDateTime::currentDateTime().date().toString());

        return (
            "<span style=\"font-size: large; color: white;\">" +
            source +
            "</span>"
        );
    })();

    QImage image = QImage(outRes.w, outRes.h, QImage::Format_ARGB32_Premultiplied);
    image.fill("transparent");

    QPainter painter(&image);
    overlayDocument.setHtml(overlaySource);
    overlayDocument.drawContents(&painter, image.rect());

    return image;
}
