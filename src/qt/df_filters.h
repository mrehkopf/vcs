/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 *  Defines small user-facing GUI dialogs which allow adjustment of filters'
 *  parameters.
 *
 * USAGE:
 *  - Inherit from the base dialog, and customize the UI elements as needed (see
 *    the existing implementations, below, for an idea of how it works).
 *
 *  - The name you give the filter dialog, through filterName, will be user-facing
 *    in the dialog's title bar, and must also match the name of the corresponding
 *    filter as defined in filter.cpp.
 *
 */

#ifndef FILTER_DIALOGS_H
#define FILTER_DIALOGS_H

#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QString>
#include <QDialog>
#include <QLabel>
#include "../common.h"
#include "../types.h"

// Filter dialog base. Inherit from this to create custom ones.
struct filter_dlg_s
{
    const QString filterName;           // Shown in the dialog's title bar.
    const uint dlgMinWidth = 180;       // How small at most the dialog can be horizontally.
    const QString noParamsMsg = "This filter takes no parameters."; // The message shown in dialogs for filters that take no parameters.

    filter_dlg_s(const char *const name) : filterName(name) {}

    // Fills the given byte array with the filter's default parameters.
    // Expects the array to be able to hold all of them.
    virtual void insert_default_params(u8 *const paramData) const = 0;

    // Pops up a dialog which presents the user with GUI controls for adjusting
    // the filter's parameters. Once the user closes the dialog, the parameters
    // they selected will be saved in the given byte array.
    virtual void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const = 0;

    const QString& name(void)
    {
        return filterName;
    }
};

struct filter_dlg_crop_s : public filter_dlg_s
{
    // Note: x, y, width, and height reserve two bytes each.
    enum data_offset_e { OFFS_X = 0, OFFS_Y = 2, OFFS_WIDTH = 4, OFFS_HEIGHT = 6, OFFS_SCALER = 8 };

    filter_dlg_crop_s() :
        filter_dlg_s("Crop") {}

    void insert_default_params(u8 *const paramData) const override
    {
        memset(paramData, 0, sizeof(u8) * FILTER_DATA_LENGTH);

        *(u16*)&(paramData[OFFS_X]) = 0;
        *(u16*)&(paramData[OFFS_Y]) = 0;
        *(u16*)&(paramData[OFFS_WIDTH]) = 640;
        *(u16*)&(paramData[OFFS_HEIGHT]) = 480;
        paramData[OFFS_SCALER] = 0;

        return;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const override
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        QLabel xLabel("X:");
        QSpinBox xSpin;
        xSpin.setRange(0, 65535);
        xSpin.setValue(*(u16*)&(paramData[OFFS_X]));

        QLabel yLabel("Y:");
        QSpinBox ySpin;
        ySpin.setRange(0, 65535);
        ySpin.setValue(*(u16*)&(paramData[OFFS_Y]));

        QLabel widthLabel("Width:");
        QSpinBox widthSpin;
        widthSpin.setRange(0, 65535);
        widthSpin.setValue(*(u16*)&(paramData[OFFS_WIDTH]));

        QLabel heightLabel("Height:");
        QSpinBox heightSpin;
        heightSpin.setRange(0, 65535);
        heightSpin.setValue(*(u16*)&(paramData[OFFS_HEIGHT]));

        QLabel scalerLabel("Scaler:");
        QComboBox scalerList;
        scalerList.addItem("Linear");
        scalerList.addItem("Nearest");
        scalerList.addItem("(Don't scale)");
        scalerList.setCurrentIndex(paramData[OFFS_SCALER]);

        QFormLayout l;
        l.addRow(&xLabel, &xSpin);
        l.addRow(&yLabel, &ySpin);
        l.addRow(&widthLabel, &widthSpin);
        l.addRow(&heightLabel, &heightSpin);
        l.addRow(&scalerLabel, &scalerList);

        d.setLayout(&l);
        d.exec();

        *(u16*)&(paramData[OFFS_X]) = xSpin.value();
        *(u16*)&(paramData[OFFS_Y]) = ySpin.value();
        *(u16*)&(paramData[OFFS_WIDTH]) = widthSpin.value();
        *(u16*)&(paramData[OFFS_HEIGHT]) = heightSpin.value();
        paramData[OFFS_SCALER] = scalerList.currentIndex();

        return;
    }
};

