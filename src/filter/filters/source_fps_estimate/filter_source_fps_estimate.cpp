/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/source_fps_estimate/filter_source_fps_estimate.h"
#include "filter/filters/render_text/filter_render_text.h"
#include "filter/filters/render_text/font_5x3.h"

static const auto FONT = font_5x3_c();
static const unsigned TEXT_SIZE = 4;

// Counts the number of unique frames per second, i.e. frames in which the pixels
// change between frames by less than a set threshold (which is to account for
// analog capture artefacts).
void filter_frame_rate_c::apply(u8 *const pixels, const resolution_s &r)
{
    this->assert_input_validity(pixels, r);

    static heap_mem<uint8_t> prevPixels(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Frame rate filter buffer");
    static auto timeElapsed = std::chrono::system_clock::now();
    static unsigned numUniqueFramesProcessed = 0;
    static unsigned estimatedFPS = 0;

    const unsigned threshold = this->parameter(PARAM_THRESHOLD);
    const unsigned cornerId = this->parameter(PARAM_CORNER);
    const unsigned fgColorId = this->parameter(PARAM_TEXT_COLOR);
    const std::vector<uint8_t> bgColor = (this->parameter(PARAM_BG_OPAQUE)? std::vector<uint8_t>{0, 0, 0} : std::vector<uint8_t>{});

    // Find out whether any pixel in the current frame differs from the previous frame
    // by more than the threshold.
    {
        for (unsigned i = 0; i < (r.w * r.h); i += (r.bpp / 8))
        {
            if (
                (abs(pixels[i + 0] - prevPixels[i + 0]) >= threshold) ||
                (abs(pixels[i + 1] - prevPixels[i + 1]) >= threshold) ||
                (abs(pixels[i + 2] - prevPixels[i + 2]) >= threshold)
            ){
                numUniqueFramesProcessed++;
                break;
            }
        }

        memcpy(prevPixels.data(), pixels, prevPixels.size_check(r.w * r.h * (r.bpp / 8)));

        const auto timeNow = std::chrono::system_clock::now();
        const double secsElapsed = (std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - timeElapsed).count() / 1000.0);
        if (secsElapsed >= 1)
        {
            estimatedFPS = std::round(numUniqueFramesProcessed / secsElapsed);
            numUniqueFramesProcessed = 0;
            timeElapsed = std::chrono::system_clock::now();
        }
    }

    // Draw the FPS counter into the current frame.
    {
        const std::string outputString = ('~' + std::to_string(estimatedFPS));

        const auto [x, y] = ([cornerId, &outputString, &r]()->std::pair<unsigned, unsigned>
        {
            const unsigned textWidth = (TEXT_SIZE * FONT.width_of(outputString));
            const unsigned textHeight = (TEXT_SIZE * FONT.height_of(outputString));

            switch (cornerId)
            {
                default:
                case TOP_LEFT: return {0, 0};
                case TOP_RIGHT: return {(r.w - textWidth), 0};
                case BOTTOM_RIGHT: return {(r.w - textWidth), (r.h - textHeight)};
                case BOTTOM_LEFT: return {0, (r.h - textHeight)};
            }
        })();

        const auto fgColor = ([fgColorId]()->std::vector<uint8_t>
        {
            switch (fgColorId)
            {
                default:
                case TEXT_YELLOW: return {0, 255, 255};
                case TEXT_PURPLE: return {255, 0, 255};
                case TEXT_BLACK: return {0, 0, 0};
                case TEXT_WHITE: return {255, 255, 255};
            }
        })();

        FONT.render(outputString, {pixels, r}, x, y, TEXT_SIZE, fgColor, bgColor);
    }

    return;
}
