/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS main qt window
 *
 * The program's main window. About half of what it does is display the filtered and
 * scaled captured frames, and the other half is relay messages from the non-ui parts
 * of the program to the control panel dialog, which needs to reflect changes in the
 * rest of the system (e.g. user-facing information about the current capture resolution).
 *
 */

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QTextDocument>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QFileDialog>
#include <QShortcut>
#include <QPainter>
#include <QScreen>
#include <QImage>
#include <QLabel>
#include <cmath>
#include "display/qt/subclasses/QOpenGLWidget_opengl_renderer.h"
#include "display/qt/subclasses/QDialog_vcs_base_dialog.h"
#include "display/qt/dialogs/linux_device_selector_dialog.h"
#include "display/qt/dialogs/output_resolution_dialog.h"
#include "display/qt/dialogs/video_parameter_dialog.h"
#include "display/qt/dialogs/input_resolution_dialog.h"
#include "display/qt/dialogs/signal_dialog.h"
#include "display/qt/dialogs/filter_graph_dialog.h"
#include "display/qt/dialogs/resolution_dialog.h"
#include "display/qt/dialogs/anti_tear_dialog.h"
#include "display/qt/dialogs/overlay_dialog.h"
#include "display/qt/dialogs/record_dialog.h"
#include "display/qt/windows/output_window.h"
#include "display/qt/dialogs/alias_dialog.h"
#include "display/qt/dialogs/about_dialog.h"
#include "display/qt/persistent_settings.h"
#include "anti_tear/anti_tear.h"
#include "common/propagate/vcs_event.h"
#include "capture/video_presets.h"
#include "capture/capture.h"
#include "capture/alias.h"
#include "common/globals.h"
#include "record/record.h"
#include "scaler/scaler.h"
#include "ui_output_window.h"

// For an optional OpenGL render surface.
static OGLWidget *OGL_SURFACE = nullptr;

