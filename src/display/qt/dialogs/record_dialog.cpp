/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * The widget that makes up the control panel's 'Record' tab.
 *
 */

#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include "display/qt/dialogs/record_dialog.h"
#include "display/qt/persistent_settings.h"
#include "common/propagate/app_events.h"
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

    this->setWindowTitle("VCS - Video Recorder");

    // Don't show the context help '?' button in the window bar.
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    this->timerUpdateRecordingInfo = new QTimer(this);
    connect(this->timerUpdateRecordingInfo, &QTimer::timeout,
            this, [=]{this->update_recording_metainfo();});

    // Initialize the table of information. Note that this also sets
    // the vertical order in which the table's parameters are shown.
    {
        ui->tableWidget_status->modify_property("Frames recorded", "-");
        ui->tableWidget_status->modify_property("Frames dropped", "-");
        ui->tableWidget_status->modify_property("Playback duration", "-");
        ui->tableWidget_status->modify_property("Resolution", "-");
        ui->tableWidget_status->modify_property("File size", "-");
        ui->tableWidget_status->modify_property("Peak buffer usage", "-");

        ui->groupBox_status->setEnabled(false);
    }

    // Certain features of recording are not available on certain operating systems;
    // so disable them accordingly.
    {
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

        // Disable recording settings not available under Linux. (To customize them,
        // you'll need to edit the relevant OpenCV source code and recompile it;
        // e.g. https://www.researchgate.net/post/Is_it_possible_to_set_the_lossfree_option_for_the_X264_codec_in_OpenCV).
        {
            #if __linux__
                ui->comboBox_recordingEncoderProfile->setVisible(false);
                ui->comboBox_recordingEncoderPixelFormat->setVisible(false);
                ui->comboBox_recordingEncoderProfile->setVisible(false);
                ui->comboBox_recordingEncoderPreset->setVisible(false);
                ui->lineEdit_recordingEncoderArguments->setVisible(false);
                ui->spinBox_recordingEncoderCRF->setVisible(false);
                ui->comboBox_recordingEncoderZeroLatency->setVisible(false);

                ui->label->setVisible(false);
                ui->label_22->setVisible(false);
                ui->label_15->setVisible(false);
                ui->label_24->setVisible(false);
                ui->label_23->setVisible(false);
                ui->label_27->setVisible(false);

                ui->groupBox_recordingSettings->layout()->removeWidget(ui->label);
                ui->groupBox_recordingSettings->layout()->removeWidget(ui->label_22);
                ui->groupBox_recordingSettings->layout()->removeWidget(ui->label_15);
                ui->groupBox_recordingSettings->layout()->removeWidget(ui->label_24);
                ui->groupBox_recordingSettings->layout()->removeWidget(ui->label_23);
                ui->groupBox_recordingSettings->layout()->removeWidget(ui->label_27);

                ui->groupBox_recordingSettings->layout()->removeWidget(ui->comboBox_recordingEncoderProfile);
                ui->groupBox_recordingSettings->layout()->removeWidget(ui->comboBox_recordingEncoderPixelFormat);
                ui->groupBox_recordingSettings->layout()->removeWidget(ui->comboBox_recordingEncoderProfile);
                ui->groupBox_recordingSettings->layout()->removeWidget(ui->comboBox_recordingEncoderPreset);
                ui->groupBox_recordingSettings->layout()->removeWidget(ui->lineEdit_recordingEncoderArguments);
                ui->groupBox_recordingSettings->layout()->removeWidget(ui->spinBox_recordingEncoderCRF);
                ui->groupBox_recordingSettings->layout()->removeWidget(ui->comboBox_recordingEncoderZeroLatency);
            #endif
        }
    }

    // Create the dialog's menu bar.
    {
        this->menubar = new QMenuBar(this);

        // Record...
        {
            QMenu *recordMenu = new QMenu("Recorder", this->menubar);

            QAction *enable = new QAction("Enabled", this->menubar);
            enable->setCheckable(true);
            enable->setChecked(this->isEnabled);

            connect(this, &RecordDialog::recording_enabled, this, [=]
            {
                enable->setChecked(true);
                this->isEnabled = true;
            });

            connect(this, &RecordDialog::recording_disabled, this, [=]
            {
                enable->setChecked(false);
                this->isEnabled = false;
            });

            connect(this, &RecordDialog::recording_could_not_be_enabled, this, [=]
            {
                enable->setChecked(false);
            });

            connect(this, &RecordDialog::recording_could_not_be_disabled, this, [=]
            {
                enable->setChecked(true);
            });

            connect(enable, &QAction::triggered, this, [=]
            {
                this->set_recording_enabled(!this->isEnabled);
            });

            recordMenu->addAction(enable);

            this->menubar->addMenu(recordMenu);
        }

        this->layout()->setMenuBar(menubar);
    }

    // Connect the GUI controls to consequences for changing their values.
    {
        connect(ui->pushButton_startStopRecording, &QPushButton::clicked, this, [this]
        {
            // We'll set the button to disabled to prevent double-clicks.
            ui->pushButton_startStopRecording->setEnabled(false);

            if (krecord_is_recording())
            {
                this->set_recording_enabled(false);
                ui->groupBox_status->setEnabled(false);
            }
            else
            {
                this->set_recording_enabled(true);
                ui->groupBox_status->setEnabled(true);
            }

            ui->pushButton_startStopRecording->setEnabled(true);
        });

        connect(ui->pushButton_recordingSelectFilename, &QPushButton::clicked, this, [this]
        {
            QString filename = QFileDialog::getSaveFileName(this,
                                                            "Record into video", "",
                                                            "All files(*.*)");

            if (filename.isEmpty()) return;

            ui->lineEdit_recordingFilename->setText(filename);
        });
    }

    // Restore persistent settings.
    {
        ui->spinBox_recordingFramerate->setValue(kpers_value_of(INI_GROUP_RECORDING, "frame_rate", 60).toUInt());
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "record", this->size()).toSize());

        #if _WIN32
            set_qcombobox_idx_c(ui->comboBox_recordingEncoderProfile)
                               .by_string(kpers_value_of(INI_GROUP_RECORDING, "profile", "High 4:4:4").toString());

            set_qcombobox_idx_c(ui->comboBox_recordingEncoderPixelFormat)
                               .by_string(kpers_value_of(INI_GROUP_RECORDING, "pixel_format", "RGB").toString());

            set_qcombobox_idx_c(ui->comboBox_recordingEncoderPreset)
                               .by_string(kpers_value_of(INI_GROUP_RECORDING, "preset", "Superfast").toString());

            set_qcombobox_idx_c(ui->comboBox_recordingEncoderZeroLatency)
                               .by_string(kpers_value_of(INI_GROUP_RECORDING, "zero_latency", "Disabled").toString());

            ui->spinBox_recordingEncoderCRF->setValue(kpers_value_of(INI_GROUP_RECORDING, "crf", 1).toUInt());
            ui->lineEdit_recordingEncoderArguments->setText(kpers_value_of(INI_GROUP_RECORDING, "command_line", "").toString());
        #endif
    }

    // Subscribe to app events.
    {
        ke_events().recorder.recordingStarted.subscribe([this]
        {
            this->set_recording_controls_enabled(false);

            this->timerUpdateRecordingInfo->start(1000);

            ui->pushButton_startStopRecording->setText("Stop recording");

            if (!PROGRAM_EXIT_REQUESTED)
            {
                this->update_recording_metainfo();
            }
        });

        ke_events().recorder.recordingEnded.subscribe([this]
        {
            this->set_recording_controls_enabled(true);

            this->timerUpdateRecordingInfo->stop();

            ui->pushButton_startStopRecording->setText("Record");

            if (!PROGRAM_EXIT_REQUESTED)
            {
                this->update_recording_metainfo();
            }
        });
    }

    // Reset the metainfo values to their defaults.
    update_recording_metainfo();

    return;
}

