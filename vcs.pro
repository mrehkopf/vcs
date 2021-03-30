# Whether this build of VCS is intended for release (defined) or is developmental,
# non-stable etc. (undefined/commented out).
#DEFINES += RELEASE_BUILD

# Comment out to disable OpenCV. You'll have no filtering or scaler, but you also
# don't need to provide the dependencies.
DEFINES += USE_OPENCV

linux {
    DEFINES += CAPTURE_API_VIDEO4LINUX

    # The base path for Datapath's Linux Vision driver header files. These are
    # bundled with the driver downloadable from Datapath's website. The files
    # are expected to be in a visionrgb subdirectory of this path e.g.
    # .../visionrgb/include/rgb133control.h.
    INCLUDEPATH += $$(HOME)/sdk/ \
                   /usr/local/include/ \

    contains(DEFINES, USE_OPENCV) {
        LIBS += -L /usr/local/lib/ -lopencv_imgproc -lopencv_videoio -lopencv_highgui -lopencv_core -lopencv_photo
    }

    QMAKE_RPATHDIR += /usr/local/lib/ \
}

win32 {
    DEFINES += CAPTURE_API_RGBEASY

    INCLUDEPATH += "C:/VisionSDK/RGBEASY 1.0/INCLUDE"
    LIBS += "C:/VisionSDK/RGBEASY 1.0/LIB/Win32/Release/RGBEASY.lib"

    contains(DEFINES, USE_OPENCV) {
        INCLUDEPATH += "C:/OpenCV/3.2.0/include"
        LIBS += -L"C:/OpenCV/3.2.0/bin/mingw"
        LIBS += -lopencv_world320
    }

    RC_ICONS = "src/display/qt/images/icons/appicon.ico"
}

QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = vcs
TEMPLATE = app
CONFIG += console c++11

OBJECTS_DIR = generated_files
RCC_DIR = generated_files
MOC_DIR = generated_files
UI_DIR = generated_files

INCLUDEPATH += $$PWD/src/ \
               $$PWD/src/display/qt/subclasses/

RESOURCES += \
    src/display/qt/res.qrc

