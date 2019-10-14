/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * Miscellaneous Qt-related utility functions.
 *
 */

#ifndef DISPLAY_UTIL_H
#define DISPLAY_UTIL_H

#include <QComboBox>
#include <QWidget>
#include <QTimer>
#include "common/globals.h"

// Emits a signal when the mouse becomes active or inactivates. Note that this class
// works manually rather than automatically: you call report_activity() to indicate
// that the mouse is doing something, and if no further calls to report_activity()
// are made in the next x seconds, the mouse will be considered inactive and a signal
// as such will be emitted.
class mouse_activity_monitor_c : public QObject
{
    Q_OBJECT

public:
    mouse_activity_monitor_c(void)
    {
        this->inactivityTimer = new QTimer(this);
        this->inactivityTimer->setSingleShot(true);

        connect(this->inactivityTimer, &QTimer::timeout, this, [=]
        {
            if (this->isActive)
            {
                emit this->inactivated();
            }

            this->isActive = false;
        });

        return;
    }

    void report_activity(void)
    {
        if (!this->isActive)
        {
            emit this->activated();
        }

        this->isActive = true;
        this->inactivityTimer->start(this->inactivityTimeoutMs);

        return;
    }

    void report_inactivity(void)
    {
        if (this->isActive)
        {
            emit this->inactivated();
        }

        this->isActive = false;
        this->inactivityTimer->stop();
    }

    bool is_active(void) { return this->isActive; }

signals:
    // Emitted when the mouse becomes inactive after having been active.
    void inactivated();

    // Emitted when the mouse becomes active after having been inactive.
    void activated();

private:
    bool isActive = false;

    // A timer used to detect when the mouse cursor, hovering over the window, has
    // become inactive, i.e. hasn't performed any actions in a set period of time.
    QTimer *inactivityTimer;
    const unsigned inactivityTimeoutMs = 2500;
};

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

class set_qcombobox_idx_c
{
public:
    set_qcombobox_idx_c(QComboBox *const box) :
        targetBox(box)
    {
        return;
    }

    // Finds the given string in the combobox, and sets the box's index to that entry.
    // If the string can't be found, the index will be set to 0.
    void by_string(const QString &string)
    {
        int idx = targetBox->findText(string, Qt::MatchExactly);
        if (idx == -1)
        {
            NBENE(("Unable to find a combo-box item called '%s'. Defaulting to the box's first entry.",
                   string.toStdString().c_str()));

            idx = 0;
        }

        targetBox->setCurrentIndex(idx);

        return;
    }

private:
    QComboBox *const targetBox;
};

#endif