RecordDialog::~RecordDialog()
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "record", this->size());

        // Encoder settings.
        {
            kpers_set_value(INI_GROUP_RECORDING, "frame_rate", ui->spinBox_recordingFramerate->value());

            #if _WIN32
                // These settings are only available in the Windows version of VCS.
                kpers_set_value(INI_GROUP_RECORDING, "profile", ui->comboBox_recordingEncoderProfile->currentText());
                kpers_set_value(INI_GROUP_RECORDING, "pixel_format", ui->comboBox_recordingEncoderPixelFormat->currentText());
                kpers_set_value(INI_GROUP_RECORDING, "preset", ui->comboBox_recordingEncoderPreset->currentText());
                kpers_set_value(INI_GROUP_RECORDING, "crf", ui->spinBox_recordingEncoderCRF->value());
                kpers_set_value(INI_GROUP_RECORDING, "zero_latency", ui->comboBox_recordingEncoderZeroLatency->currentIndex());
                kpers_set_value(INI_GROUP_RECORDING, "command_line", ui->lineEdit_recordingEncoderArguments->text());
            #endif
        }
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
    ui->groupBox_recordingSettings->setEnabled(areEnabled);
    ui->lineEdit_recordingFilename->setEnabled(areEnabled);
    ui->pushButton_recordingSelectFilename->setEnabled(areEnabled);

    return;
}

