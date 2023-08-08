# Whether this build of VCS is intended for release. Release builds may perform better
# but have fewer run-time error checks and contain less debug information.
#DEFINES += VCS_RELEASE_BUILD

linux {
    # Select one, depending on whether you're aiming to have VCS run on X11 or Wayland.
    # Some of VCS's functionality may be available on one but not the other.
    DEFINES += \
        VCS_FOR_X11
        #VCS_FOR_WAYLAND

    DEFINES += CAPTURE_BACKEND_VISION_V4L

    INCLUDEPATH += \
        # The base path for Datapath's Linux Vision driver header files. These are
        # bundled with the driver downloadable from Datapath's website. The header
        # files are expected to be in a "visionrgb/" subdirectory of this path, eg.
        # ".../visionrgb/include/rgb133control.h".
        $$(HOME)/sdk/

    LIBS += \
        -L/usr/local/lib/ \
        -lopencv_imgproc \
        -lopencv_highgui \
        -lopencv_core \
        -lopencv_photo

    contains(DEFINES, VCS_FOR_X11) {
        SOURCES += src/display/prevent_screensaver_x11.cpp
        LIBS += -lX11 -lXext
    } else {
        SOURCES += src/display/prevent_screensaver_null.cpp
    }
}

INCLUDEPATH += \
    $$PWD/src/ \
    $$PWD/src/display/qt/widgets/

RESOURCES += \
    src/display/qt/res.qrc

SOURCES += \
    src/common/refresh_rate.cpp \
    src/common/vcs_event/vcs_event.cpp \
    src/display/qt/widgets/DialogFileMenu.cpp \
    src/display/qt/widgets/DialogFragment.cpp \
    src/display/qt/widgets/InteractibleNodeGraph.cpp \
    src/display/qt/widgets/InteractibleNodeGraphNode.cpp \
    src/display/qt/widgets/InteractibleNodeGraphView.cpp \
    src/display/qt/widgets/MagnifyingGlass.cpp \
    src/display/qt/widgets/OGLWidget.cpp \
    src/display/qt/widgets/ParameterGrid.cpp \
    src/display/qt/widgets/PropertyTable.cpp \
    src/display/qt/widgets/QtAbstractGUI.cpp \
    src/display/qt/widgets/VideoPresetList.cpp \
    src/display/qt/windows/control_panel.cpp \
    src/display/qt/windows/control_panel/about_vcs.cpp \
    src/display/qt/windows/control_panel/capture.cpp \
    src/display/qt/windows/control_panel/capture/input_channel.cpp \
    src/display/qt/windows/control_panel/capture/input_resolution.cpp \
    src/display/qt/windows/control_panel/capture/signal_status.cpp \
    src/display/qt/windows/control_panel/filter_graph.cpp \
    src/display/qt/windows/control_panel/filter_graph/base_filter_graph_node.cpp \
    src/display/qt/windows/control_panel/filter_graph/output_scaler_node.cpp \
    src/display/qt/windows/control_panel/output.cpp \
    src/display/qt/windows/control_panel/output/renderer.cpp \
    src/display/qt/windows/control_panel/output/scaler.cpp \
    src/display/qt/windows/control_panel/output/size.cpp \
    src/display/qt/windows/control_panel/overlay.cpp \
    src/display/qt/windows/control_panel/video_presets.cpp \
    src/display/qt/dialogs/resolution_query.cpp \
    src/display/qt/keyboard_shortcuts.cpp \
    src/display/qt/windows/output_window.cpp \
    src/filter/filters/output_scaler/filter_output_scaler.cpp \
    src/filter/filters/output_scaler/gui/filtergui_output_scaler.cpp \
    src/filter/filters/render_text/filter_render_text.cpp \
    src/filter/filters/render_text/gui/filtergui_render_text.cpp \
    src/filter/filters/source_fps_estimate/filter_source_fps_estimate.cpp \
    src/filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.cpp \
    src/main.cpp \
    src/display/display.cpp \
    src/display/qt/d_main.cpp \
    src/anti_tear/anti_tear_multiple_per_frame.cpp \
    src/anti_tear/anti_tear_one_per_frame.cpp \
    src/anti_tear/anti_tearer.cpp \
    src/filter/abstract_filter.cpp \
    src/filter/filters/anti_tear/filter_anti_tear.cpp \
    src/filter/filters/anti_tear/gui/filtergui_anti_tear.cpp \
    src/filter/filters/blur/filter_blur.cpp \
    src/filter/filters/blur/gui/filtergui_blur.cpp \
    src/filter/filters/color_depth/filter_color_depth.cpp \
    src/filter/filters/color_depth/gui/filtergui_color_depth.cpp \
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
    src/filter/filters/flip/filter_flip.cpp \
    src/filter/filters/flip/gui/filtergui_flip.cpp \
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
    src/common/log/log.cpp \
    src/filter/filter.cpp \
    src/common/command_line/command_line.cpp \
    src/capture/capture.cpp \
    src/anti_tear/anti_tear.cpp \
    src/display/qt/persistent_settings.cpp \
    src/common/disk/disk.cpp \
    src/common/disk/file_writers/file_writer_filter_graph_version_b.cpp \
    src/common/disk/file_reader.cpp \
    src/common/disk/file_readers/file_reader_filter_graph_version_b.cpp \
    src/common/disk/file_readers/file_reader_filter_graph_version_a.cpp \
    src/common/disk/file_writer.cpp \
    src/common/disk/file_writers/file_writer_video_params_version_a.cpp \
    src/common/disk/file_readers/file_reader_video_params_version_a.cpp \
    src/display/qt/windows/control_panel/filter_graph/output_gate_node.cpp \
    src/display/qt/windows/control_panel/filter_graph/input_gate_node.cpp \
    src/display/qt/windows/control_panel/filter_graph/filter_node.cpp \
    src/capture/video_presets.cpp \
    src/common/disk/file_writers/file_writer_video_presets_version_a.cpp \
    src/common/disk/file_readers/file_reader_video_presets_version_a.cpp \
    src/common/timer/timer.cpp