static unsigned CURRENT_OUTPUT_FPS = 0;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowFlags(this->defaultWindowFlags);

    ui->centralwidget->setMouseTracking(true);
    ui->centralwidget->setFocusPolicy(Qt::StrongFocus);

    // Set up a layout for the central widget, so we can add the OpenGL
    // render surface to it when/if OpenGL is enabled.
    QVBoxLayout *const mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);

    // Set up the child dialogs.
    {
        outputResolutionDlg = new OutputResolutionDialog;
        inputResolutionDlg = new InputResolutionDialog;
        videoParamDlg = new VideoParameterDialog;
        filterGraphDlg = new FilterGraphDialog;
        antitearDlg = new AntiTearDialog;
        overlayDlg = new OverlayDialog;
        signalDlg = new SignalDialog;
        aliasDlg = new AliasDialog;
        aboutDlg = new AboutDialog;
        recordDlg = new RecordDialog;

        this->dialogs << outputResolutionDlg
                      << inputResolutionDlg
                      << filterGraphDlg
                      << videoParamDlg
                      << antitearDlg
                      << overlayDlg
                      << signalDlg
                      << aliasDlg
                      << aboutDlg
                      << recordDlg;
    }

    // Create the window's context menu.
    {
        this->contextMenu = new QMenu(this);

        QMenu *captureMenu = new QMenu("Input", this);
        {
            QMenu *dialogs = new QMenu("Dialogs", this);

            QMenu *channel = new QMenu("Channel", this);
            {
                #if __linux__
                    QAction *select = new QAction("Select...", this);
                    channel->addAction(select);
                    connect(select, &QAction::triggered, this, [this]
                    {
                        unsigned newIdx = kc_get_device_input_channel_idx();

                        if (LinuxDeviceSelectorDialog(&newIdx).exec() != QDialog::Rejected)
                        {
                            kc_set_capture_input_channel(newIdx);
                        }
                    });

                    channel->addSeparator();
                #else
                    QActionGroup *group = new QActionGroup(this);

                    for (int i = 0; i < kc_get_device_maximum_input_count(); i++)
                    {
                        QAction *inputChannel = new QAction(QString::number(i+1), this);
                        inputChannel->setActionGroup(group);
                        inputChannel->setCheckable(true);
                        channel->addAction(inputChannel);

                        if (i == int(INPUT_CHANNEL_IDX))
                        {
                            inputChannel->setChecked(true);
                        }

                        connect(inputChannel, &QAction::triggered, this, [=]{kc_set_input_channel(i);});
                    }
                #endif
            }

            QMenu *colorDepth = new QMenu("Color depth", this);
            {
                QActionGroup *group = new QActionGroup(this);

                QAction *c24 = new QAction("24-bit (RGB 888)", this);
                c24->setActionGroup(group);
                c24->setCheckable(true);
                c24->setChecked(true);
                colorDepth->addAction(c24);

                // Note: the Video4Linux capture API currently supports no other capture
                // color format besides RGB888.
                #ifndef CAPTURE_DEVICE_VISION_V4L
                    QAction *c16 = new QAction("16-bit (RGB 565)", this);
                    c16->setActionGroup(group);
                    c16->setCheckable(true);
                    colorDepth->addAction(c16);

                    QAction *c15 = new QAction("15-bit (RGB 555)", this);
                    c15->setActionGroup(group);
                    c15->setCheckable(true);
                    colorDepth->addAction(c15);

                    connect(c16, &QAction::triggered, this, [=]{kc_set_capture_pixel_format(capture_pixel_format_e::rgb_565);});
                    connect(c15, &QAction::triggered, this, [=]{kc_set_capture_pixel_format(capture_pixel_format_e::rgb_555);});
                #endif
                connect(c24, &QAction::triggered, this, [=]{kc_set_capture_pixel_format(capture_pixel_format_e::rgb_888);});
            }

            QMenu *deinterlacing = new QMenu("De-interlacing", this);
            {
                deinterlacing->setEnabled(kc_device_supports_deinterlacing());

                QActionGroup *group = new QActionGroup(this);

                QAction *weave = new QAction("Weave", this);
                weave->setActionGroup(group);
                weave->setCheckable(true);
                deinterlacing->addAction(weave);

                QAction *bob = new QAction("Bob", this);
                bob->setActionGroup(group);
                bob->setCheckable(true);
                deinterlacing->addAction(bob);

                QAction *field0 = new QAction("Field 1", this);
                field0->setActionGroup(group);
                field0->setCheckable(true);
                deinterlacing->addAction(field0);

                QAction *field1 = new QAction("Field 2", this);
                field1->setActionGroup(group);
                field1->setCheckable(true);
                deinterlacing->addAction(field1);

                const auto set_mode = [=](const capture_deinterlacing_mode_e mode)
                {
                    switch (mode)
                    {
                        case capture_deinterlacing_mode_e::bob:
                        {
                            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::bob);
                            kpers_set_value(INI_GROUP_OUTPUT, "interlacing_mode", "Bob");
                            break;
                        }
                        case capture_deinterlacing_mode_e::weave:
                        {
                            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::weave);
                            kpers_set_value(INI_GROUP_OUTPUT, "interlacing_mode", "Weave");
                            break;
                        }
                        case capture_deinterlacing_mode_e::field_0:
                        {
                            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::field_0);
                            kpers_set_value(INI_GROUP_OUTPUT, "interlacing_mode", "Field 1");
                            break;
                        }
                        case capture_deinterlacing_mode_e::field_1:
                        {
                            kc_set_deinterlacing_mode(capture_deinterlacing_mode_e::field_1);
                            kpers_set_value(INI_GROUP_OUTPUT, "interlacing_mode", "Field 2");
                            break;
                        }
                        default: k_assert(0, "Unknown deinterlacing mode."); break;
                    }
                };

                connect(bob, &QAction::triggered, this, [=]{set_mode(capture_deinterlacing_mode_e::bob);});
                connect(weave, &QAction::triggered, this, [=]{set_mode(capture_deinterlacing_mode_e::weave);});
                connect(field0, &QAction::triggered, this, [=]{set_mode(capture_deinterlacing_mode_e::field_0);});
                connect(field1, &QAction::triggered, this, [=]{set_mode(capture_deinterlacing_mode_e::field_1);});

                // Activate the default setting.
                {
                    const QString defaultMode = kpers_value_of(INI_GROUP_OUTPUT, "interlacing_mode", "Weave").toString();

                    QAction *action = bob;

                    if      (defaultMode.toLower() == "bob") action = bob;
                    else if (defaultMode.toLower() == "weave") action = weave;
                    else if (defaultMode.toLower() == "field 1") action = field0;
                    else if (defaultMode.toLower() == "field 2") action = field1;

                    action->trigger();
                }
            }

            captureMenu->addMenu(channel);
            captureMenu->addMenu(colorDepth);
            captureMenu->addMenu(deinterlacing);
            captureMenu->addSeparator();
            captureMenu->addMenu(dialogs);

            QAction *aliases = new QAction("Aliases", this);
            dialogs->addAction(aliases);

            QAction *resolution = new QAction("Resolution", this);
            resolution->setShortcut(QKeySequence("ctrl+i"));
            dialogs->addAction(resolution);

            QAction *signal = new QAction("Signal info", this);
            signal->setShortcut(QKeySequence("ctrl+s"));
            dialogs->addAction(signal);

            QAction *videoParams = new QAction("Video presets", this);
            videoParams->setShortcut(QKeySequence("ctrl+v"));
            dialogs->addAction(videoParams);

            #if CAPTURE_DEVICE_VISION_V4L
                aliases->setEnabled(false);
                INFO(("Aliases are not supported with Video4Linux."));

                resolution->setEnabled(false);
                INFO(("Custom input resolutions are not supported with Video4Linux."));
            #endif

            connect(signal, &QAction::triggered, this, [=]{this->signalDlg->open();});
            connect(videoParams, &QAction::triggered, this, [=]{this->videoParamDlg->open();});
            connect(resolution, &QAction::triggered, this, [=]{this->inputResolutionDlg->open();});
            connect(aliases, &QAction::triggered, this, [=]{this->aliasDlg->open();});
        }

        QMenu *outputMenu = new QMenu("Output", this);
        {
            const std::vector<std::string> scalerNames = ks_list_of_scaling_filter_names();
            k_assert(!scalerNames.empty(), "Expected to receive a list of scalers, but got an empty list.");

            connect(outputMenu->addAction("Screenshot..."), &QAction::triggered, this, [this]
            {
                QString filename = QFileDialog::getSaveFileName(this,
                                                                "Save screenshot",
                                                                "",
                                                                "Image files (*.png *.jpeg *.bmp *.ppm)");

                if (QFileInfo(filename).suffix().isEmpty())
                {
                    filename.append(".png");
                }

                if (!filename.isEmpty())
                {
                    const QImage frameImage = ([]()->QImage
                    {
                        const captured_frame_s &frame = ks_frame_buffer();

                        k_assert((frame.pixelFormat == capture_pixel_format_e::rgb_888),
                                 "Expected frame pixel data to be 32-bit RGB.");

                        if (frame.pixels.is_null())
                        {
                            DEBUG(("Requested the scaler output as a QImage while the scaler's output buffer was uninitialized."));
                            return QImage();
                        }
                        else return QImage(frame.pixels.ptr(), frame.r.w, frame.r.h, QImage::Format_RGB32);
                    })();

                    if (frameImage.save(filename))
                    {
                        DEBUG(("Screenshotted into \"%s\".", filename.toStdString().c_str()));
                    }
                    else
                    {
                        DEBUG(("Could not screenshot into \"%s\". Possibly an unrecognized file format.",
                               filename.toStdString().c_str()));
                    }
                }

            });

            outputMenu->addSeparator();

            QMenu *upscaler = new QMenu("Upscaler", this);
            {
                QActionGroup *group = new QActionGroup(this);

                const QString defaultUpscalerName = kpers_value_of(INI_GROUP_OUTPUT, "upscaler", "Linear").toString();

                for (const auto &scalerName: scalerNames)
                {
                    QAction *scaler = new QAction(QString::fromStdString(scalerName), this);
                    scaler->setActionGroup(group);
                    scaler->setCheckable(true);
                    upscaler->addAction(scaler);

                    connect(scaler, &QAction::toggled, this, [=](const bool checked)
                    {
                        if (checked)
                        {
                            ks_set_upscaling_filter(scalerName);
                        }
                    });

                    if (QString::fromStdString(scalerName) == defaultUpscalerName)
                    {
                        scaler->setChecked(true);
                    }
                }
            }

            QMenu *downscaler = new QMenu("Downscaler", this);
            {
                QActionGroup *group = new QActionGroup(this);

                const QString defaultDownscalerName = kpers_value_of(INI_GROUP_OUTPUT, "downscaler", "Linear").toString();

                for (const auto &scalerName: scalerNames)
                {
                    QAction *scaler = new QAction(QString::fromStdString(scalerName), this);
                    scaler->setActionGroup(group);
                    scaler->setCheckable(true);
                    downscaler->addAction(scaler);

                    connect(scaler, &QAction::toggled, this, [=](const bool checked){if (checked) ks_set_downscaling_filter(scalerName);});


                    if (QString::fromStdString(scalerName) == defaultDownscalerName)
                    {
                        scaler->setChecked(true);
                    }
                }
            }

            QMenu *aspectRatio = new QMenu("Aspect ratio", this);
            {
                QActionGroup *group = new QActionGroup(this);

                QAction *native = new QAction("Native", this);
                native->setActionGroup(group);
                native->setCheckable(true);
                aspectRatio->addAction(native);

                QAction *always43 = new QAction("Always 4:3", this);
                always43->setActionGroup(group);
                always43->setCheckable(true);
                aspectRatio->addAction(always43);

                QAction *traditional43 = new QAction("Traditional 4:3", this);
                traditional43->setActionGroup(group);
                traditional43->setCheckable(true);
                aspectRatio->addAction(traditional43);

                const QString defaultAspectRatio = kpers_value_of(INI_GROUP_OUTPUT, "aspect_mode", "Native").toString();

                connect(native, &QAction::toggled, this,
                        [=](const bool checked){if (checked) { ks_set_forced_aspect_enabled(false); ks_set_aspect_mode(aspect_mode_e::native);}});
                connect(traditional43, &QAction::toggled, this,
                        [=](const bool checked){if (checked) { ks_set_forced_aspect_enabled(true); ks_set_aspect_mode(aspect_mode_e::traditional_4_3);}});
                connect(always43, &QAction::toggled, this,
                        [=](const bool checked){if (checked) { ks_set_forced_aspect_enabled(true); ks_set_aspect_mode(aspect_mode_e::always_4_3);}});

                if (defaultAspectRatio == "Native") native->setChecked(true);
                else if (defaultAspectRatio == "Always 4:3") always43->setChecked(true);
                else if (defaultAspectRatio == "Traditional 4:3") traditional43->setChecked(true);
            }

            QMenu *dialogs = new QMenu("Dialogs", this);

            outputMenu->addMenu(aspectRatio);
            outputMenu->addSeparator();
            outputMenu->addMenu(upscaler);
            outputMenu->addMenu(downscaler);
            outputMenu->addSeparator();
            outputMenu->addMenu(dialogs);

            QAction *overlay = new QAction("Overlay", this);
            overlay->setShortcut(QKeySequence("ctrl+l"));
            dialogs->addAction(overlay);
            connect(overlay, &QAction::triggered, this, [=]{this->overlayDlg->open();});

            QAction *antiTear = new QAction("Anti-tear", this);
            antiTear->setShortcut(QKeySequence("ctrl+a"));
            dialogs->addAction(antiTear);
            connect(antiTear, &QAction::triggered, this, [=]{this->antitearDlg->open();});

            QAction *resolution = new QAction("Resolution", this);
            resolution->setShortcut(QKeySequence("ctrl+o"));
            dialogs->addAction(resolution);
            connect(resolution, &QAction::triggered, this, [=]{this->outputResolutionDlg->open();});

            QAction *filter = new QAction("Filter graph", this);
            filter->setShortcut(QKeySequence("ctrl+f"));
            dialogs->addAction(filter);
            connect(filter, &QAction::triggered, this, [=]{this->filterGraphDlg->open();});

            QAction *record = new QAction("Video recorder", this);
            record->setShortcut(QKeySequence("ctrl+r"));
            dialogs->addAction(record);
            connect(record, &QAction::triggered, this, [=]{this->recordDlg->open();});
        }

        QMenu *windowMenu = new QMenu("Window", this);
        {
            {
                QMenu *rendererMenu = new QMenu("Renderer", this);

                QActionGroup *group = new QActionGroup(this);

                QAction *opengl = new QAction("OpenGL", this);
                opengl->setActionGroup(group);
                opengl->setCheckable(true);
                rendererMenu->addAction(opengl);

                QAction *software = new QAction("Software", this);
                software->setActionGroup(group);
                software->setCheckable(true);
                rendererMenu->addAction(software);

                connect(this, &MainWindow::entered_fullscreen, this, [=]
                {
                    rendererMenu->setEnabled(false);
                });

                connect(this, &MainWindow::left_fullscreen, this, [=]
                {
                    rendererMenu->setEnabled(true);
                });

                connect(opengl, &QAction::triggered, this, [=]
                {
                    this->set_opengl_enabled(true);
                });

                connect(software, &QAction::triggered, this, [=]
                {
                    this->set_opengl_enabled(false);
                });

                if (kpers_value_of(INI_GROUP_OUTPUT, "renderer", "Software").toString() == "Software")
                {
                    software->setChecked(true);
                }
                else
                {
                    opengl->setChecked(true);
                }

                windowMenu->addMenu(rendererMenu);
            }

            {
                windowMenu->addSeparator();

                QAction *showBorder = new QAction("Border", this);

                showBorder->setCheckable(true);
                showBorder->setChecked(this->window_has_border());
                showBorder->setShortcut(QKeySequence("f1"));

                connect(this, &MainWindow::border_hidden, this, [=]
                {
                    showBorder->setChecked(false);
                });

                connect(this, &MainWindow::border_revealed, this, [=]
                {
                    showBorder->setChecked(true);
                });

                connect(this, &MainWindow::entered_fullscreen, this, [=]
                {
                    showBorder->setEnabled(false);
                });

                connect(this, &MainWindow::left_fullscreen, this, [=]
                {
                    showBorder->setEnabled(true);
                });

                connect(showBorder, &QAction::triggered, this, [this]
                {
                    this->toggle_window_border();
                });

                windowMenu->addAction(showBorder);
            }

            {
                QAction *fullscreen = new QAction("Fullscreen", this);

                fullscreen->setCheckable(true);
                fullscreen->setChecked(this->isFullScreen());
                fullscreen->setShortcut(QKeySequence("f11"));

                connect(this, &MainWindow::entered_fullscreen, this, [=]
                {
                    fullscreen->setChecked(true);
                });

                connect(this, &MainWindow::left_fullscreen, this, [=]
                {
                    fullscreen->setChecked(false);
                });

                connect(fullscreen, &QAction::triggered, this, [this]
                {
                    if (this->isFullScreen())
                    {
                        this->showNormal();
                    }
                    else
                    {
                        this->showFullScreen();
                    }
                });

                windowMenu->addAction(fullscreen);
            }

            {
                QAction *center = new QAction("Center", this);

                QAction *topLeft = new QAction("Top left", this);
                topLeft->setShortcut(QKeySequence("f2"));

                connect(center, &QAction::triggered, this, [=]
                {
                    this->move(this->pos() + (QGuiApplication::primaryScreen()->geometry().center() - this->geometry().center()));
                });

                connect(topLeft, &QAction::triggered, this, [=]
                {
                    this->move(0, 0);
                });

                connect(this, &MainWindow::entered_fullscreen, this, [=]
                {
                    center->setEnabled(false);
                    topLeft->setEnabled(false);
                });

                connect(this, &MainWindow::left_fullscreen, this, [=]
                {
                    center->setEnabled(true);
                    topLeft->setEnabled(true);
                });

                windowMenu->addSeparator();
                windowMenu->addAction(center);
                windowMenu->addAction(topLeft);
            }

            {
                windowMenu->addSeparator();

                QAction *customTitle = new QAction("Set title...", this);

                connect(customTitle, &QAction::triggered, this, [=]
                {
                    const QString newTitle = QInputDialog::getText(this,
                                                                   "VCS - Enter a custom window title",
                                                                   "Title (empty restores the default):",
                                                                   QLineEdit::Normal,
                                                                   this->windowTitleOverride);

                    if (!newTitle.isNull())
                    {
                        this->windowTitleOverride = newTitle;
                        this->update_window_title();
                    }
                });

                connect(this, &MainWindow::entered_fullscreen, this, [=]
                {
                    customTitle->setEnabled(false);
                });

                connect(this, &MainWindow::left_fullscreen, this, [=]
                {
                    customTitle->setEnabled(true);
                });

                windowMenu->addAction(customTitle);
            }
        }

        QAction *about = new QAction("About...", this);
        {
            connect(about, &QAction::triggered, this, [=]{this->aboutDlg->open();});
        }

        this->contextMenuEyedropper = new QAction("0, 0, 0", this);
        this->contextMenuEyedropper->setEnabled(false);
        this->contextMenuEyedropper->setIcon(QIcon(":/res/images/icons/newie/eyedropper.png"));

        this->contextMenu->addAction(this->contextMenuEyedropper);
        this->contextMenu->addSeparator();
        this->contextMenu->addMenu(captureMenu);
        this->contextMenu->addMenu(outputMenu);
        this->contextMenu->addSeparator();
        this->contextMenu->addMenu(windowMenu);
        this->contextMenu->addSeparator();
        this->contextMenu->addAction(about);

        // Ensure that action shortcuts can be used regardless of which dialog has focus.
        for (const auto *menu: {captureMenu, outputMenu, windowMenu})
        {
            this->addActions(menu->actions());

            for (auto dialog: this->dialogs)
            {
                dialog->addActions(menu->actions());
            }
        }
    }

    // We intend to repaint the entire window every time we update it, so ask for no automatic fill.
    this->setAttribute(Qt::WA_OpaquePaintEvent, true);

    this->update_window_size();
    this->update_window_title();
    this->set_keyboard_shortcuts();

