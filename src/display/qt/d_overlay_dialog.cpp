/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS overlay dialog
 *
 * The overlay allows the user to have custom text appear over the captured frame
 * stream. The overlay dialog, then, allows the user to edit that overlay's contents.
 *
 */

#include <QPlainTextEdit>
#include <QSignalMapper>
#include <QFileDialog>
#include <QDateTime>
#include <QDebug>
#include <QMenu>
#include "../../common/persistent_settings.h"
#include "../../capture/capture.h"
#include "../display.h"
#include "d_overlay_dialog.h"
#include "ui_d_overlay_dialog.h"
#include "d_window.h"
#include "d_util.h"

/*
 * TODOS:
 *
 * - on linux (qt 5.5), tosses out a bunch of qfont errors if overlay is enabled.
 *
 */

// The overlay's current, html-formatted string.
static QString OVERLAY_STRING;

static MainWindow *MAIN_WIN = nullptr;

OverlayDialog::OverlayDialog(MainWindow *const mainWin, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OverlayDialog)
{
    MAIN_WIN = mainWin;
    k_assert(MAIN_WIN != nullptr,
             "Expected a valid main window pointer in the overlay dialog, but got null.");

    ui->setupUi(this);

    make_button_menus();

    setWindowTitle("VCS - Overlay");

    // Don't show the context help '?' button in the window bar.
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Load a previous overlay from disk, if any.
    ui->plainTextEdit->setPlainText(kpers_value_of("content", INI_GROUP_OVERLAY, "").toString());
    ui->groupBox_overlay->setChecked(kpers_value_of("enabled", INI_GROUP_OVERLAY, 0).toBool());

    resize(kpers_value_of("overlay", INI_GROUP_GEOMETRY, size()).toSize());

    return;
}

OverlayDialog::~OverlayDialog()
{
    // Save the current settings.
    kpers_set_value("content", INI_GROUP_OVERLAY, ui->plainTextEdit->toPlainText());
    kpers_set_value("enabled", INI_GROUP_OVERLAY, ui->groupBox_overlay->isChecked());
    kpers_set_value("overlay", INI_GROUP_GEOMETRY, size());

    delete ui; ui = nullptr;

    return;
}

// To get a displayable (parsed) overlay string, call this function and use the string
// it returns.
//
QString OverlayDialog::overlay_html_string()
{
    finalize_overlay_string();

    return OVERLAY_STRING;
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
                                  QSignalMapper *const sMapper,
                                  const QString &menuText,
                                  const QString &outText)
{
    QAction *const action = menu->addAction(menuText);

    connect(action, SIGNAL(triggered()), sMapper, SLOT(map()));
    sMapper->setMapping(action, outText);

    return;
}

// Creates menus for the overlay dialog's push-button through which the buttons'
// actions can be triggered.
//
void OverlayDialog::make_button_menus()
{
    QSignalMapper *sm = new QSignalMapper(this);

    QMenu *resolution = new QMenu(this);
    add_menu_item(resolution, sm, "Input", "|inRes|");
    add_menu_item(resolution, sm, "Output", "|outRes|");
    ui->pushButton_resolution->setMenu(resolution);

    QMenu *latency = new QMenu(this);
    add_menu_item(latency, sm, "Peak (ms)", "|msLatP|");
    add_menu_item(latency, sm, "Average (ms)", "|msLatA|");
    latency->addSeparator();
    add_menu_item(latency, sm, "Dropping frames?", "|strLat|");
    ui->pushButton_latency->setMenu(latency);

    QMenu *system = new QMenu(this);
    add_menu_item(system, sm, "Time", "|sysTime|");
    add_menu_item(system, sm, "Date", "|sysDate|");
    ui->pushButton_system->setMenu(system);

    QMenu *refresh = new QMenu(this);
    add_menu_item(refresh, sm, "Input (Hz)", "|inHz|");
    add_menu_item(refresh, sm, "Output (FPS)", "|outFPS|");
    ui->pushButton_refresh->setMenu(refresh);

    QMenu *formatting = new QMenu(this);
    add_menu_item(formatting, sm, "Bullet", "&bull;");
    add_menu_item(formatting, sm, "Line break", "<br>\n");
    add_menu_item(formatting, sm, "Force space", "&nbsp;");
    formatting->addSeparator();
    add_menu_item(formatting, sm, "Bold", "<b></b>");
    add_menu_item(formatting, sm, "Italic", "<i></i>");
    add_menu_item(formatting, sm, "Underline", "<u></u>");
    formatting->addSeparator();
    add_menu_item(formatting, sm, "Align right", "<div align=\"right\">Right</div>");
    formatting->addSeparator();
    formatting->addAction("Image...", this, SLOT(add_image_to_overlay()));
    ui->pushButton_htmlFormat->setMenu(formatting);

    // When a menu item is triggered, the given slot will be called.
    connect(sm, SIGNAL(mapped(QString)), this, SLOT(insert_text_into_overlay(QString)));

    return;
}

// Parses the overlay string to replace certain tags with their corresponding data.
//
void OverlayDialog::finalize_overlay_string()
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

    OVERLAY_STRING = "<font style=\"font-size: x-large; color: white; background-color: black;\">" +
                     o + "</font>";

    return;
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
    return (!kc_is_no_signal() && ui->groupBox_overlay->isChecked());
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
