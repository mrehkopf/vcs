/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include <QPixmap>
#include <QPainter>
#include "display/qt/widgets/ColorHistogram.h"
#include "common/assert.h"

ColorHistogram::ColorHistogram(QWidget *parent) : QWidget(parent)
{
    this->update_pixmap_size();
    this->clear();

    return;
}

void ColorHistogram::refresh(const image_s &image, const ColorHistogram::channels channels)
{
    k_assert_optional((image.bytes_per_pixel() == 4), "Unsupported color depth.");

    this->redBins.fill(0);
    this->greenBins.fill(0);
    this->blueBins.fill(0);

    for (unsigned i = 0; i < image.byte_size(); i += image.bytes_per_pixel())
    {
        this->blueBins[image.pixels[i+0]]++;
        this->greenBins[image.pixels[i+1]]++;
        this->redBins[image.pixels[i+2]]++;
    }

    this->channelsToShow = channels;
    this->maxBinHeight = std::max({
        *std::max_element(this->redBins.begin(), this->redBins.end()),
        *std::max_element(this->greenBins.begin(), this->greenBins.end()),
        *std::max_element(this->blueBins.begin(), this->blueBins.end()),
    });

    this->redraw_pixmap();
    this->update();

    return;
}

void ColorHistogram::redraw_pixmap(void)
{
    this->histogram.fill(Qt::transparent);
    QPainter painter(&this->histogram);

    const double binWidth = (this->histogram.width() / double(numBins));

    const auto draw_bins = [&painter, binWidth, this](const std::array<unsigned, numBins> &bins, const QColor color)->void
    {
        const int maxHeight = (this->histogram.height() - 1);
        const int maxWidth = ((numBins - 1) * binWidth);

        QPolygon p;

        // We assume a max bin height of 0 means the source image was null.
        if (this->maxBinHeight <= 0)
        {
            // Fake a histogram where all pixels are black.
            p << QPoint(0, maxHeight) << QPoint(0, 0) << QPoint(binWidth, maxHeight) << QPoint(maxWidth, maxHeight);
        }
        else
        {
            p << QPoint(0, maxHeight);

            for (unsigned x = 0; x < numBins; x++)
            {
                const unsigned binHeight = ((bins[x] / double(this->maxBinHeight)) * maxHeight);
                p << QPoint((x * binWidth), (maxHeight - binHeight));
            }

            p << QPoint(maxWidth, maxHeight);
        }

        painter.setPen(QPen(color, 1, Qt::SolidLine));
        painter.setBrush(QBrush(color));
        painter.drawPolygon(p);
    };


    if (this->channelsToShow & ColorHistogram::channels::blue)
    {
        draw_bins(this->blueBins, "#157efd");
    }

    if (this->channelsToShow & ColorHistogram::channels::green)
    {
        draw_bins(this->greenBins, "#53d76a");
    }

    if (this->channelsToShow & ColorHistogram::channels::red)
    {
        draw_bins(this->redBins, "#fa3244");
    }

    return;
}

void ColorHistogram::clear(void)
{
    this->redBins.fill(0);
    this->greenBins.fill(0);
    this->blueBins.fill(0);

    this->histogram.fill(Qt::transparent);

    return;
}

void ColorHistogram::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, this->histogram);

    return;
}

void ColorHistogram::update_pixmap_size(void)
{
    this->histogram = QPixmap(
        std::max(1, this->width()),
        std::max(1, this->height())
    );

    this->redraw_pixmap();

    return;
}

void ColorHistogram::resizeEvent(QResizeEvent *)
{
    this->update_pixmap_size();

    return;
}
