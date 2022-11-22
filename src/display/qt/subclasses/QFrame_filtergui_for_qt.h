/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QFRAME_FILTERGUI_FOR_QT_H
#define VCS_DISPLAY_QT_SUBCLASSES_QFRAME_FILTERGUI_FOR_QT_H

#include <vector>
#include <QFrame>
#include "filter/filter.h"

class FilterGUIForQt : public QFrame
{
    Q_OBJECT

public:
    // Constructs a Qt widget out of the filter's framework-indepdent GUI description.
    explicit FilterGUIForQt(const abstract_filter_c *const filter, QWidget *parent = 0);

signals:
    // Emitted when the filter's parameter data is changed via this widget.
    void parameter_changed(void);
};

#endif