#ifdef _WIN32
    /// Temp hack. On my system, need to toggle this or there'll be a 2-pixel
    /// gap between the frame and the bottom corner of the window.
    toggle_window_border();
    toggle_window_border();
#endif

    this->activateWindow();
    this->raise();

    // Check over the network whether there are updates available for VCS.
    #ifdef RELEASE_BUILD
    {
        QNetworkAccessManager *network = new QNetworkAccessManager(this);
        connect(network, &QNetworkAccessManager::finished, [=](QNetworkReply *reply)
        {
            if (reply->error() == QNetworkReply::NoError)
            {
                bool isNewVersionAvailable = reply->readAll().trimmed().toInt();

                if (isNewVersionAvailable)
                {
                    INFO(("A newer version of VCS is available."));

                    if (this->aboutDlg)
                    {
                        this->aboutDlg->notify_of_new_program_version();
                    }
                }
            }
            else
            {
                /// TODO. Handle the error.
                return;
            }
        });

        // Make the request.
        const QString url = QString("http://www.tarpeeksihyvaesoft.com/vcs/is_newer_version_available.php?uv=%1").arg(PROGRAM_VERSION_STRING);
        network->get(QNetworkRequest(QUrl(url)));
    }
    #endif

    // Apply any custom styling.
    {
        qApp->setWindowIcon(QIcon(":/res/images/icons/appicon.ico"));
        this->apply_programwide_styling(":/res/stylesheets/appstyle-newie.qss");
    }

    // Listen for app events.
    {
        ks_evNewScaledFrame.listen([this]
        {
            this->redraw();
        });

        ks_evFramesPerSecond.listen([this](const unsigned fps)
        {
            if (CURRENT_OUTPUT_FPS != fps)
            {
                CURRENT_OUTPUT_FPS = fps;
                this->update_window_title();
            }
        });

        ks_evNewScaledFrameResolution.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
        });

        kc_evSignalLost.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
            this->redraw();
        });

        kc_evInvalidDevice.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
            this->redraw();
        });

        kc_evInvalidSignal.listen([this]
        {
            this->update_window_title();
            this->update_window_size();
            this->redraw();
        });

        kc_evNewVideoMode.listen([this](capture_video_mode_s)
        {
            this->update_window_title();
        });

        krecord_evRecordingStarted.listen([this]
        {
            if (!PROGRAM_EXIT_REQUESTED)
            {
                this->update_window_title();
            }
        });

        krecord_evRecordingEnded.listen([this]
        {
            if (!PROGRAM_EXIT_REQUESTED)
            {
                this->update_window_title();

                // The output resolution might have changed while we were recording, but
                // since we also prevent the size of the output window from changing while
                // recording, we should now - that recording has stopped - tell the window
                // to update its size to match the current proper output size.
                this->update_window_size();
            }
        });

        kc_evMissedFramesCount.listen([this](const unsigned numMissed)
        {
            const bool areDropped = (numMissed > 0);

            if (this->areFramesBeingDropped != areDropped)
            {
                this->areFramesBeingDropped = areDropped;
                this->update_window_title();
            }
        });
    }

    return;
}