SOURCES += \
    src/display/qt/subclasses/QComboBox_video_preset_list.cpp \
    src/display/qt/subclasses/QDialog_vcs_base_dialog.cpp \
    src/display/qt/subclasses/QFrame_filtergui_for_qt.cpp \
    src/display/qt/subclasses/QGroupBox_parameter_grid.cpp \
    src/display/qt/subclasses/QMenu_dialog_file_menu.cpp \
    src/display/qt/windows/output_window.cpp \
    src/display/qt/dialogs/resolution_dialog.cpp \
    src/display/qt/d_main.cpp \
    src/display/qt/dialogs/overlay_dialog.cpp \
    src/display/qt/dialogs/alias_dialog.cpp \
    src/display/qt/dialogs/anti_tear_dialog.cpp \
    src/anti_tear/anti_tear_multiple_per_frame.cpp \
    src/anti_tear/anti_tear_one_per_frame.cpp \
    src/anti_tear/anti_tearer.cpp \
    src/filter/filters/blur/filter_blur.cpp \
    src/filter/filters/blur/gui/filtergui_blur.cpp \
    src/filter/filters/crop/filter_crop.cpp \
    src/filter/filters/crop/gui/filtergui_crop.cpp \
    src/filter/filters/decimate/filter_decimate.cpp \
    src/filter/filters/decimate/gui/filtergui_decimate.cpp \
    src/filter/filters/delta_histogram/filter_delta_histogram.cpp \
    src/filter/filters/delta_histogram/gui/filtergui_delta_histogram.cpp \
    src/filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.cpp \
    src/filter/filters/denoise_nonlocal_means/gui/filtergui_denoise_nonlocal_means.cpp \
    src/filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.cpp \
    src/filter/filters/denoise_pixel_gate/gui/filtergui_denoise_pixel_gate.cpp \
    src/filter/filtergui.cpp \
    src/filter/filters/flip/filter_flip.cpp \
    src/filter/filters/flip/gui/filtergui_flip.cpp \
    src/filter/filters/frame_rate/filter_frame_rate.cpp \
    src/filter/filters/frame_rate/gui/filtergui_frame_rate.cpp \
    src/filter/filters/input_gate/filter_input_gate.cpp \
    src/filter/filters/input_gate/gui/filtergui_input_gate.cpp \
    src/filter/filters/kernel_3x3/filter_kernel_3x3.cpp \
    src/filter/filters/kernel_3x3/gui/filtergui_kernel_3x3.cpp \
    src/filter/filters/median/filter_median.cpp \
    src/filter/filters/median/gui/filtergui_median.cpp \
    src/filter/filters/output_gate/filter_output_gate.cpp \
    src/filter/filters/output_gate/gui/filtergui_output_gate.cpp \
    src/filter/filters/rotate/filter_rotate.cpp \
    src/filter/filters/rotate/gui/filtergui_rotate.cpp \
    src/filter/filters/sharpen/filter_sharpen.cpp \
    src/filter/filters/sharpen/gui/filtergui_sharpen.cpp \
    src/filter/filters/unsharp_mask/filter_unsharp_mask.cpp \
    src/filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.cpp \
    src/scaler/scaler.cpp \
    src/main.cpp \
    src/common/log/log.cpp \
    src/filter/filter.cpp \
    src/common/command_line/command_line.cpp \
    src/capture/capture.cpp \
    src/anti_tear/anti_tear.cpp \
    src/display/qt/persistent_settings.cpp \
    src/common/memory/memory.cpp \
    src/record/record.cpp \
    src/common/disk/disk.cpp \
    src/capture/alias.cpp \
    src/display/qt/subclasses/QOpenGLWidget_opengl_renderer.cpp \
    src/display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.cpp \
    src/display/qt/subclasses/QGraphicsScene_interactible_node_graph.cpp \
    src/display/qt/subclasses/QGraphicsView_interactible_node_graph_view.cpp \
    src/display/qt/dialogs/filter_graph_dialog.cpp \
    src/display/qt/dialogs/about_dialog.cpp \
    src/display/qt/dialogs/record_dialog.cpp \
    src/display/qt/dialogs/output_resolution_dialog.cpp \
    src/display/qt/dialogs/input_resolution_dialog.cpp \
    src/capture/capture_api_virtual.cpp \
    src/capture/capture_api_rgbeasy.cpp \
    src/capture/capture_api.cpp \
    src/display/qt/subclasses/QTableWidget_property_table.cpp \
    src/display/qt/dialogs/signal_dialog.cpp \
    src/capture/capture_api_video4linux.cpp \
    src/common/disk/file_writers/file_writer_filter_graph_version_b.cpp \
    src/common/disk/file_reader.cpp \
    src/common/disk/file_readers/file_reader_filter_graph_version_b.cpp \
    src/common/disk/file_readers/file_reader_filter_graph_version_a.cpp \
    src/common/disk/file_writers/file_writer_aliases_version_a.cpp \
    src/common/disk/file_readers/file_reader_aliases_version_a.cpp \
    src/common/disk/file_readers/file_reader_video_params_legacy_1_6_5.cpp \
    src/common/disk/file_writers/file_writer_video_params_legacy_1_6_5.cpp \
    src/common/disk/file_writer.cpp \
    src/common/disk/file_writers/file_writer_video_params_version_a.cpp \
    src/common/disk/file_readers/file_reader_video_params_version_a.cpp \
    src/common/disk/file_readers/file_reader_aliases_legacy_1_6_5.cpp \
    src/display/qt/dialogs/filter_graph/output_gate_node.cpp \
    src/display/qt/dialogs/filter_graph/input_gate_node.cpp \
    src/display/qt/dialogs/filter_graph/filter_node.cpp \
    src/display/qt/dialogs/filter_graph/filter_graph_node.cpp \
    src/display/qt/dialogs/video_parameter_dialog.cpp \
    src/capture/video_presets.cpp \
    src/common/disk/file_writers/file_writer_video_presets_version_a.cpp \
    src/common/disk/file_readers/file_reader_video_presets_version_a.cpp \
    src/common/propagate/app_events.cpp \
    src/capture/input_channel_v4l.cpp \
    src/capture/ic_v4l_video_parameters.cpp \
    src/record/recording_buffer.cpp \
    src/record/framerate_estimator.cpp \
    src/common/timer/timer.cpp \
    src/display/qt/dialogs/linux_device_selector_dialog.cpp

