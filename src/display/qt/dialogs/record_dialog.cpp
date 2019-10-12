/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * The widget that makes up the control panel's 'Record' tab.
 *
 */

#include <QFileDialog>
#include <QFileInfo>
#include "display/qt/dialogs/record_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/qt/utility.h"
#include "scaler/scaler.h"
#include "record/record.h"
#include "ui_record_dialog.h"

#if _WIN32
    // For accessing the Windows registry.
    #include <windows.h>
#endif

RecordDialog::RecordDialog(QDialog *parent) :
    QDialog(parent),
    ui(new Ui::RecordDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("VCS - Record");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Certain features of recording are not available on certain operating systems;
    // so disable them accordingly.
    {
        // Encoder settings.
        #if _WIN32
            /// At the moment, no changes are needed on Windows.
        #elif __linux__
            // The x264 settings are hardcoded into OpenCV's libraries on Linux, so
            // they can't be altered via the VCS GUI.
            ui->frame_recordingCodecSettings->setEnabled(false);
        #else
            #error "Unknown platform."
        #endif

        // Video container.
        {
            QString containerName;
            #if _WIN32
                // We'll use the x264vfw encoder on Windows, which outputs into AVI.
                containerName = "AVI";
            #elif __linux__
                containerName = "MP4";
            #else
                #error "Unknown platform."
            #endif

            ui->comboBox_recordingContainer->addItem(containerName);
            ui->lineEdit_recordingFilename->setText(ui->lineEdit_recordingFilename->text().append(containerName.toLower()));
        }

        // Encoder for video recording.
        {
            QString encoderName;
            #if _WIN32
                encoderName = "x264vfw";
            #elif __linux__
                encoderName = "x264";
            #else
                #error "Unknown platform."
            #endif

            ui->comboBox_recordingEncoding->addItem(encoderName);
        }
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        connect(ui->pushButton_recordingStart, &QPushButton::clicked, this, [this]
        {
            if (!apply_x264_registry_settings()) return;

            // Remove the existing file, if any. Without this, the file size reported
            // in VCS's GUI during recording may be inaccurate on some platforms.
            QFile(ui->lineEdit_recordingFilename->text()).remove();

            const resolution_s videoResolution = ks_output_resolution();

            krecord_start_recording(ui->lineEdit_recordingFilename->text().toStdString().c_str(),
                                        videoResolution.w, videoResolution.h,
                                        ui->spinBox_recordingFramerate->value(),
                                        ui->checkBox_recordingLinearFrameInsertion->isChecked());
        });

        connect(ui->pushButton_recordingStop, &QPushButton::clicked, this, [this]
        {
            krecord_stop_recording();
        });

        connect(ui->pushButton_recordingSelectFilename, &QPushButton::clicked, this, [this]
        {
            QString filename = QFileDialog::getSaveFileName(this,
                                                            "Select a file to record the video into", "",
                                                            "All files(*.*)");

            if (filename.isEmpty()) return;

            ui->lineEdit_recordingFilename->setText(filename);
        });
    }

    // Restore persistent settings.
    {
        ui->spinBox_recordingFramerate->setValue(kpers_value_of(INI_GROUP_RECORDING, "frame_rate", 60).toUInt());
        ui->checkBox_recordingLinearFrameInsertion->setChecked(kpers_value_of(INI_GROUP_RECORDING, "linear_sampling", true).toBool());
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "record", this->size()).toSize());

        #if _WIN32
            set_qcombobox_idx_c(ui->comboBox_recordingEncoderProfile)
                               .by_string(kpers_value_of(INI_GROUP_RECORDING, "profile", "High 4:4:4").toString());

            set_qcombobox_idx_c(ui->comboBox_recordingEncoderPixelFormat)
                               .by_string(kpers_value_of(INI_GROUP_RECORDING, "pixel_format", "RGB").toString());

            set_qcombobox_idx_c(ui->comboBox_recordingEncoderPreset)
                               .by_string(kpers_value_of(INI_GROUP_RECORDING, "preset", "Superfast").toString());

            ui->spinBox_recordingEncoderCRF->setValue(kpers_value_of(INI_GROUP_RECORDING, "crf", 1).toUInt());
            ui->checkBox_recordingEncoderZeroLatency->setChecked(kpers_value_of(INI_GROUP_RECORDING, "zero_latency", false).toBool());
            ui->lineEdit_recordingEncoderArguments->setText(kpers_value_of(INI_GROUP_RECORDING, "command_line", "").toString());
        #endif
    }

    return;
}