void MainWindow::update_context_menu_eyedropper(const QPoint &scalerOutputPos)
{
    const captured_frame_s &frame = ks_frame_buffer();

    if (((unsigned long)scalerOutputPos.x() >= frame.r.w) ||
        ((unsigned long)scalerOutputPos.y() >= frame.r.h))
    {
        this->contextMenuEyedropper->setText("No eyedrop");
        return;
    }

    const unsigned idx = ((scalerOutputPos.x() + scalerOutputPos.y() * frame.r.w) * (frame.r.bpp / 8));

    const u8 red =   frame.pixels[idx + 2];
    const u8 green = frame.pixels[idx + 1];
    const u8 blue =  frame.pixels[idx + 0];

    this->contextMenuEyedropper->setText(QString("%1, %2, %3").arg(red).arg(green).arg(blue));

    return;
}

// Loads a QSS stylesheet from the given file, and assigns it to the entire
// program. Returns true if the file was successfully opened; false otherwise;
// will not signal whether actually assigning the stylesheet succeeded or not.
bool MainWindow::apply_programwide_styling(const QString &filename)
{
    // Take an empty filename to mean that all custom stylings should be removed.
    if (filename.isEmpty())
    {
        qApp->setStyleSheet("");

        return true;
    }

    // Apply the given style, prepended by an OS-specific font style.
    QFile styleFile(filename);
    #if _WIN32
        QFile defaultFontStyleFile(":/res/stylesheets/font-windows.qss");
    #else
        QFile defaultFontStyleFile(":/res/stylesheets/font-linux.qss");
    #endif
    if (styleFile.open(QIODevice::ReadOnly) &&
        defaultFontStyleFile.open(QIODevice::ReadOnly))
    {
        qApp->setStyleSheet(QString("%1 %2").arg(QString(styleFile.readAll()))
                                            .arg(QString(defaultFontStyleFile.readAll())));

        return true;
    }

    return false;
}