HEADERS += \
    src/common/assert.h \
    src/common/globals.h \
    src/common/types.h \
    src/display/qt/widgets/DialogFileMenu.h \
    src/display/qt/widgets/DialogFragment.h \
    src/display/qt/widgets/InteractibleNodeGraph.h \
    src/display/qt/widgets/InteractibleNodeGraphNode.h \
    src/display/qt/widgets/InteractibleNodeGraphView.h \
    src/display/qt/widgets/MagnifyingGlass.h \
    src/display/qt/widgets/OGLWidget.h \
    src/display/qt/widgets/ParameterGrid.h \
    src/display/qt/widgets/PropertyTable.h \
    src/display/qt/widgets/QtAbstractGUI.h \
    src/display/qt/widgets/VideoPresetList.h \
    src/display/qt/windows/control_panel.h \
    src/display/qt/windows/control_panel/about_vcs.h \
    src/display/qt/windows/control_panel/capture.h \
    src/display/qt/windows/control_panel/capture/input_channel.h \
    src/display/qt/windows/control_panel/capture/input_resolution.h \
    src/display/qt/windows/control_panel/capture/signal_status.h \
    src/display/qt/windows/control_panel/filter_graph.h \
    src/display/qt/windows/control_panel/filter_graph/base_filter_graph_node.h \
    src/display/qt/windows/control_panel/filter_graph/output_scaler_node.h \
    src/display/qt/windows/control_panel/output.h \
    src/display/qt/windows/control_panel/output/renderer.h \
    src/display/qt/windows/control_panel/output/scaler.h \
    src/display/qt/windows/control_panel/output/size.h \
    src/display/qt/windows/control_panel/overlay.h \
    src/display/qt/windows/control_panel/video_presets.h \
    src/display/qt/dialogs/resolution_query.h \
    src/display/qt/keyboard_shortcuts.h \
    src/anti_tear/anti_tear_multiple_per_frame.h \
    src/anti_tear/anti_tear_one_per_frame.h \
    src/anti_tear/anti_tearer.h \
    src/display/qt/windows/output_window.h \
    src/filter/filters/anti_tear/filter_anti_tear.h \
    src/filter/filters/anti_tear/gui/filtergui_anti_tear.h \
    src/filter/filters/blur/filter_blur.h \
    src/filter/filters/blur/gui/filtergui_blur.h \
    src/filter/filters/color_depth/filter_color_depth.h \
    src/filter/filters/color_depth/gui/filtergui_color_depth.h \
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
    src/filter/filters/filters.h \
    src/filter/filters/flip/filter_flip.h \
    src/filter/filters/flip/gui/filtergui_flip.h \
    src/filter/filters/output_scaler/filter_output_scaler.h \
    src/filter/filters/output_scaler/gui/filtergui_output_scaler.h \
    src/filter/filters/render_text/font.h \
    src/filter/filters/render_text/font_10x6_sans_serif.h \
    src/filter/filters/render_text/font_10x6_serif.h \
    src/filter/filters/render_text/font_5x3.h \
    src/filter/filters/render_text/font_fraps.h \
    src/filter/filters/source_fps_estimate/filter_source_fps_estimate.h \
    src/filter/filters/source_fps_estimate/gui/filtergui_source_fps_estimate.h \
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
    src/filter/filters/render_text/filter_render_text.h \
    src/filter/filters/render_text/gui/filtergui_render_text.h \
    src/filter/filters/unsharp_mask/filter_unsharp_mask.h \
    src/filter/filters/unsharp_mask/gui/filtergui_unsharp_mask.h \
    src/main.h \
    src/scaler/scaler.h \
    src/capture/capture.h \
    src/display/display.h \
    src/common/log/log.h \
    src/common/abstract_gui.h \
    src/anti_tear/anti_tear.h \
    src/filter/filter.h \
    src/common/command_line/command_line.h \
    src/display/qt/persistent_settings.h \
    src/display/qt/utility.h \
    src/common/disk/csv.h \
    src/common/disk/disk.h \
    src/display/display.h \
    src/common/disk/file_writers/file_writer_filter_graph.h \
    src/common/disk/file_readers/file_reader_filter_graph.h \
    src/common/disk/file_reader.h \
    src/common/disk/file_streamer.h \
    src/common/disk/file_readers/file_reader_video_params.h \
    src/common/disk/file_writers/file_writer_video_params.h \
    src/common/disk/file_writer.h \
    src/display/qt/windows/control_panel/filter_graph/output_gate_node.h \
    src/display/qt/windows/control_panel/filter_graph/input_gate_node.h \
    src/display/qt/windows/control_panel/filter_graph/filter_node.h \
    src/common/refresh_rate.h \
    src/capture/video_presets.h \
    src/common/disk/file_writers/file_writer_video_presets.h \
    src/common/disk/file_readers/file_reader_video_presets.h \
    src/common/vcs_event/vcs_event.h \
    src/common/timer/timer.h

