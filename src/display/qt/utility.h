/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * Miscellaneous Qt-related utility functions.
 *
 */

#ifndef VCS_DISPLAY_QT_UTILITY_H
#define VCS_DISPLAY_QT_UTILITY_H

#include <QComboBox>
#include <QWidget>
#include <QTimer>
#include "common/globals.h"

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