MainWindow::~MainWindow()
{
    // Save persistent settings.
    {
        const QString aspectMode = []()->QString
        {
            switch (ks_aspect_mode())
            {
                case aspect_mode_e::native: return "Native";
                case aspect_mode_e::always_4_3: return "Always 4:3";
                case aspect_mode_e::traditional_4_3: return "Traditional 4:3";
                default: return "(Unknown)";
            }
        }();

        kpers_set_value(INI_GROUP_OUTPUT, "aspect_mode", aspectMode);
        kpers_set_value(INI_GROUP_OUTPUT, "renderer", (OGL_SURFACE? "OpenGL" : "Software"));
        kpers_set_value(INI_GROUP_OUTPUT, "upscaler", QString::fromStdString(ks_upscaling_filter_name()));
        kpers_set_value(INI_GROUP_OUTPUT, "downscaler", QString::fromStdString(ks_downscaling_filter_name()));
    }

    delete ui;
    ui = nullptr;

    for (auto dialog: this->dialogs)
    {
        delete dialog;
        dialog = nullptr;
    }

    return;
}

bool MainWindow::load_font(const QString &filename)
{
    if (QFontDatabase::addApplicationFont(filename) == -1)
    {
        NBENE(("Failed to load the font '%s'.", filename.toStdString().c_str()));
        return false;
    }

    return true;
}