FORMS += \
    src/display/qt/windows/control_panel.ui \
    src/display/qt/windows/control_panel/about_vcs.ui \
    src/display/qt/windows/control_panel/capture.ui \
    src/display/qt/windows/control_panel/capture/input_channel.ui \
    src/display/qt/windows/control_panel/capture/input_resolution.ui \
    src/display/qt/windows/control_panel/capture/signal_status.ui \
    src/display/qt/windows/control_panel/filter_graph.ui \
    src/display/qt/windows/control_panel/output.ui \
    src/display/qt/windows/control_panel/output/renderer.ui \
    src/display/qt/windows/control_panel/output/scaler.ui \
    src/display/qt/windows/control_panel/output/size.ui \
    src/display/qt/windows/control_panel/overlay.ui \
    src/display/qt/windows/control_panel/video_presets.ui \
    src/display/qt/dialogs/resolution_query.ui \
    src/display/qt/windows/output_window.ui

contains(DEFINES, CAPTURE_BACKEND_VIRTUAL) {
    SOURCES += src/capture/virtual/capture_virtual.cpp
}

contains(DEFINES, CAPTURE_BACKEND_DOSBOX_MMAP) {
    SOURCES += src/capture/dosbox_mmap/capture_dosbox_mmap.cpp
    LIBS += -lrt
}

contains(DEFINES, CAPTURE_BACKEND_VISION_V4L) {
    SOURCES += \
        src/capture/vision_v4l/capture_vision_v4l.cpp \
        src/capture/vision_v4l/input_channel_v4l.cpp \
        src/capture/vision_v4l/ic_v4l_video_parameters.cpp

    HEADERS += \
        src/capture/vision_v4l/input_channel_v4l.h \
        src/capture/vision_v4l/ic_v4l_video_parameters.h
}

QT += core gui widgets
TARGET = vcs
TEMPLATE = app
CONFIG += console release object_parallel_to_source
QMAKE_CXXFLAGS += -pedantic -std=c++2a -Wno-missing-field-initializers
OBJECTS_DIR = generated_files
RCC_DIR = generated_files
MOC_DIR = generated_files
UI_DIR = generated_files

!contains(DEFINES, VCS_RELEASE_BUILD) {
    CONFIG = $$replace(CONFIG, "release", "debug")
    CONFIG += sanitizer sanitize_address sanitize_undefined
    DEFINES += _GLIBCXX_DEBUG _GLIBCXX_DEBUG_PEDANTIC
}
