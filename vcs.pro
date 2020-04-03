# Comment out to disable OpenCV. You'll have no filtering or scaler, but you also don't need to provide the dependencies.
DEFINES += USE_OPENCV

# Enable non-critical asserts. May perform slower, but will e.g. look to guard against buffer overflow in memory access.
#DEFINES += ENFORCE_OPTIONAL_ASSERTS

linux {
    DEFINES += CAPTURE_API_VIDEO4LINUX

    # The base path for Datapath's Linux Vision driver header files. These are
    # bundled with the driver downloadable from Datapath's website. The files
    # are expected to be in a visionrgb subdirectory of this path e.g.
    # .../visionrgb/include/rgb133control.h.
    INCLUDEPATH += $$(HOME)/sdk/

    contains(DEFINES, USE_OPENCV) {
        LIBS += -lopencv_imgproc -lopencv_videoio -lopencv_highgui -lopencv_core -lopencv_photo
    }
}

win32 {
    DEFINES += CAPTURE_API_RGBEASY
    
    INCLUDEPATH += "C:/Program Files (x86)/Vision RGB-PRO/SDK/RGBEASY/INCLUDE"
    LIBS += "C:/Program Files (x86)/Vision RGB-PRO/SDK/RGBEASY/LIB/RGBEASY.lib"

    contains(DEFINES, USE_OPENCV) {
        INCLUDEPATH += "C:/Program Files (x86)/OpenCV/3.2.0/include"
        LIBS += -L"C:/Program Files (x86)/OpenCV/3.2.0/bin/mingw"
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
    src/display/qt/windows/output_window.cpp \
    src/display/qt/dialogs/resolution_dialog.cpp \
    src/display/qt/d_main.cpp \
    src/display/qt/dialogs/overlay_dialog.cpp \
    src/display/qt/dialogs/alias_dialog.cpp \
    src/display/qt/dialogs/anti_tear_dialog.cpp \
    src/scaler/scaler.cpp \
    src/main.cpp \
    src/common/log/log.cpp \
    src/filter/filter.cpp \
    src/common/command_line/command_line.cpp \
    src/capture/capture.cpp \
    src/filter/anti_tear.cpp \
    src/display/qt/persistent_settings.cpp \
    src/common/memory/memory.cpp \
    src/record/record.cpp \
    src/common/propagate/propagate.cpp \
    src/common/disk/disk.cpp \
    src/capture/alias.cpp \
    src/display/qt/subclasses/QOpenGLWidget_opengl_renderer.cpp \
    src/display/qt/widgets/filter_widgets.cpp \
    src/display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.cpp \
    src/display/qt/subclasses/QGraphicsScene_interactible_node_graph.cpp \
    src/display/qt/subclasses/QGraphicsView_interactible_node_graph_view.cpp \
    src/display/qt/dialogs/filter_graph_dialog.cpp \
    src/display/qt/subclasses/InteractibleNodeGraphNode_filter_graph_nodes.cpp \
    src/display/qt/dialogs/about_dialog.cpp \
    src/display/qt/dialogs/record_dialog.cpp \
    src/display/qt/dialogs/output_resolution_dialog.cpp \
    src/display/qt/dialogs/input_resolution_dialog.cpp \
    src/capture/capture_api_virtual.cpp \
    src/capture/capture_api_rgbeasy.cpp \
    src/capture/capture_api.cpp \
    src/capture/video_parameters.cpp \
    src/display/qt/subclasses/QTableWidget_property_table.cpp \
    src/display/qt/dialogs/signal_dialog.cpp \
    src/capture/capture_api_video4linux.cpp \
    src/filter/filter_funcs.cpp \
    src/common/disk/file_writer_filter_graph_version_b.cpp \
    src/common/disk/file_reader.cpp \
    src/common/disk/file_reader_filter_graph_version_b.cpp \
    src/common/disk/file_reader_filter_graph_version_a.cpp

HEADERS += \
    src/common/globals.h \
    src/capture/null_rgbeasy.h \
    src/common/types.h \
    src/display/qt/windows/output_window.h \
    src/display/qt/dialogs/resolution_dialog.h \
    src/scaler/scaler.h \
    src/capture/capture.h \
    src/display/display.h \
    src/common/log/log.h \
    src/display/qt/dialogs/overlay_dialog.h \
    src/display/qt/dialogs/alias_dialog.h \
    src/filter/anti_tear.h \
    src/display/qt/dialogs/anti_tear_dialog.h \
    src/filter/filter.h \
    src/common/command_line/command_line.h \
    src/display/qt/persistent_settings.h \
    src/display/qt/utility.h \
    src/common/disk/csv.h \
    src/common/memory/memory.h \
    src/common/memory/memory_interface.h \
    src/record/record.h \
    src/common/propagate/propagate.h \
    src/common/disk/disk.h \
    src/capture/alias.h \
    src/display/qt/subclasses/QOpenGLWidget_opengl_renderer.h \
    src/display/qt/widgets/filter_widgets.h \
    src/display/qt/subclasses/QGraphicsItem_interactible_node_graph_node.h \
    src/display/qt/subclasses/QGraphicsScene_interactible_node_graph.h \
    src/display/qt/subclasses/QGraphicsView_interactible_node_graph_view.h \
    src/common/disk/file_writer.h \
    src/display/qt/dialogs/filter_graph_dialog.h \
    src/display/qt/subclasses/InteractibleNodeGraphNode_filter_graph_nodes.h \
    src/display/qt/dialogs/about_dialog.h \
    src/display/qt/dialogs/record_dialog.h \
    src/display/qt/dialogs/output_resolution_dialog.h \
    src/display/qt/dialogs/input_resolution_dialog.h \
    src/capture/capture_api.h \
    src/capture/capture_api_virtual.h \
    src/capture/capture_api_rgbeasy.h \
    src/capture/video_parameters.h \
    src/display/display.h \
    src/display/qt/subclasses/QTableWidget_property_table.h \
    src/display/qt/dialogs/signal_dialog.h \
    src/capture/capture_api_video4linux.h \
    src/filter/filter_funcs.h \
    src/common/disk/file_writer_filter_graph.h \
    src/common/disk/file_reader_filter_graph.h \
    src/common/disk/file_reader.h

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
    src/display/qt/dialogs/ui/signal_dialog.ui

# C++. For GCC/Clang/MinGW.
QMAKE_CXXFLAGS += -g
QMAKE_CXXFLAGS += -O2
QMAKE_CXXFLAGS += -Wall
QMAKE_CXXFLAGS += -pipe
QMAKE_CXXFLAGS += -pedantic
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -Wno-missing-field-initializers
