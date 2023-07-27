/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QFRAME_QT_ABSTRACT_GUI_H
#define VCS_DISPLAY_QT_SUBCLASSES_QFRAME_QT_ABSTRACT_GUI_H

#include <QFrame>

struct abstract_gui_s;

// A Qt widget generated from an abstract GUI description.
class QtAbstractGUI : public QFrame
{
    Q_OBJECT

public:
    explicit QtAbstractGUI(const abstract_gui_s &gui);

signals:
    // Emitted when the state of a widget in the GUI is mutated.
    void mutated(QWidget *const widget);
};

#endif
