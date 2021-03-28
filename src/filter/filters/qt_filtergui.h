/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_QT_FILTERGUI_H
#define VCS_FILTER_FILTERS_QT_FILTERGUI_H

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QComboBox>
#include <QWidget>
#include <QObject>
#include <QFrame>
#include <QLabel>
#include <cstring>
#include <string>
#include "common/globals.h"
#include "filter/filter.h"
#include "common/types.h"

struct filtergui_c : public QObject
{
protected:
    // The parent filter to this filter GUI widget.
    filter_c *const filter;

public:
    filtergui_c(filter_c *const filter,
                const unsigned minWidth = 220);

    virtual ~filtergui_c(void);

    void set_parameter(const unsigned offset,
                       const uint32_t value,
                       const bool silent = false)
    {
        this->filter->set_parameter(offset, value);

        if (!silent)
        {
            emit this->parameter_changed();
        }

        return;
    }

    QWidget *widget = nullptr;

    // The user-facing name of this filter - e.g. "Blur" for a blur filter.
    const QString title;

    // The string shown to the user in the widget if this filter has no
    // user-configurable parameters.
    const QString noParamsMsg = "(No parameters.)";

    // The default width of the widget.
    const unsigned minWidth;

signals:
    // Emitted when the widget's parameter data is changed.
    void parameter_changed(void);

private:
    Q_OBJECT
};  

#endif
