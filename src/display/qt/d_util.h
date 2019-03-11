/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef DISPLAY_UTIL_H
#define DISPLAY_UTIL_H

#include <QWidget>

// Has the given widget's signals blocked while in the scope of this object, after
// which the widget's previous blocking status is restored.
class block_widget_signals_c
{
public:
    block_widget_signals_c(QWidget *widget) :
        w(widget),
        previousBlockStatus(w->blockSignals(true))
    {
        return;
    }
    ~block_widget_signals_c()
    {
        w->blockSignals(previousBlockStatus);
    }

private:
    QWidget *const w;
    const bool previousBlockStatus;
};

#endif
