/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_WHEEL_BLOCKER_H
#define VCS_DISPLAY_QT_WHEEL_BLOCKER_H

#include <QObject>

class WheelBlocker : public QObject
{
    Q_OBJECT

public:
    explicit WheelBlocker(QObject *parent = nullptr);

private:
    bool eventFilter(QObject *target, QEvent *event) override;

    QObject *const parent;
};

#endif
