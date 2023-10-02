/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QApplication>
#include <QComboBox>
#include <QSpinBox>
#include "display/qt/widgets/HorizontalSlider.h"
#include "display/qt/wheel_blocker.h"
#include "common/assert.h"

WheelBlocker::WheelBlocker(QObject *parent) :
    parent(parent)
{
    k_assert(parent, "Expected a non-null parent.");

    const auto install_filter = [this](const auto widgets)
    {
        for (QWidget *w: widgets)
        {
            w->installEventFilter(this);
            w->setFocusPolicy(Qt::StrongFocus);
        }
    };

    install_filter(parent->findChildren<QSpinBox*>());
    install_filter(parent->findChildren<QDoubleSpinBox*>());
    install_filter(parent->findChildren<QComboBox*>());
    install_filter(parent->findChildren<QComboBox*>());
    install_filter(parent->findChildren<HorizontalSlider*>());

    return;
}

bool WheelBlocker::eventFilter(QObject *target, QEvent *event)
{
    if (
        (target != this->parent) &&
        (event->type() == QEvent::Wheel) &&
        !(qApp->keyboardModifiers() & Qt::ShiftModifier)
    ){
        // Have the wheel scroll the parent and ignore the child.
        qApp->sendEvent(this->parent, event);
        return true;
    }

    // Let other events travel down to their respective objects.
    return false;
}
