# Comment out to disable OpenCV. You'll have no filtering or scaler, but you also don't need to provide the dependencies.
DEFINES += USE_OPENCV

# Comment out to disable capture functionality. Useful for testing the program where a capture card isn't present.
DEFINES += USE_RGBEASY_API

# Enable non-critical asserts. May perform slower, but will e.g. look to guard against buffer overflow in memory access.
#DEFINES += ENFORCE_OPTIONAL_ASSERTS

# For now, disable the RGBEASY API while doing a validation run, to simplify things.
# Once the validatiom system is a bit better fleshed out, this should not be needed.
contains(DEFINES, VALIDATION_RUN) {
    DEFINES -= USE_RGBEASY_API
}

# Specific configurations for the current platform.
linux {
    # I can't test capture functionality under Linux (my card doesn't support it),
    # so I'll have to disable it, by default.
    DEFINES -= USE_RGBEASY_API

    # OpenCV.
    contains(DEFINES, USE_OPENCV) {
        LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui
    }
}
win32 {
    # The RGBEASY API.
    contains(DEFINES, USE_RGBEASY_API) {
        INCLUDEPATH += "C:/Program Files (x86)/Vision RGB-PRO/SDK/RGBEASY/INCLUDE"
        LIBS += "C:/Program Files (x86)/Vision RGB-PRO/SDK/RGBEASY/LIB/RGBEASY.lib"
    }

    # OpenCV.
    contains(DEFINES, USE_OPENCV) {
        INCLUDEPATH += "C:/Program Files (x86)/OpenCV/3.2.0/include"
        LIBS += -L"C:/Program Files (x86)/OpenCV/3.2.0/bin/mingw"
        LIBS += -lopencv_world320
    }
}

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = vcs
TEMPLATE = app
CONFIG += console c++11

OBJECTS_DIR = generated_files
MOC_DIR = generated_files
UI_DIR = generated_files

INCLUDEPATH += $$PWD/src/display/qt/

SOURCES += \
    src/display/qt/d_window.cpp \
    src/display/qt/d_resolution_dialog.cpp \
    src/display/qt/d_main.cpp \
    src/display/qt/d_control_panel.cpp \
    src/display/qt/d_video_and_color_dialog.cpp \
    src/display/qt/d_overlay_dialog.cpp \
    src/display/qt/d_alias_dialog.cpp \
    src/display/qt/d_anti_tear_dialog.cpp \
    src/scaler/scaler.cpp \
    src/main.cpp \
    src/common/log.cpp \
    src/filter/filter.cpp \
    src/common/command_line.cpp \
    src/capture/capture.cpp \
    src/filter/anti_tear.cpp \
    src/common/persistent_settings.cpp \
    src/common/memory.cpp \
    src/display/qt/d_filter_set_dialog.cpp \
    src/display/qt/dw_filter_list.cpp \
    src/display/qt/d_filter_sets_list_dialog.cpp \
    src/display/qt/w_opengl.cpp \
    src/record/record.cpp

HEADERS += \
    src/common/globals.h \
    src/capture/null_rgbeasy.h \
    src/common/types.h \
    src/display/qt/d_window.h \
    src/display/qt/d_resolution_dialog.h \
    src/scaler/scaler.h \
    src/capture/capture.h \
    src/display/display.h \
    src/main.h \
    src/display/qt/d_control_panel.h \
    src/common/log.h \
    src/display/qt/d_video_and_color_dialog.h \
    src/display/qt/d_overlay_dialog.h \
    src/display/qt/d_alias_dialog.h \
    src/filter/anti_tear.h \
    src/display/qt/d_anti_tear_dialog.h \
    src/filter/filter.h \
    src/common/command_line.h \
    src/display/qt/df_filters.h \
    src/common/persistent_settings.h \
    src/display/qt/d_util.h \
    src/common/csv.h \
    src/common/memory.h \
    src/common/memory_interface.h \
    src/display/qt/d_filter_set_dialog.h \
    src/display/qt/dw_filter_list.h \
    src/display/qt/d_filter_sets_list_dialog.h \
    src/display/qt/w_opengl.h \
    src/record/record.h

FORMS += \
    src/display/qt/d_control_panel.ui \
    src/display/qt/d_window.ui \
    src/display/qt/d_video_and_color_dialog.ui \
    src/display/qt/d_overlay_dialog.ui \
    src/display/qt/d_resolution_dialog.ui \
    src/display/qt/d_alias_dialog.ui \
    src/display/qt/d_anti_tear_dialog.ui \
    src/display/qt/d_filter_set_dialog.ui \
    src/display/qt/d_filter_sets_list_dialog.ui

# C++. For GCC/Clang/MinGW.
QMAKE_CXXFLAGS += -g
QMAKE_CXXFLAGS += -O2
QMAKE_CXXFLAGS += -Wall
QMAKE_CXXFLAGS += -ansi
QMAKE_CXXFLAGS += -pipe
QMAKE_CXXFLAGS += -pedantic
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -Wno-missing-field-initializers