void MainWindow::redraw(void)
{
    if (OGL_SURFACE != nullptr)
    {
        OGL_SURFACE->repaint();
    }
    else this->repaint();

    return;
}

void MainWindow::set_opengl_enabled(const bool enabled)
{
    if (enabled)
    {
        INFO(("Renderer: OpenGL."));

        k_assert((OGL_SURFACE == nullptr), "Can't doubly enable OpenGL.");

        OGL_SURFACE = new OGLWidget(std::bind(&MainWindow::overlay_image, this), this);
        OGL_SURFACE->show();
        OGL_SURFACE->raise();

        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setVersion(1, 2);
        format.setSwapInterval(0); // Vsync.
        format.setSamples(0);
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
        format.setRenderableType(QSurfaceFormat::OpenGL);
        QSurfaceFormat::setDefaultFormat(format);

        ui->centralwidget->layout()->addWidget(OGL_SURFACE);
    }
    else
    {
        INFO(("Renderer: Software."));

        ui->centralwidget->layout()->removeWidget(OGL_SURFACE);
        delete OGL_SURFACE;
        OGL_SURFACE = nullptr;
    }

    return;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // The main loop will close the window if it detects PROGRAM_EXIT_REQUESTED.
    event->ignore();

    // If there are unsaved changes, ask the user to confirm to exit.
    {
        QStringList dialogsWithUnsavedChanges;

        for (const VCSBaseDialog *dialog: this->dialogs)
        {
            if (dialog->has_unsaved_changes())
            {
                dialogsWithUnsavedChanges << QString("* %1").arg(dialog->name());
            }
        }

        if (dialogsWithUnsavedChanges.count() &&
            (QMessageBox::question(this, "Confirm VCS exit",
                                   "The following dialogs have unsaved changes:"
                                   "<br><br>" + dialogsWithUnsavedChanges.join("<br>") +
                                   "<br><br>Exit anyway?",
                                   (QMessageBox::No | QMessageBox::Yes)) == QMessageBox::No))
        {
            return;
        }
    }

    PROGRAM_EXIT_REQUESTED = 1;

    return;
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        toggle_window_border();
    }

    return;
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        // Hide the window's cursor when in full-screen mode, and restore it when not.
        /// TODO: Ideally, this would only get set when we enter full-screen
        /// mode and back, not every time the window's state changes.
        if (this->windowState() & Qt::WindowFullScreen)
        {
            emit this->entered_fullscreen();
            this->setCursor(QCursor(Qt::BlankCursor));
        }
        else
        {
            emit this->left_fullscreen();
            this->unsetCursor();
        }
    }

    QWidget::changeEvent(event);
}

/// Temp. Used to track mouse movement delta across frames.
static bool LEFT_MOUSE_BUTTON_DOWN = false;
static QPoint PREV_MOUSE_POS;

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // If the cursor is over the capture window and the left mouse button is being
    // held down, drag the window. Note: We disable dragging in fullscreen mode,
    // since really you shouldn't be able to drag a fullscreen window.
    if (LEFT_MOUSE_BUTTON_DOWN &&
        !this->isFullScreen())
    {
        QPoint delta = (event->globalPos() - PREV_MOUSE_POS);
        this->move(pos() + delta);
    }

    PREV_MOUSE_POS = event->globalPos();

    return;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    ui->centralwidget->setFocus();

    if (event->button() == Qt::LeftButton)
    {
        LEFT_MOUSE_BUTTON_DOWN = true;
        PREV_MOUSE_POS = event->globalPos();
    }

    return;
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        LEFT_MOUSE_BUTTON_DOWN = false;
    }

    return;
}


void MainWindow::wheelEvent(QWheelEvent *event)
{
    if ((this->outputResolutionDlg != nullptr) &&
        this->is_mouse_wheel_scaling_allowed())
    {
        // Adjust the size of the capture window with the mouse scroll wheel.
        const int dir = (event->angleDelta().y() < 0)? 1 : -1;
        this->outputResolutionDlg->adjust_output_scaling(dir);
    }

    return;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Escape) &&
        this->isFullScreen())
    {
        this->showNormal();
    }

    return;
}