RecordDialog::~RecordDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_RECORDING, "frame_rate", ui->spinBox_recordingFramerate->value());
        kpers_set_value(INI_GROUP_RECORDING, "linear_sampling", ui->checkBox_recordingLinearFrameInsertion->isChecked());
        kpers_set_value(INI_GROUP_GEOMETRY, "record", this->size());

        #if _WIN32
            // Encoder settings. These aren't available to the user on Linux (non-Windows) builds.
            kpers_set_value(INI_GROUP_RECORDING, "profile", ui->comboBox_recordingEncoderProfile->currentText());
            kpers_set_value(INI_GROUP_RECORDING, "pixel_format", ui->comboBox_recordingEncoderPixelFormat->currentText());
            kpers_set_value(INI_GROUP_RECORDING, "preset", ui->comboBox_recordingEncoderPreset->currentText());
            kpers_set_value(INI_GROUP_RECORDING, "crf", ui->spinBox_recordingEncoderCRF->value());
            kpers_set_value(INI_GROUP_RECORDING, "zero_latency", ui->checkBox_recordingEncoderZeroLatency->isChecked());
            kpers_set_value(INI_GROUP_RECORDING, "command_line", ui->lineEdit_recordingEncoderArguments->text());
        #endif
    }

    delete ui;

    return;
}

// Enables or disables the dialog's GUI controls for changing recording settings,
// starting/stopping the recording, etc. Note that when the controls are set to
// disabled, the 'stop recording' button will be enabled - it's assumed that one
// calls this function to disable the controls as a result of recording having
// begun.
void RecordDialog::set_recording_controls_enabled(const bool areEnabled)
{
    if (areEnabled)
    {
        ui->pushButton_recordingStart->setEnabled(false);
        ui->pushButton_recordingStop->setEnabled(true);
        ui->frame_recordingSettings->setEnabled(false);
        ui->frame_recordingFile->setEnabled(false);
    }
    else
    {
        ui->pushButton_recordingStart->setEnabled(true);
        ui->pushButton_recordingStop->setEnabled(false);
        ui->frame_recordingSettings->setEnabled(true);
        ui->frame_recordingFile->setEnabled(true);
    }

    return;
}

void RecordDialog::update_recording_metainfo(void)
{
    if (krecord_is_recording())
    {
        const uint fileBytesize = QFileInfo(krecord_video_filename().c_str()).size();
        {
            double size = fileBytesize;
            QString suffix = "byte(s)";

            if (size > 1024*1024*1024)
            {
                size /= 1024*1024*1024;
                suffix = "GB";
            }
            else if (size > 1024*1024)
            {
                size /= 1024*1024;
                suffix = "MB";
            }
            else if (size > 1024)
            {
                size /= 1024;
                suffix = "KB";
            }

            ui->label_recordingMetaFileSize->setEnabled(true);
            ui->label_recordingMetaFileSize->setText(QString("%1 %2").arg(QString::number(size, 'f', 2)).arg(suffix));
        }

        ui->label_recordingMetaFramerate->setEnabled(true);
        ui->label_recordingMetaFramerate->setText(QString::number(krecord_recording_framerate(), 'f', 2));

        ui->label_recordingMetaTargetFramerate->setEnabled(true);
        ui->label_recordingMetaTargetFramerate->setText(QString::number(krecord_playback_framerate()));

        const resolution_s resolution = krecord_video_resolution();
        ui->label_recordingMetaResolution->setEnabled(true);
        ui->label_recordingMetaResolution->setText(QString("%1 x %2").arg(resolution.w)
                                                                     .arg(resolution.h));

        const uint totalDuration = (((1000.0/krecord_playback_framerate()) * krecord_num_frames_recorded()) / 1000);
        const uint seconds = totalDuration % 60;
        const uint minutes = totalDuration / 60;
        const uint hours = minutes / 60;
        ui->label_recordingMetaDuration->setEnabled(true);
        ui->label_recordingMetaDuration->setText(QString("%1:%2:%3").arg(QString::number(hours).rightJustified(2, '0'))
                                                                    .arg(QString::number(minutes).rightJustified(2, '0'))
                                                                    .arg(QString::number(seconds).rightJustified(2, '0')));
    }
    else
    {
        ui->label_recordingMetaResolution->setText("n/a");
        ui->label_recordingMetaResolution->setEnabled(false);

        ui->label_recordingMetaDuration->setText("n/a");
        ui->label_recordingMetaDuration->setEnabled(false);

        ui->label_recordingMetaFileSize->setText("n/a");
        ui->label_recordingMetaFileSize->setEnabled(false);

        ui->label_recordingMetaFramerate->setText("n/a");
        ui->label_recordingMetaFramerate->setEnabled(false);

        ui->label_recordingMetaTargetFramerate->setText("n/a");
        ui->label_recordingMetaTargetFramerate->setEnabled(false);
    }

    return;
}

