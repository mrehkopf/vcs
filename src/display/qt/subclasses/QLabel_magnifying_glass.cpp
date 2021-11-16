/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QImage>
#include <QPoint>
#include "display/qt/subclasses/QLabel_magnifying_glass.h"

MagnifyingGlass::MagnifyingGlass(QWidget *parent) :
    QLabel(parent)
{
    this->setStyleSheet(
        "background-color: rgba(0, 0, 0, 255);"
        "border-color: blue;"
        "border-style: solid;"
        "border-width: 3px;"
        "border-radius: 3px;"
    );

    this->hide();
                                
    return;
}

void MagnifyingGlass::magnify(const QImage &fromImage, const QPoint &fromPosition)
{
    /// TODO: Let the parent define these.
    static const unsigned magnification = 8;
    static const QSize regionSize = QSize(40, 30);
    static const QSize glassSize = QSize(
        (regionSize.width() * magnification),
        (regionSize.height() * magnification)
    );
    
    QPoint regionTopLeft = QPoint(
        (fromPosition.x() - (regionSize.width() / 2)),
        (fromPosition.y() - (regionSize.height() / 2))
    );

    // Don't let the magnification overflow the source image.
    regionTopLeft.setX(std::max(0, std::min(regionTopLeft.x(), (fromImage.width() - regionSize.width()))));
    regionTopLeft.setY(std::max(0, std::min(regionTopLeft.y(), (fromImage.height() - regionSize.height()))));

    // Grab the magnified region's pixel data.
    const unsigned startIdx = (regionTopLeft.x() + regionTopLeft.y() * fromImage.width()) * (fromImage.depth() / 8);
    const QImage magnified(
        (fromImage.bits() + startIdx),
        regionSize.width(),
        regionSize.height(),
        fromImage.bytesPerLine(),
        fromImage.format()
    );

    this->setPixmap(QPixmap::fromImage(
        magnified.scaled(glassSize.width(), glassSize.height(), Qt::IgnoreAspectRatio, Qt::FastTransformation))
    );

    this->resize(glassSize);

    // Center over the magnified region.
    this->move(
        std::min((this->parentWidget()->width() - glassSize.width() + 3), std::max(0, fromPosition.x() - (glassSize.width() / 2))),
        std::max(-3, std::min((this->parentWidget()->height() - glassSize.height()), fromPosition.y() - (glassSize.height() / 2)))
    );

    this->show();

    return;
}
