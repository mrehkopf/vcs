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
#include <QMenuBar>
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
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        connect(ui->groupBox_overlayEnabled, &QGroupBox::toggled, this, [=]
        {
            kd_update_output_window_title();
            this->menubar->setEnabled(ui->groupBox_overlayEnabled->isChecked());
        });
    }

    // Create the dialog's menu bar.
    {
        this->menubar = new QMenuBar(this);

        this->menubar->setEnabled(ui->groupBox_overlayEnabled->isChecked());

        // Variables...
        {
            QMenu *variablesMenu = new QMenu("Variables", this->menubar);

            // Capture input.
            {
                QMenu *inputMenu = new QMenu("Input", this->menubar);

                connect(inputMenu->addAction("Resolution"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("$inputResolution");
                });

                connect(inputMenu->addAction("Refresh rate (Hz)"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("$inputHz");
                });

                variablesMenu->addMenu(inputMenu);
            }

            // Capture output.
            {
                QMenu *outputMenu = new QMenu("Output", this->menubar);

                connect(outputMenu->addAction("Resolution"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("$outputResolution");
                });

                connect(outputMenu->addAction("Frame rate (FPS)"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("$outputFPS");
                });

                connect(outputMenu->addAction("Frames dropped?"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("$areFramesDropped");
                });

                connect(outputMenu->addAction("Peak latency (ms)"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("$peakLatencyMs");
                });

                connect(outputMenu->addAction("Average latency (ms)"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("$averageLatencyMs");
                });

                variablesMenu->addMenu(outputMenu);
            }

            variablesMenu->addSeparator();

            // System.
            {
                QMenu *systemMenu = new QMenu("System", this->menubar);

                connect(systemMenu->addAction("Time"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("$systemTime");
                });

                connect(systemMenu->addAction("Date"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("$systemDate");
                });

                variablesMenu->addMenu(systemMenu);
            }

            this->menubar->addMenu(variablesMenu);
        }

        // Formatting...
        {
            QMenu *formattingMenu = new QMenu("Formatting", this->menubar);

            connect(formattingMenu->addAction("Line break"), &QAction::triggered, this, [=]
            {
                this->insert_text_into_overlay_editor("<br>\n");
            });

            connect(formattingMenu->addAction("Image..."), &QAction::triggered, this, [=]
            {
                const QString filename = QFileDialog::getOpenFileName(this, "Select an image", "",
                                                                      "Image files (*.png *.gif *.jpeg *.jpg *.bmp);;"
                                                                      "All files(*.*)");

                if (filename.isEmpty())
                {
                    return;
                }

                insert_text_into_overlay_editor("<img src=\"" + filename + "\">");
            });

            formattingMenu->addSeparator();

            // Align.
            {
                QMenu *alignMenu = new QMenu("Align", this->menubar);

                connect(alignMenu->addAction("Left"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("<div style=\"text-align: left;\"></div>");
                });

                connect(alignMenu->addAction("Right"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("<div style=\"text-align: right;\"></div>");
                });

                connect(alignMenu->addAction("Center"), &QAction::triggered, this, [=]
                {
                    this->insert_text_into_overlay_editor("<div style=\"text-align: center;\"></div>");
                });

                formattingMenu->addMenu(alignMenu);
            }

            this->menubar->addMenu(formattingMenu);
        }

        this->layout()->setMenuBar(menubar);
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

    const auto inRes = kc_capture_api().get_resolution();
    const auto outRes = ks_output_resolution();

    parsed.replace("$inputResolution",  QString("%1 x %2").arg(inRes.w).arg(inRes.h));
    parsed.replace("$outputResolution", QString("%1 x %2").arg(outRes.w).arg(outRes.h));
    parsed.replace("$inputHz",          QString::number(kc_capture_api().get_refresh_rate()));
    parsed.replace("$outputFPS",        QString::number(kd_output_framerate()));
    parsed.replace("$areFramesDropped", ((kc_capture_api().get_missed_frames_count() > 0)? "Dropping frames" : ""));
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