bool MainWindow::is_mouse_wheel_scaling_allowed(void)
{
    return (!kd_is_fullscreen() && // On my virtual machine, at least, wheel scaling while in full-screen messes up the full-screen mode.
            !krecord_is_recording());
}

QImage MainWindow::overlay_image(void)
{
    if (!kc_has_no_signal() &&
        overlayDlg != nullptr &&
        overlayDlg->is_enabled())
    {
        return overlayDlg->overlay_as_qimage();
    }
    else return QImage();
}

void MainWindow::paintEvent(QPaintEvent *)
{
    // If OpenGL is enabled, its own paintGL() should be getting called instead of paintEvent().
    if (OGL_SURFACE != nullptr) return;

    // Convert the output buffer into a QImage frame.
    const QImage frameImage = ([]()->QImage
    {
       const captured_frame_s &frame = ks_frame_buffer();

       k_assert((frame.pixelFormat == capture_pixel_format_e::rgb_888),
                "Expected frame pixel data to be 32-bit RGB.");

       if (frame.pixels.is_null())
       {
           DEBUG(("Requested the scaler output as a QImage while the scaler's output buffer was uninitialized."));
           return QImage();
       }
       else return QImage(frame.pixels.ptr(), frame.r.w, frame.r.h, QImage::Format_RGB32);
    })();

    QPainter painter(this);

    // Draw the frame.
    if (!frameImage.isNull())
    {
        painter.drawImage(0, 0, frameImage);
    }

    // Draw the overlay.
    const QImage overlayImg = overlay_image();
    if (!overlayImg.isNull())
    {
        painter.drawImage(0, 0, overlayImg);
    }

    // Show a magnifying glass effect which blows up part of the captured image.
    static QLabel *magnifyingGlass = nullptr;
    if (!kc_has_no_signal() &&
        this->isActiveWindow() &&
        this->rect().contains(this->mapFromGlobal(QCursor::pos())) &&
        (QGuiApplication::mouseButtons() & Qt::MidButton))
    {
        if (magnifyingGlass == nullptr)
        {
            magnifyingGlass = new QLabel(this);
            magnifyingGlass->setStyleSheet("background-color: rgba(0, 0, 0, 255);"
                                           "border-color: blue;"
                                           "border-style: solid;"
                                           "border-width: 3px;"
                                           "border-radius: 0px;");
        }

        const QSize magnifiedRegionSize = QSize(40, 30);
        const QSize glassSize = QSize(280, 210);
        const QPoint cursorPos = this->mapFromGlobal(QCursor::pos());
        QPoint regionTopLeft = QPoint((cursorPos.x() - (magnifiedRegionSize.width() / 2)),
                                      (cursorPos.y() - (magnifiedRegionSize.height() / 2)));

        // Don't let the magnification overflow the image buffer.
        if (regionTopLeft.x() < 0)
        {
            regionTopLeft.setX(0);
        }
        else if (regionTopLeft.x() > (frameImage.width() - magnifiedRegionSize.width()))
        {
            regionTopLeft.setX(frameImage.width() - magnifiedRegionSize.width());
        }
        if (regionTopLeft.y() < 0)
        {
            regionTopLeft.setY(0);
        }
        else if (regionTopLeft.y() > (frameImage.height() - magnifiedRegionSize.height()))
        {
            regionTopLeft.setY(frameImage.height() - magnifiedRegionSize.height());
        }

        // Grab the magnified region's pixel data.
        const u32 startIdx = (regionTopLeft.x() + regionTopLeft.y() * frameImage.width()) * (frameImage.depth() / 8);
        QImage magn(frameImage.bits() + startIdx, magnifiedRegionSize.width(), magnifiedRegionSize.height(),
                    frameImage.bytesPerLine(), frameImage.format());

        magnifyingGlass->resize(glassSize);
        magnifyingGlass->setPixmap(QPixmap::fromImage(magn.scaled(glassSize.width(), glassSize.height(),
                                                                  Qt::IgnoreAspectRatio, Qt::FastTransformation)));
        magnifyingGlass->move(std::min((this->width() - glassSize.width()), std::max(0, cursorPos.x() - (glassSize.width() / 2))),
                              std::max(0, std::min((this->height() - glassSize.height()), cursorPos.y() - (glassSize.height() / 2))));
        magnifyingGlass->show();
    }
    else
    {
        if (magnifyingGlass != nullptr &&
            magnifyingGlass->isVisible())
        {
            magnifyingGlass->setVisible(false);
        }
    }

    return;
}

