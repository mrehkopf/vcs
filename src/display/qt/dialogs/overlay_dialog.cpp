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
#include "display/qt/dialogs/overlay_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "display/display.h"
#include "capture/capture_api.h"
#include "capture/capture.h"
#include "ui_overlay_dialog.h"

OverlayDialog::OverlayDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OverlayDialog)
{
    overlayDocument.setDefaultFont(QGuiApplication::font());
    overlayDocument.setDocumentMargin(0);

    ui->setupUi(this);

    this->setWindowTitle("VCS - Overlay");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Set the GUI controls to their proper initial values.
    {
        ui->groupBox_overlayEnabled->setChecked(false);

        // Create push button menus for inserting variables into the overlay.
        {
            // Adds an action into the given menu, such that when the action is
            // triggered, the given output string is inserted into the overlay's text
            // field.
            auto add_action_to_menu = [=](QMenu *const menu,
                                          const QString &actionText,
                                          const QString &actionOutput)
            {
                connect(menu->addAction(actionText), &QAction::triggered, this,
                        [=]{ this->insert_text_into_overlay_editor(actionOutput); });

                return;
            };

            QMenu *capture = new QMenu(this);
            {
                QMenu *input = new QMenu("Input", this);
                QMenu *output = new QMenu("Output", this);

                add_action_to_menu(input, "Resolution", "$inputResolution");
                add_action_to_menu(input, "Refresh rate (Hz)", "$inputHz");

                add_action_to_menu(output, "Resolution", "$outputResolution");
                add_action_to_menu(output, "Frame rate", "$outputFPS");
                add_action_to_menu(output, "Frames dropped?", "$areFramesDropped");
                add_action_to_menu(output, "Peak capture latency (ms)", "$peakLatencyMs");
                add_action_to_menu(output, "Average capture latency (ms)", "$averageLatencyMs");

                capture->addMenu(input);
                capture->addMenu(output);
                ui->pushButton_capture->setMenu(capture);
            }

            QMenu *system = new QMenu(this);
            add_action_to_menu(system, "Time", "$systemTime");
            add_action_to_menu(system, "Date", "$systemDate");
            ui->pushButton_system->setMenu(system);

            QMenu *formatting = new QMenu(this);
            add_action_to_menu(formatting, "Bullet", "&bull;");
            add_action_to_menu(formatting, "Line break", "<br>\n");
            add_action_to_menu(formatting, "Force space", "&nbsp;");
            formatting->addSeparator();
            add_action_to_menu(formatting, "Bold", "<b></b>");
            add_action_to_menu(formatting, "Italic", "<i></i>");
            add_action_to_menu(formatting, "Underline", "<u></u>");
            formatting->addSeparator();
            add_action_to_menu(formatting, "Align right", "<div align=\"right\">Right</div>");
            formatting->addSeparator();
            formatting->addAction("Image...", this, SLOT(add_image_to_overlay()));
            ui->pushButton_htmlFormat->setMenu(formatting);
        }
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        connect(ui->groupBox_overlayEnabled, &QGroupBox::toggled, this, [=]{kd_update_output_window_title();});
    }

    // Restore persistent settings.
    {
        ui->plainTextEdit->setPlainText(kpers_value_of(INI_GROUP_OVERLAY, "content", "").toString());
        ui->groupBox_overlayEnabled->setChecked(kpers_value_of(INI_GROUP_OVERLAY, "enabled", false).toBool());
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "overlay", this->size()).toSize());
    }

    return;
}

OverlayDialog::~OverlayDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_OVERLAY, "enabled", ui->groupBox_overlayEnabled->isChecked());
        kpers_set_value(INI_GROUP_OVERLAY, "content", ui->plainTextEdit->toPlainText());
        kpers_set_value(INI_GROUP_GEOMETRY, "overlay", this->size());
    }

    delete ui;
    ui = nullptr;

    return;
}

void OverlayDialog::set_overlay_max_width(const uint width)
{
    overlayDocument.setTextWidth(width);

    return;
}

// Renders the overlay into a QImage, and returns the image.
QImage OverlayDialog::overlay_as_qimage(void)
{
    const resolution_s outputRes = ks_output_resolution();

    QImage image = QImage(outputRes.w, outputRes.h, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(0, 0, 0, 0));

    QPainter painter(&image);
    overlayDocument.setHtml(parsed_overlay_string());
    overlayDocument.drawContents(&painter, image.rect());

    return image;
}

// Appends the given bit of text into the overlay editor's text field at its
// current cursor position.
void OverlayDialog::insert_text_into_overlay_editor(const QString &text)
{
    ui->plainTextEdit->insertPlainText(text);
    ui->plainTextEdit->setFocus();

    return;
}

// Parses the overlay string to replace variable tags with their corresponding
// data, and returns the parsed version.
QString OverlayDialog::parsed_overlay_string(void)
{
    QString parsed = ui->plainTextEdit->toPlainText();

    /// TODO. I'm not totally sure how good, performance-wise, it is to poll the
    /// capture hardware every frame. Depends on how the Datapath API caches this
    /// info etc.
    const auto inRes = kc_capture_api().get_resolution();
    const auto outRes = ks_output_resolution();

    parsed.replace("$inputResolution",  QString("%1 x %2").arg(inRes.w).arg(inRes.h));
    parsed.replace("$outputResolution", QString("%1 x %2").arg(outRes.w).arg(outRes.h));
    parsed.replace("$inputHz",          QString::number(kc_capture_api().get_refresh_rate()));
    parsed.replace("$outputFPS",        QString::number(kd_output_framerate()));
    parsed.replace("$areFramesDropped", (kc_capture_api().are_frames_being_dropped()? "Dropping frames" : ""));
    parsed.replace("$peakLatencyMs",    QString::number(kd_peak_pipeline_latency()));
    parsed.replace("$averageLatencyMs", QString::number(kd_average_pipeline_latency()));
    parsed.replace("$systemTime",       QDateTime::currentDateTime().time().toString());
    parsed.replace("$systemDate",       QDateTime::currentDateTime().date().toString());

    return ("<font style=\"font-size: large; color: white; background-color: black;\">" + parsed + "</font>");
}

// A convenience function to query the user for the name of an image file, and
// then to insert into the overlay editor's text field corresponding HTML code
// to display that image in the overlay.
void OverlayDialog::add_image_to_overlay(void)
{
    QString filename = QFileDialog::getOpenFileName(this, "Select an image", "",
                                                    "Image files (*.png *.gif *.jpeg *.jpg *.bmp);;"
                                                    "All files(*.*)");

    if (filename.isEmpty())
    {
        return;
    }

    insert_text_into_overlay_editor("<img src=\"" + filename + "\">");

    return;
}

bool OverlayDialog::is_overlay_enabled(void)
{
    return ui->groupBox_overlayEnabled->isChecked();
}

void OverlayDialog::toggle_overlay(void)
{
    ui->groupBox_overlayEnabled->setChecked(!ui->groupBox_overlayEnabled->isChecked());

    return;
}
