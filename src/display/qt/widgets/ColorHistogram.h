/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_WIDGETS_COLORHISTOGRAM_H
#define VCS_DISPLAY_QT_WIDGETS_COLORHISTOGRAM_H

#include <QWidget>
#include <QFuture>
#include <QFutureWatcher>
#include "display/display.h"

class ColorHistogram : public QWidget
{
    Q_OBJECT

public:
    explicit ColorHistogram(QWidget *parent = 0);
    ~ColorHistogram();

    void refresh(const image_s &image);
    void clear(void);

private:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);

    void update_pixmap_size(void);
    void redraw_graph(void);

    QVector<QPixmap> histogram{2};
    QPixmap *frontBuffer = &histogram[0];
    QPixmap *backBuffer = &histogram[1];

    bool isEmpty = true;

    unsigned maxBinHeight = 1;
    static const unsigned numBins = 256;
    std::array<unsigned, numBins> redBins;
    std::array<unsigned, numBins> greenBins;
    std::array<unsigned, numBins> blueBins;

    QFutureWatcher<void> graphingThread;
};

#endif