HEADERS += \
    src/common/globals.h \
    src/capture/null_rgbeasy.h \
    src/common/types.h \
    src/display/qt/subclasses/QComboBox_video_preset_list.h \
    src/display/qt/subclasses/QDialog_vcs_base_dialog.h \
    src/display/qt/subclasses/QFrame_filtergui_for_qt.h \
    src/display/qt/subclasses/QGroupBox_parameter_grid.h \
    src/display/qt/subclasses/QMenu_dialog_file_menu.h \
    src/display/qt/windows/output_window.h \
    src/display/qt/dialogs/resolution_dialog.h \
    src/anti_tear/anti_tear_multiple_per_frame.h \
    src/anti_tear/anti_tear_one_per_frame.h \
    src/anti_tear/anti_tearer.h \
    src/filter/filters/blur/filter_blur.h \
    src/filter/filters/blur/gui/filtergui_blur.h \
    src/filter/filters/crop/filter_crop.h \
    src/filter/filters/crop/gui/filtergui_crop.h \
    src/filter/filters/decimate/filter_decimate.h \
    src/filter/filters/decimate/gui/filtergui_decimate.h \
    src/filter/filters/delta_histogram/filter_delta_histogram.h \
    src/filter/filters/delta_histogram/gui/filtergui_delta_histogram.h \
    src/filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.h \
    src/filter/filters/denoise_nonlocal_means/gui/filtergui_denoise_nonlocal_means.h \
    src/filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h \
    src/filter/filters/denoise_pixel_gate/gui/filtergui_denoise_pixel_gate.h \
    src/filter/filtergui.h \
    src/filter/filters/filters.h \
    src/filter/filters/flip/filter_flip.h \
    src/filter/filters/flip/gui/filtergui_flip.h \
    src/filter/filters/frame_rate/filter_frame_rate.h \
    src/filter/filters/frame_rate/gui/filtergui_frame_rate.h \
    src/filter/filters/input_gate/filter_input_gate.h \
    src/filter/filters/input_gate/gui/filtergui_input_gate.h \
    src/filter/filters/kernel_3x3/filter_kernel_3x3.h \
    src/filter/filters/kernel_3x3/gui/filtergui_kernel_3x3.h \
    src/filter/filters/median/filter_median.h \
    src/filter/filters/median/gui/filtergui_median.h \
    src/filter/filters/output_gate/filter_output_gate.h \
    src/filter/filters/output_gate/gui/filtergui_output_gate.h \
    src/filter/filters/rotate/filter_rotate.h \
    src/filter/filters/rotate/gui/filtergui_rotate.h \
    src/filter/filters/sharpen/filter_sharpen.h \
    src/filter/filters/sharpen/gui/filtergui_sharpen.h \
    src/filter/filters/unsharp_mask/filter_unsharp_mask.h \
    src/filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.h \
    src/scaler/scaler.h \
    src/capture/capture.h \
    src/display/display.h \
    src/common/log/log.h \
    src/display/qt/dialogs/overlay_dialog.h \
    src/display/qt/dialogs/alias_dialog.h \
    src/anti_tear/anti_tear.h \
    src/display/qt/dialogs/anti_tear_dialog.h \
    src/filter/filter.h \
    src/common/command_line/command_line.h \
    src/display/qt/persistent_settings.h \
    src/display/qt/utility.h \
    src/common/disk/csv.h \
    src/common/memory/memory.h \
    src/common/memory/memory_interface.h \
    src/record/record.h \
    src/common/disk/disk.h \
    src/capture/alias.h \
    src/display/qt/subclasses/QOpenGLWidget_opengl_renderer.h \
    src/display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h \
    src/display/qt/subclasses/QGraphicsScene_interactible_node_graph.h \
    src/display/qt/subclasses/QGraphicsView_interactible_node_graph_view.h \
    src/display/qt/dialogs/filter_graph_dialog.h \
    src/display/qt/dialogs/about_dialog.h \
    src/display/qt/dialogs/record_dialog.h \
    src/display/qt/dialogs/output_resolution_dialog.h \
    src/display/qt/dialogs/input_resolution_dialog.h \
    src/capture/capture_api.h \
    src/capture/capture_api_virtual.h \
    src/capture/capture_api_rgbeasy.h \
    src/display/display.h \
    src/display/qt/subclasses/QTableWidget_property_table.h \
    src/display/qt/dialogs/signal_dialog.h \
    src/capture/capture_api_video4linux.h \
    src/common/disk/file_writers/file_writer_filter_graph.h \
    src/common/disk/file_readers/file_reader_filter_graph.h \
    src/common/disk/file_reader.h \
    src/common/disk/file_streamer.h \
    src/common/disk/file_writers/file_writer_aliases.h \
    src/common/disk/file_readers/file_reader_video_params.h \
    src/common/disk/file_writers/file_writer_video_params.h \
    src/common/disk/file_writer.h \
    src/display/qt/dialogs/filter_graph/output_gate_node.h \
    src/display/qt/dialogs/filter_graph/input_gate_node.h \
    src/display/qt/dialogs/filter_graph/filter_node.h \
    src/display/qt/dialogs/filter_graph/filter_graph_node.h \
    src/display/qt/dialogs/video_parameter_dialog.h \
    src/common/refresh_rate.h \
    src/capture/video_presets.h \
    src/common/disk/file_writers/file_writer_video_presets.h \
    src/common/disk/file_readers/file_reader_video_presets.h \
    src/common/propagate/app_events.h \
    src/capture/input_channel_v4l.h \
    src/capture/ic_v4l_video_parameters.h \
    src/record/recording_buffer.h \
    src/record/recording_meta.h \
    src/record/framerate_estimator.h \
    src/common/timer/timer.h \
    src/display/qt/dialogs/linux_device_selector_dialog.h

FORMS += \
    src/display/qt/windows/ui/output_window.ui \
    src/display/qt/dialogs/ui/overlay_dialog.ui \
    src/display/qt/dialogs/ui/resolution_dialog.ui \
    src/display/qt/dialogs/ui/alias_dialog.ui \
    src/display/qt/dialogs/ui/anti_tear_dialog.ui \
    src/display/qt/dialogs/ui/filter_graph_dialog.ui \
    src/display/qt/dialogs/ui/about_dialog.ui \
    src/display/qt/dialogs/ui/record_dialog.ui \
    src/display/qt/dialogs/ui/output_resolution_dialog.ui \
    src/display/qt/dialogs/ui/input_resolution_dialog.ui \
    src/display/qt/dialogs/ui/signal_dialog.ui \
    src/display/qt/dialogs/ui/video_parameter_dialog.ui \
    src/display/qt/dialogs/ui/linux_device_selector_dialog.ui

# C++. For GCC/Clang/MinGW.
QMAKE_CXXFLAGS += -g
QMAKE_CXXFLAGS += -O2
QMAKE_CXXFLAGS += -Wall
QMAKE_CXXFLAGS += -pipe
QMAKE_CXXFLAGS += -pedantic
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -Wno-missing-field-initializers