struct filter_dlg_flip_s : public filter_dlg_s
{
    enum data_offset_e { OFFS_AXIS = 0 };

    filter_dlg_flip_s() :
        filter_dlg_s("Flip") {}

    void insert_default_params(u8 *const paramData) const override
    {
        memset(paramData, 0, sizeof(u8) * FILTER_DATA_LENGTH);

        return;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const override
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        QLabel axisLabel("Axis:");
        QComboBox axisList;
        axisList.addItem("Vertical");
        axisList.addItem("Horizontal");
        axisList.addItem("Both");
        axisList.setCurrentIndex(paramData[OFFS_AXIS]);

        QFormLayout l;
        l.addRow(&axisLabel, &axisList);

        d.setLayout(&l);
        d.exec();

        paramData[OFFS_AXIS] = axisList.currentIndex();

        return;
    }
};

struct filter_dlg_rotate_s : public filter_dlg_s
{
    // Note: the rotation angle and scale reserve two bytes.
    enum data_offset_e { OFFS_ROT = 0, OFFS_SCALE = 2 };

    filter_dlg_rotate_s() :
        filter_dlg_s("Rotate") {}

    void insert_default_params(u8 *const paramData) const override
    {
        memset(paramData, 0, sizeof(u8) * FILTER_DATA_LENGTH);

        // The scale value gets divided by 100 when used.
        *(i16*)&(paramData[OFFS_SCALE]) = 100;

        // The rotation value gets divided by 10 when used.
        *(i16*)&(paramData[OFFS_ROT]) = 0;

        return;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const override
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        QLabel rotLabel("Angle:");
        QDoubleSpinBox rotSpin;
        rotSpin.setDecimals(1);
        rotSpin.setRange(-360, 360);
        rotSpin.setValue(*(i16*)&(paramData[OFFS_ROT]) / 10.0);

        QLabel scaleLabel("Scale:");
        QDoubleSpinBox scaleSpin;
        scaleSpin.setDecimals(2);
        scaleSpin.setRange(0, 20);
        scaleSpin.setValue((*(i16*)&(paramData[OFFS_SCALE])) / 100.0);

        QFormLayout l;
        l.addRow(&rotLabel, &rotSpin);
        l.addRow(&scaleLabel, &scaleSpin);

        d.setLayout(&l);
        d.exec();

        *(i16*)&(paramData[OFFS_ROT]) = (rotSpin.value() * 10);
        *(i16*)&(paramData[OFFS_SCALE]) = (scaleSpin.value() * 100);

        return;
    }
};

struct filter_dlg_median_s : public filter_dlg_s
{
    enum data_offset_e { OFF_KERNEL_SIZE = 0 };

    filter_dlg_median_s() :
        filter_dlg_s("Median") {}

    void insert_default_params(u8 *const paramData) const override
    {
        memset(paramData, 0, sizeof(u8) * FILTER_DATA_LENGTH);
        paramData[OFF_KERNEL_SIZE] = 3;

        return;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const override
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        // Median radius.
        QLabel radiusLabel("Radius:");
        QSpinBox radiusSpin;
        radiusSpin.setRange(0, 99);
        radiusSpin.setValue(paramData[OFF_KERNEL_SIZE] / 2);

        QFormLayout l;
        l.addRow(&radiusLabel, &radiusSpin);

        d.setLayout(&l);
        d.exec();       // Blocking execute until the user closes the dialog.

        paramData[OFF_KERNEL_SIZE] = ((radiusSpin.value() * 2) + 1);

        return;
    }
};

struct filter_dlg_blur_s : public filter_dlg_s
{
    enum data_offset_e { OFF_TYPE = 0, OFF_KERNEL_SIZE = 1 };
    enum filter_type_e { FILTER_TYPE_BOX = 0, FILTER_TYPE_GAUSSIAN = 1 };

