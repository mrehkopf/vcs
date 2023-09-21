/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_WIDGETS_COLORHISTOGRAM_H
#define VCS_DISPLAY_QT_WIDGETS_COLORHISTOGRAM_H

#include <QWidget>
#include "display/display.h"

class ColorHistogram : public QWidget
{
    Q_OBJECT

public:
    enum channels : unsigned
    {
        red   = 0b1,
        green = 0b10,
        blue  = 0b100,
        rgb   = 0b111,
    };

    explicit ColorHistogram(QWidget *parent = 0);

    void refresh(const image_s &image, const channels channels = channels::rgb);
    void clear(void);

private:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);

    void update_pixmap_size(void);
    void redraw_pixmap(void);

    QPixmap histogram = QPixmap(1, 1);
    unsigned maxBinHeight = 1;
    channels channelsToShow = channels::rgb;
    static const unsigned numBins = 256;
    std::array<unsigned, numBins> redBins;
    std::array<unsigned, numBins> greenBins;
    std::array<unsigned, numBins> blueBins;
};

#endif
