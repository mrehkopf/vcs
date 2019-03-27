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
#include <QDateTime>
#include <QPainter>
#include <QDebug>
#include <QMenu>
#include "../../common/persistent_settings.h"
#include "../../capture/capture.h"
#include "../display.h"
#include "d_overlay_dialog.h"
#include "ui_d_overlay_dialog.h"
#include "d_window.h"
#include "d_util.h"

static MainWindow *MAIN_WIN = nullptr;

// Used to render the overlay's HTML into an image.
static QTextDocument OVERLAY_DOC;

OverlayDialog::OverlayDialog(MainWindow *const mainWin, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OverlayDialog)
{
    MAIN_WIN = mainWin;
    k_assert(MAIN_WIN != nullptr,
             "Expected a valid main window pointer in the overlay dialog, but got null.");

    OVERLAY_DOC.setDocumentMargin(0);
    OVERLAY_DOC.setTextWidth(width());    // Fixes HTML align not working.

    ui->setupUi(this);

    make_button_menus();

    setWindowTitle("VCS - Overlay");

    // Don't show the context help '?' button in the window bar.
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Load a previous overlay from disk, if any.
    ui->plainTextEdit->setPlainText(kpers_value_of(INI_GROUP_OVERLAY, "content", "").toString());
    ui->groupBox_overlay->setChecked(kpers_value_of(INI_GROUP_OVERLAY, "enabled", 0).toBool());

    resize(kpers_value_of(INI_GROUP_GEOMETRY, "overlay", size()).toSize());

    return;
}

OverlayDialog::~OverlayDialog()
{
    // Save the current settings.
    kpers_set_value(INI_GROUP_OVERLAY, "content", ui->plainTextEdit->toPlainText());
    kpers_set_value(INI_GROUP_OVERLAY, "enabled", ui->groupBox_overlay->isChecked());
    kpers_set_value(INI_GROUP_GEOMETRY, "overlay", size());

    delete ui; ui = nullptr;

    return;
}

// Renders the overlay into a QImage, and returns the image.
//
QImage OverlayDialog::overlay_as_qimage(void)
{
    if (!is_overlay_enabled()) return QImage();

    const resolution_s r = ks_output_resolution();
    QImage image = QImage(r.w, r.h, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(0, 0, 0, 0));

    QPainter painter(&image);
    OVERLAY_DOC.setHtml(parsed_overlay_string());
    OVERLAY_DOC.drawContents(&painter, image.rect());

    return image;
}

void OverlayDialog::insert_text_into_overlay(const QString &t)
{
    ui->plainTextEdit->insertPlainText(t);
    ui->plainTextEdit->setFocus();

    return;
}

// Adds an action to the given menu with the given text, such that when the action
// is triggered, the string in outText is emitted into the overlay.
//
void OverlayDialog::add_menu_item(QMenu *const menu,
                                  const QString &menuText,
                                  const QString &outText)
{
    QAction *const action = menu->addAction(menuText);

    connect(action, &QAction::triggered,
              this, [this,outText]{this->insert_text_into_overlay(outText);});

    return;
}

// Creates menus for the overlay dialog's push-button through which the buttons'
// actions can be triggered.
//
void OverlayDialog::make_button_menus()
{
    QMenu *resolution = new QMenu(this);
    add_menu_item(resolution, "Input", "|inRes|");
    add_menu_item(resolution, "Output", "|outRes|");
    ui->pushButton_resolution->setMenu(resolution);

    QMenu *latency = new QMenu(this);
    add_menu_item(latency, "Peak (ms)", "|msLatP|");
    add_menu_item(latency, "Average (ms)", "|msLatA|");
    latency->addSeparator();
    add_menu_item(latency, "Dropping frames?", "|strLat|");
    ui->pushButton_latency->setMenu(latency);

    QMenu *system = new QMenu(this);
    add_menu_item(system, "Time", "|sysTime|");
    add_menu_item(system, "Date", "|sysDate|");
    ui->pushButton_system->setMenu(system);

    QMenu *refresh = new QMenu(this);
    add_menu_item(refresh, "Input (Hz)", "|inHz|");
    add_menu_item(refresh, "Output (FPS)", "|outFPS|");
    ui->pushButton_refresh->setMenu(refresh);

    QMenu *formatting = new QMenu(this);
    add_menu_item(formatting, "Bullet", "&bull;");
    add_menu_item(formatting, "Line break", "<br>\n");
    add_menu_item(formatting, "Force space", "&nbsp;");
    formatting->addSeparator();
    add_menu_item(formatting, "Bold", "<b></b>");
    add_menu_item(formatting, "Italic", "<i></i>");
    add_menu_item(formatting, "Underline", "<u></u>");
    formatting->addSeparator();
    add_menu_item(formatting, "Align right", "<div align=\"right\">Right</div>");
    formatting->addSeparator();
    formatting->addAction("Image...", this, SLOT(add_image_to_overlay()));
    ui->pushButton_htmlFormat->setMenu(formatting);

    return;
}

// Parses the overlay string to replace certain tags with their corresponding data.
// Returns the modified version.
//
QString OverlayDialog::parsed_overlay_string()
{
    QString o = ui->plainTextEdit->toPlainText();

    o.replace("|inRes|", MAIN_WIN->GetString_InputResolution());
    o.replace("|outRes|", MAIN_WIN->GetString_OutputResolution());
    o.replace("|inHz|", QString("%1").arg(kc_input_signal_info().refreshRate));
    o.replace("|outFPS|", MAIN_WIN->GetString_OutputFrameRate());
    o.replace("|strLat|", (MAIN_WIN->GetString_DroppingFrames() == "Dropping frames")? "Dropping frames" : "");
    o.replace("|msLatP|", QString("%1").arg(kd_peak_system_latency()));
    o.replace("|msLatA|", QString("%1").arg(kd_average_system_latency()));
    o.replace("|sysTime|", QDateTime::currentDateTime().time().toString());
    o.replace("|sysDate|", QDateTime::currentDateTime().date().toString());

    return "<font style=\"font-size: x-large; color: white; background-color: black;\">" + o + "</font>";
}

void OverlayDialog::add_image_to_overlay()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select an image", "",
                                                    "Image files (*.png *.gif *.jpeg *.jpg *.bmp);;"
                                                    "All files(*.*)");
    if (filename.isNull())
    {
        return;
    }

    insert_text_into_overlay("<img src=\"" + filename + "\">");

    return;
}

void OverlayDialog::set_overlay_enabled(const bool state)
{
    { block_widget_signals_c b(ui->groupBox_overlay);
        ui->groupBox_overlay->setChecked(state);
    }

    return;
}

bool OverlayDialog::is_overlay_enabled()
{
    return (!kc_no_signal() && ui->groupBox_overlay->isChecked());
}

void OverlayDialog::on_groupBox_overlay_toggled(bool)
{
    const bool enabled = ui->groupBox_overlay->isChecked();

    if (enabled)
    {
        ui->plainTextEdit->setFocus();
    }

    k_assert(MAIN_WIN != nullptr, "");
    MAIN_WIN->signal_that_overlay_is_enabled(enabled);

    return;
}