    filter_dlg_blur_s() :
        filter_dlg_s("Blur") {}

    void insert_default_params(u8 *const paramData) const override
    {
        memset(paramData, 0, sizeof(u8) * FILTER_DATA_LENGTH);
        paramData[OFF_KERNEL_SIZE] = 10;
        paramData[OFF_TYPE] = FILTER_TYPE_GAUSSIAN;

        return;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const override
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        // Blur type.
        QLabel typeLabel("Type:");
        QComboBox typeList;
        typeList.addItem("Box");
        typeList.addItem("Gaussian");
        typeList.setCurrentIndex(paramData[OFF_TYPE]);

        // Blur radius.
        QLabel radiusLabel("Radius:");
        QDoubleSpinBox radiusSpin;
        radiusSpin.setRange(0, 25);
        radiusSpin.setDecimals(1);
        radiusSpin.setValue(paramData[OFF_KERNEL_SIZE] / 10.0);

        QFormLayout l;
        l.addRow(&typeLabel, &typeList);
        l.addRow(&radiusLabel, &radiusSpin);

        d.setLayout(&l);
        d.exec();       // Blocking execute until the user closes the dialog.

        // Fetch the settings the user gave.
        paramData[OFF_TYPE] = typeList.currentIndex();
        paramData[OFF_KERNEL_SIZE] = round(radiusSpin.value() * 10.0);

        return;
    }
};

struct filter_dlg_denoise_s : public filter_dlg_s
{
    enum data_offset_e { OFFS_THRESHOLD = 0};
    enum filter_type_e { FILTER_TYPE_TEMPORAL = 0, FILTER_TYPE_SPATIAL = 1 };

    filter_dlg_denoise_s() :
        filter_dlg_s("Denoise") {}

    void insert_default_params(u8 *const paramData) const override
    {
        memset(paramData, 0, sizeof(u8) * FILTER_DATA_LENGTH);
        paramData[OFFS_THRESHOLD] = 5;

        return;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const override
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        // Denoising threshold.
        QLabel thresholdLabel("Threshold:");
        QSpinBox thresholdSpin;
        thresholdSpin.setRange(0, 255);
        thresholdSpin.setValue(paramData[OFFS_THRESHOLD]);

        QFormLayout l;
        l.addRow(&thresholdLabel, &thresholdSpin);

        d.setLayout(&l);
        d.exec();       // Blocking execute until the user closes the dialog.

        paramData[OFFS_THRESHOLD] = thresholdSpin.value();

        return;
    }
};

struct filter_dlg_sharpen_s : public filter_dlg_s
{
    filter_dlg_sharpen_s() :
        filter_dlg_s("Sharpen") {}

    void insert_default_params(u8 *const paramData) const
    {
        memset(paramData, 0, sizeof(u8) * FILTER_DATA_LENGTH);

        return;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        QLabel noneLabel(noParamsMsg);

        QHBoxLayout l;
        l.addWidget(&noneLabel);

        d.setLayout(&l);
        d.exec();       // Blocking execute until the user closes the dialog.

        (void)paramData;/// Not used.

        return;
    }
};

struct filter_dlg_unsharpmask_s : public filter_dlg_s
{
    enum data_offset_e { OFFS_STRENGTH = 0, OFFS_RADIUS = 1 };

    filter_dlg_unsharpmask_s() :
        filter_dlg_s("Unsharp Mask") {}

    void insert_default_params(u8 *const paramData) const override
    {
        memset(paramData, 0, sizeof(u8) * FILTER_DATA_LENGTH);
        paramData[OFFS_STRENGTH] = 50;
        paramData[OFFS_RADIUS] = 10;

        return;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const override
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        // Strength.
        QLabel strLabel("Strength:");
        QSpinBox strSpin;
        strSpin.setRange(0, 255);
        strSpin.setValue(paramData[OFFS_STRENGTH]);

        // Radius.
        QLabel radiusLabel("Radius:");
        QSpinBox radiusdSpin;
        radiusdSpin.setRange(0, 255);
        radiusdSpin.setValue(paramData[OFFS_RADIUS] / 10);

        QFormLayout l;
        l.addRow(&strLabel, &strSpin);
        l.addRow(&radiusLabel, &radiusdSpin);

        d.setLayout(&l);
        d.exec();       // Blocking execute until the user closes the dialog.

        paramData[OFFS_STRENGTH] = strSpin.value();
        paramData[OFFS_RADIUS] = radiusdSpin.value() * 10;

        return;
    }
};