void MainWindow::set_keyboard_shortcuts(void)
{
    // Creates a new QShortcut instance assigned to the given QKeySequence-
    // compatible sequence string (e.g. "F1" for the F1 key).
    auto keyboardShortcut = [this](const std::string keySequence)->QShortcut*
    {
        QShortcut *shortcut = new QShortcut(QKeySequence(keySequence.c_str()), this);

        shortcut->setContext(Qt::ApplicationShortcut);
        shortcut->setAutoRepeat(false);

        return shortcut;
    };

    // Assign ctrl + numeral key 1-9 to the input resolution force buttons.
    k_assert(this->inputResolutionDlg != nullptr, "");
    for (uint i = 1; i <= 9; i++)
    {
        connect(keyboardShortcut(QString("ctrl+%1").arg(QString::number(i)).toStdString()),
                &QShortcut::activated, [=]{this->inputResolutionDlg->activate_resolution_button(i);});
    }

    // Make Ctrl + Shift + <x> toggle the various dialogs' functionality on/off.
    connect(keyboardShortcut("ctrl+shift+f"), &QShortcut::activated, [=]{this->filterGraphDlg->set_enabled(!this->filterGraphDlg->is_enabled());});
    connect(keyboardShortcut("ctrl+shift+l"), &QShortcut::activated, [=]{this->overlayDlg->set_enabled(!this->overlayDlg->is_enabled());});
    connect(keyboardShortcut("ctrl+shift+a"), &QShortcut::activated, [=]{this->antitearDlg->set_enabled(!this->antitearDlg->is_enabled());});
    connect(keyboardShortcut("ctrl+shift+r"), &QShortcut::activated, [=]{this->recordDlg->set_enabled(!this->recordDlg->is_enabled());});

    // Ctrl + function keys maps to video presets.
    for (uint i = 1; i <= 12; i++)
    {
        const std::string shortcutString = QString("ctrl+f%1").arg(QString::number(i)).toStdString();

        connect(keyboardShortcut(shortcutString), &QShortcut::activated, this, [=]
        {
            kvideopreset_activate_keyboard_shortcut(shortcutString);
        });
    }

    // Shift + number assigns the current input channel.
    connect(keyboardShortcut("shift+1"), &QShortcut::activated, [=]
    {
        if (kc_get_device_input_channel_idx() != 0)
        {
            kc_set_capture_input_channel(0);
        }
    });
    connect(keyboardShortcut("shift+2"), &QShortcut::activated, [=]
    {
        if (kc_get_device_input_channel_idx() != 1)
        {
            kc_set_capture_input_channel(1);
        }
    });

    return;
}

void MainWindow::update_window_title(void)
{
    QString title = PROGRAM_NAME;

    if (PROGRAM_EXIT_REQUESTED)
    {
        title = QString("%1 - Closing...").arg(PROGRAM_NAME);
    }
    else if (kc_has_invalid_device())
    {
        title = QString("%1 - Invalid capture channel").arg(this->windowTitleOverride.isEmpty()? PROGRAM_NAME : this->windowTitleOverride);
    }
    else if (kc_has_invalid_signal())
    {
        title = QString("%1 - Signal out of range").arg(this->windowTitleOverride.isEmpty()? PROGRAM_NAME : this->windowTitleOverride);
    }
    else if (kc_has_no_signal())
    {
        title = QString("%1 - No signal").arg(this->windowTitleOverride.isEmpty()? PROGRAM_NAME : this->windowTitleOverride);
    }
    else
    {
        // A symbol shown in the title if VCS is currently dropping frames.
        const QString missedFramesMarker = "{!}";

        const resolution_s inRes = kc_get_capture_resolution();
        const resolution_s outRes = ks_scaler_output_resolution();
        const refresh_rate_s refreshRate = kc_get_capture_refresh_rate();

        QStringList programStatus;
        if (recordDlg->is_enabled())      programStatus << "R";
        if (filterGraphDlg->is_enabled()) programStatus << "F";
        if (overlayDlg->is_enabled())     programStatus << "O";
        if (antitearDlg->is_enabled())    programStatus << "A";

        if (!this->windowTitleOverride.isEmpty())
        {
            title = QString("%1%2")
                    .arg(this->areFramesBeingDropped? (missedFramesMarker + " ") : "")
                    .arg(this->windowTitleOverride);
        }
        else
        {
            title = QString("%1%2 - %3%4 \u00d7 %5 (%6 Hz) scaled to %7 \u00d7 %8 (%9 FPS)")
                    .arg(this->areFramesBeingDropped? (missedFramesMarker + " ") : "")
                    .arg(PROGRAM_NAME)
                    .arg(programStatus.count()? QString("%1 - ").arg(programStatus.join("")) : "")
                    .arg(inRes.w)
                    .arg(inRes.h)
                    .arg(QString::number(refreshRate.value<double>(), 'f', 3))
                    .arg(outRes.w)
                    .arg(outRes.h)
                    .arg(CURRENT_OUTPUT_FPS);
        }
    }

    this->setWindowTitle(title);

    return;
}

void MainWindow::update_gui_state()
{
    // Manually spin the event loop.
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents();

    return;
}

void MainWindow::update_window_size(void)
{
    const resolution_s r = ks_scaler_output_resolution();

    this->setFixedSize(r.w, r.h);
    overlayDlg->set_overlay_max_width(r.w);

    update_window_title();

    return;
}

bool MainWindow::window_has_border()
{
    return !bool(windowFlags() & Qt::FramelessWindowHint);
}

void MainWindow::toggle_window_border()
{
    if (this->isFullScreen())
    {
        return;
    }

    const Qt::WindowFlags borderlessFlags = (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    // Show the border.
    if (!window_has_border())
    {
        this->setWindowFlags(this->defaultWindowFlags & ~borderlessFlags);
        this->show();

        update_window_size();

        emit this->border_revealed();
    }
    // Hide the border.
    else
    {
        this->setWindowFlags(Qt::FramelessWindowHint | borderlessFlags);
        this->show();

        update_window_size();

        emit this->border_hidden();
    }
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    this->update_context_menu_eyedropper(this->mapFromGlobal(event->globalPos()));
    this->contextMenu->popup(event->globalPos());

    return;
}

void MainWindow::add_gui_log_entry(const log_entry_s e)
{
    /// GUI logging is currently not implemented.

    (void)e;

    return;
}
