/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QLABEL_MAGNIFYING_GLASS_H
#define VCS_DISPLAY_QT_SUBCLASSES_QLABEL_MAGNIFYING_GLASS_H

#include <QLabel>

class QImage;
class QPoint;

class MagnifyingGlass : public QLabel
{
    Q_OBJECT

public:
    explicit MagnifyingGlass(QWidget *parent = 0);
    void magnify(const QImage &fromImage, const QPoint &fromPosition);
};

#endif