struct filter_dlg_decimate_s : public filter_dlg_s
{
    enum data_offset_e { OFFS_TYPE = 0, OFFS_FACTOR = 1 };

    enum filter_type_e { FILTER_TYPE_NEAREST = 0, FILTER_TYPE_AVERAGE = 1 };

    filter_dlg_decimate_s() :
        filter_dlg_s("Decimate") {}

    void insert_default_params(u8 *const paramData) const override
    {
        memset(paramData, 0, sizeof(u8) * FILTER_DATA_LENGTH);
        paramData[OFFS_FACTOR] = 2;
        paramData[OFFS_TYPE] = FILTER_TYPE_AVERAGE;

        return;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const override
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        // Factor.
        QLabel factorLabel("Factor:");
        QComboBox factorList;
        factorList.addItem("2");
        factorList.addItem("4");
        factorList.addItem("8");
        factorList.addItem("16");
        factorList.setCurrentIndex((round(sqrt(paramData[OFFS_FACTOR])) - 1));

        // Sampling.
        QLabel radiusLabel("Sampling:");
        QComboBox samplingList;
        samplingList.addItem("Nearest");
        samplingList.addItem("Average");
        samplingList.setCurrentIndex(paramData[OFFS_TYPE]);

        QFormLayout l;
        l.addRow(&factorLabel, &factorList);
        l.addRow(&radiusLabel, &samplingList);

        d.setLayout(&l);
        d.exec();       // Blocking execute until the user closes the dialog.

        paramData[OFFS_FACTOR] = factorList.itemText(factorList.currentIndex()).toUInt();
        paramData[OFFS_TYPE] = samplingList.currentIndex();

        return;
    }
};

struct filter_dlg_deltahistogram_s : public filter_dlg_s
{
    filter_dlg_deltahistogram_s() :
        filter_dlg_s("Delta Histogram") {}

    void insert_default_params(u8 *const paramData) const
    {
        memset(paramData, 0, sizeof(u8) * FILTER_DATA_LENGTH);

        return;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        QLabel noneLabel(noParamsMsg);

        QHBoxLayout l;
        l.addWidget(&noneLabel);

        d.setLayout(&l);
        d.exec();       // Blocking execute until the user closes the dialog.

        (void)paramData;/// Not used.

        return;
    }
};

struct filter_dlg_uniquecount_s : public filter_dlg_s
{
    enum data_offset_e { OFFS_THRESHOLD = 0 };

    filter_dlg_uniquecount_s() :
        filter_dlg_s("Unique Count") {}

    void insert_default_params(u8 *const paramData) const override
    {
        memset(paramData, 0, sizeof(u8) * FILTER_DATA_LENGTH);
        paramData[OFFS_THRESHOLD] = 20;

        return;
    }

    void poll_user_for_params(u8 *const paramData, QWidget *const parent = nullptr) const override
    {
        QDialog d(parent, QDialog().windowFlags() & ~Qt::WindowContextHelpButtonHint);
        d.setWindowTitle(filterName + " Filter");
        d.setMinimumWidth(dlgMinWidth);

        // Denoising threshold.
        QLabel thresholdLabel("Threshold:");
        QSpinBox thresholdSpin;
        thresholdSpin.setRange(0, 255);
        thresholdSpin.setValue(paramData[OFFS_THRESHOLD]);

        QFormLayout l;
        l.addRow(&thresholdLabel, &thresholdSpin);

        d.setLayout(&l);
        d.exec();       // Blocking execute until the user closes the dialog.

        paramData[OFFS_THRESHOLD] = thresholdSpin.value();

        return;
    }
};

#endif
