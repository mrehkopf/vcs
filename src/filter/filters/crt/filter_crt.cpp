/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include <map>
#include "filter/filters/crt/filter_crt.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>

static const int INTERNAL_SCALE = 2;

static void apply_barrel_distortion(
    const double curvature,
    const double scale,
    uint8_t *const src,
    uint8_t *const dst,
    const resolution_s &resolution
)
{
    const double centerX = (resolution.w / 2.0);
    const double centerY = (resolution.h / 2.0);

    for (unsigned y = 0; y < resolution.h; y++)
    {
        for (unsigned x = 0; x < resolution.w; x++)
        {
            const unsigned bufferIdx = ((x + y * resolution.w) * 4);
            const double normX = ((x - centerX) / centerX);
            const double normY = ((y - centerY) / centerY);
            const double radius = std::sqrt(normX * normX + normY * normY);
            const double barrelFactor = (1 + curvature * radius * radius);
            const double barrelX = normX * barrelFactor;
            const double barrelY = normY * barrelFactor;
            const unsigned sourceX = std::round((barrelX * centerX / scale) + centerX);
            const unsigned sourceY = std::round((barrelY * centerY / scale) + centerY);

            if (
                (sourceX < resolution.w) &&
                (sourceY < resolution.h)
            ){
                const unsigned sourceIdx = (sourceX + sourceY * resolution.w) * 4;
                dst[bufferIdx + 0] = src[sourceIdx + 0];
                dst[bufferIdx + 1] = src[sourceIdx + 1];
                dst[bufferIdx + 2] = src[sourceIdx + 2];
            }
            else
            {
                dst[bufferIdx + 0] = 0;
                dst[bufferIdx + 1] = 0;
                dst[bufferIdx + 2] = 0;
            }
        }
    }

    return;
}

void filter_crt_c::apply(image_s *const image)
{
    ASSERT_FILTER_ARGUMENTS(image);

    static uint8_t *baseBuffer = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME * INTERNAL_SCALE];
    static uint8_t *baseGlowBuffer = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME * INTERNAL_SCALE];
    static uint8_t *barrelBuffer = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME * INTERNAL_SCALE];
    static uint8_t *phosphorBuffer = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME];

    const resolution_s scaledResolution = {
        (image->resolution.w * INTERNAL_SCALE),
        (image->resolution.h * INTERNAL_SCALE)
    };

    cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
    cv::Mat phosphor = cv::Mat(output.size(), CV_8UC4, phosphorBuffer);
    cv::Mat base = cv::Mat(scaledResolution.h, scaledResolution.w, CV_8UC4, baseBuffer);
    cv::Mat baseGlow = cv::Mat(scaledResolution.h, scaledResolution.w, CV_8UC4, baseGlowBuffer);

    // Phosphor decay.
    for (unsigned i = 0; i < image->byte_size(); i += 4)
    {
        phosphorBuffer[i+0] = LERP(phosphorBuffer[i+0], image->pixels[i+0], 0.96);
        phosphorBuffer[i+1] = LERP(phosphorBuffer[i+1], image->pixels[i+1], 0.94);
        phosphorBuffer[i+2] = LERP(phosphorBuffer[i+2], image->pixels[i+2], 0.91);
    }

    // Barrel distortion.
    {
        // Upscale the source image for more accurate results.
        cv::resize(phosphor, base, {scaledResolution.w, scaledResolution.h}, 0, 0, cv::INTER_LINEAR);

        memcpy(barrelBuffer, baseBuffer, (scaledResolution.w * scaledResolution.h * 4));
        memcpy(baseGlowBuffer, baseBuffer, (scaledResolution.w * scaledResolution.h * 4));

        apply_barrel_distortion(0.025, 1.0, barrelBuffer, baseBuffer, scaledResolution);
        apply_barrel_distortion(0.02, 1.005, barrelBuffer, baseGlowBuffer, scaledResolution);
    }

    baseGlow = (2.45 * baseGlow - cv::Scalar(35, 20, 20));
    cv::blur(baseGlow, baseGlow, cv::Size(5, 5));

    base = (base - cv::Scalar(20, 20, 30));
    cv::resize((base + (baseGlow * 0.3)), output, output.size(), 0, 0, cv::INTER_AREA);

    // Scanlines.
    {
        const double scanlineIntensity = 0.15;

        for (unsigned y = 0; y < image->resolution.h; y++)
        {
            for (unsigned x = 0; x < image->resolution.w; x++)
            {
                const unsigned bufferIdx = ((x + y * image->resolution.w) * 4);
                const double scanlineFactor = ((y % 2 == 0)? (1 - scanlineIntensity) : 1);

                image->pixels[bufferIdx] = (image->pixels[bufferIdx + 0] * scanlineFactor);
                image->pixels[bufferIdx + 1] = (image->pixels[bufferIdx + 1] * scanlineFactor);
                image->pixels[bufferIdx + 2] = (image->pixels[bufferIdx + 2] * scanlineFactor);
            }
        }
    }

    return;
}
