/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include <QPixmap>
#include <QPainter>
#include <QtConcurrent/QtConcurrent>
#include "display/qt/widgets/ColorHistogram.h"
#include "common/assert.h"

ColorHistogram::ColorHistogram(QWidget *parent) : QWidget(parent)
{
    this->update_pixmap_size();
    this->clear();

    connect(&this->graphingThread, &QFutureWatcher<void>::finished, this, [this]
    {
        this->update();
    });

    return;
}

ColorHistogram::~ColorHistogram()
{
    this->graphingThread.waitForFinished();

    return;
}

void ColorHistogram::refresh(const image_s &image)
{
    static uint8_t *const scratchBuffer = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    k_assert_optional((image.bytes_per_pixel() == 4), "Unsupported color depth.");

    if (!this->graphingThread.isFinished())
    {
        return;
    }

    std::memcpy(scratchBuffer, image.pixels, image.byte_size());
    const unsigned imageDataLen = image.byte_size();
    const unsigned imageBytesPerPixel = image.bytes_per_pixel();

    graphingThread.setFuture(QtConcurrent::run([this, imageDataLen, imageBytesPerPixel]{
        this->redBins.fill(0);
        this->greenBins.fill(0);
        this->blueBins.fill(0);

        for (unsigned i = 0; i < imageDataLen; i += imageBytesPerPixel)
        {
            this->blueBins[scratchBuffer[i+0]]++;
            this->greenBins[scratchBuffer[i+1]]++;
            this->redBins[scratchBuffer[i+2]]++;
        }

        this->maxBinHeight = std::max({
            *std::max_element(this->redBins.begin(), this->redBins.end()),
            *std::max_element(this->greenBins.begin(), this->greenBins.end()),
            *std::max_element(this->blueBins.begin(), this->blueBins.end()),
        });

        this->isEmpty = false;
        this->redraw_graph();
    }));

    return;
}

void ColorHistogram::redraw_graph(void)
{
    QPixmap &histogram = *this->backBuffer;
    histogram.fill(Qt::transparent);

    if (this->isEmpty)
    {
        std::swap(this->frontBuffer, this->backBuffer);
        return;
    }

    QPainter painter(&histogram);

    const double binWidth = (histogram.width() / double(numBins));

    const auto draw_bins = [&painter, binWidth, &histogram, this](const std::array<unsigned, numBins> &bins, const QColor color)->void
    {
        const int maxHeight = (histogram.height() - 1);
        const int maxWidth = ((numBins - 1) * binWidth);

        QPolygon p;

        // We assume a max bin height of 0 means the source image was null.
        if (this->maxBinHeight > 0)
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

    draw_bins(this->blueBins, "#157efd");
    draw_bins(this->greenBins, "#53d76a");
    draw_bins(this->redBins, "#fa3244");

    std::swap(this->frontBuffer, this->backBuffer);

    return;
}

void ColorHistogram::clear(void)
{
    graphingThread.waitForFinished();

    this->isEmpty = true;
    this->redBins.fill(0);
    this->greenBins.fill(0);
    this->blueBins.fill(0);
    frontBuffer->fill(Qt::transparent);

    this->update();

    return;
}

void ColorHistogram::paintEvent(QPaintEvent *)
{
    if (!this->isEmpty)
    {
        QPainter painter(this);
        painter.drawPixmap(0, 0, *this->frontBuffer);
    }

    return;
}

void ColorHistogram::update_pixmap_size(void)
{
    graphingThread.waitForFinished();

    *frontBuffer = QPixmap(
        std::max(1, this->width()),
        std::max(1, this->height())
    );

    *backBuffer = QPixmap(
        std::max(1, this->width()),
        std::max(1, this->height())
    );

    this->redraw_graph();

    return;
}

void ColorHistogram::resizeEvent(QResizeEvent *)
{
    this->update_pixmap_size();

    return;
}