// Applies the x264 codec settings from VCS's GUI into the Windows registry, from
// where the codec can pick them up when it starts. On Linux, no settings need be
// written.
//
bool RecordDialog::apply_x264_registry_settings(void)
{
#if _WIN32
    const auto open_x264_registry = []()->HKEY
    {
        HKEY key;

        if (RegOpenKeyExA(HKEY_CURRENT_USER,
                          "Software\\GNU\\x264",
                          0,
                          (KEY_QUERY_VALUE | KEY_SET_VALUE),
                          &key) != ERROR_SUCCESS)
        {
            return nullptr;
        }

        return key;
    };

    const auto close_x264_registry = [](HKEY regKey)->bool
    {
        return bool(RegCloseKey(regKey) == ERROR_SUCCESS);
    };

    const auto set_x264_registry_value = [](const HKEY key,
                                            const char *const valueName,
                                            const int value)->bool
    {
        return (RegSetValueExA(key, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(value)) == ERROR_SUCCESS);
    };

    const auto set_x264_registry_string = [](const HKEY key,
                                             const char *const stringName,
                                             const char *const string)->bool
    {
        return (RegSetValueExA(key, stringName, 0, REG_SZ, (const BYTE*)string, (strlen(string) + 1)) == ERROR_SUCCESS);
    };

    const HKEY reg = open_x264_registry();
    if (reg == nullptr)
    {
        kd_show_headless_error_message("VCS can't start recording",
                                       "Can't access the 32-bit x264vfw video codec's registry information.\n\n"
                                       "If you've already installed the codec and are still getting this message, "
                                       "run its configurator once, and try again.");
        return false;
    }

    const uint colorSpace = [=]
    {
        const QString pixelFormat = ui->comboBox_recordingEncoderPixelFormat->currentText();
        if (pixelFormat == "YUV 4:2:0") return 0;
        else if (pixelFormat == "RGB") return 4;
        else k_assert(0, "Unrecognized x264 pixel format name.");

        return 0;
    }();

    const QString profileString = [=]
    {
        const QString profile = ui->comboBox_recordingEncoderProfile->currentText();
        if (profile == "High 4:4:4" || colorSpace == 4) return "high444";
        else if (profile == "High") return "high";
        else if (profile == "Main") return "main";
        else if (profile == "Baseline") return "baseline";
        else k_assert(0, "Unrecognized x264 profile name.");

        return "unknown";
    }();

    const QString presetString = ui->comboBox_recordingEncoderPreset->currentText().toLower();

    const QString commandLineString = ui->lineEdit_recordingEncoderArguments->text();

    if (!set_x264_registry_string(reg, "profile", profileString.toStdString().c_str()) ||
        !set_x264_registry_string(reg, "preset", presetString.toStdString().c_str()) ||
        !set_x264_registry_string(reg, "extra_cmdline", commandLineString.toStdString().c_str()) ||
        !set_x264_registry_value(reg, "output_mode", 0) || // Always output via the VFW interface.
        !set_x264_registry_value(reg, "colorspace", colorSpace) ||
        !set_x264_registry_value(reg, "zerolatency", ui->checkBox_recordingEncoderZeroLatency->isChecked()) ||
        !set_x264_registry_value(reg, "ratefactor", (ui->spinBox_recordingEncoderCRF->value() * 10)))
    {
        return false;
    }

    close_x264_registry(reg);
#endif

    return true;
}