void RecordDialog::update_recording_metainfo(void)
{
    if (krecord_is_recording())
    {
        const uint totalDuration = (((1000.0/krecord_playback_framerate()) * krecord_num_frames_recorded()) / 1000);
        const uint seconds = totalDuration % 60;
        const uint minutes = totalDuration / 60;
        const uint hours = minutes / 60;
        ui->tableWidget_status->modify_property("Playback duration", QString("%1:%2:%3").arg(QString::number(hours).rightJustified(2, '0'))
                                                                                        .arg(QString::number(minutes).rightJustified(2, '0'))
                                                                                        .arg(QString::number(seconds).rightJustified(2, '0')));

        ui->tableWidget_status->modify_property("Resolution", QString("%1 x %2").arg(krecord_video_resolution().w)
                                                                                .arg(krecord_video_resolution().h));

        const uint fileBytesize = QFileInfo(krecord_video_filename().c_str()).size();
        {
            double size = fileBytesize;
            QString suffix = "B";

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

            ui->tableWidget_status->modify_property("File size", QString("%1 %2").arg(QString::number(size, 'f', 2)).arg(suffix));
        }

        ui->tableWidget_status->modify_property("Frames recorded", QString::number(krecord_num_frames_recorded()));

        ui->tableWidget_status->modify_property("Frames dropped", QString::number(krecord_num_frames_dropped()));

        ui->tableWidget_status->modify_property("Peak buffer usage", QString("%1%").arg(krecord_peak_buffer_usage_percent()));
    }
    else
    {
        ui->tableWidget_status->modify_property("Frames recorded", "-");
        ui->tableWidget_status->modify_property("Frames dropped", "-");
        ui->tableWidget_status->modify_property("Playback duration", "-");
        ui->tableWidget_status->modify_property("Resolution", "-");
        ui->tableWidget_status->modify_property("File size", "-");
        ui->tableWidget_status->modify_property("Peak buffer usage", "-");
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
        !set_x264_registry_value(reg, "zerolatency", ui->comboBox_recordingEncoderZeroLatency->currentIndex()) ||
        !set_x264_registry_value(reg, "ratefactor", (ui->spinBox_recordingEncoderCRF->value() * 10)))
    {
        return false;
    }

    close_x264_registry(reg);
#endif

    return true;
}

void RecordDialog::set_recording_enabled(const bool enabled)
{
    if (!enabled)
    {
        krecord_stop_recording();

        if (!krecord_is_recording())
        {
            emit this->recording_disabled();
        }
        else
        {
            emit this->recording_could_not_be_disabled();
        }
    }
    // Start recording.
    else
    {
        // This could fail if the codec we want isn't available on the system.
        if (!apply_x264_registry_settings())
        {
            emit this->recording_could_not_be_enabled();
        }
        else
        {
            // Remove the existing output file, if any. Without this, the file size
            // reported in VCS's GUI during recording may be inaccurate on some
            // platforms.
            QFile(ui->lineEdit_recordingFilename->text()).remove();

            const resolution_s videoResolution = ks_output_resolution();

            krecord_start_recording(ui->lineEdit_recordingFilename->text().toStdString().c_str(),
                                    videoResolution.w,
                                    videoResolution.h,
                                    ui->spinBox_recordingFramerate->value());

            if (krecord_is_recording())
            {
                emit this->recording_enabled();
            }
            else
            {
                emit this->recording_could_not_be_enabled();
            }
        }
    }

    kd_update_output_window_title();

    return;
}

bool RecordDialog::is_recording_enabled(void)
{
    return this->isEnabled;
}
