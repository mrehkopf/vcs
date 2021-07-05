/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/frame_rate/filter_frame_rate.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

// Counts the number of unique frames per second, i.e. frames in which the pixels
// change between frames by less than a set threshold (which is to account for
// analog capture artefacts).
void filter_frame_rate_c::apply(u8 *const pixels, const resolution_s &r)
{
    this->assert_input_validity(pixels, r);

    #ifdef USE_OPENCV
        static heap_mem<u8> prevPixels(MAX_NUM_BYTES_IN_CAPTURED_FRAME, "Frame rate filter buffer");
        static u32 uniqueFramesProcessed = 0;
        static u32 uniqueFramesPerSecond = 0;
        static auto timeElapsed = std::chrono::system_clock::now();

        const unsigned threshold = this->parameter(PARAM_THRESHOLD);
        const unsigned corner = this->parameter(PARAM_CORNER);
        const unsigned bgColorType = this->parameter(PARAM_BG_COLOR);
        const unsigned textColorType = this->parameter(PARAM_TEXT_COLOR);

        for (u32 i = 0; i < (r.w * r.h); i++)
        {
            const u32 idx = (i * (r.bpp / 8));

            if ((abs(pixels[idx + 0] - prevPixels[idx + 0]) >= threshold) ||
                (abs(pixels[idx + 1] - prevPixels[idx + 1]) >= threshold) ||
                (abs(pixels[idx + 2] - prevPixels[idx + 2]) >= threshold))
            {
                uniqueFramesProcessed++;
                break;
            }
        }

        memcpy(prevPixels.data(),
               pixels,
               prevPixels.size_check(r.w * r.h * (r.bpp / 8)));

        const auto timeNow = std::chrono::system_clock::now();
        const double secsElapsed = (std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - timeElapsed).count() / 1000.0);
        if (secsElapsed >= 1)
        {
            uniqueFramesPerSecond = std::round(uniqueFramesProcessed / secsElapsed);
            uniqueFramesProcessed = 0;
            timeElapsed = std::chrono::system_clock::now();
        }

        // Draw the counter into the frame.
        {
            const unsigned margin = 7;
            std::string counterString = std::to_string(uniqueFramesPerSecond);
            cv::Size textSize = cv::getTextSize(counterString, cv::FONT_HERSHEY_DUPLEX, 1, 2, nullptr);
            cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);

            const cv::Point cornerPos = ([&]()
            {
                switch (corner)
                {
                    case TOP_RIGHT: return cv::Point((r.w - textSize.width - margin), (textSize.height + margin));
                    case BOTTOM_RIGHT: return cv::Point((r.w - textSize.width - margin), (r.h - margin));
                    case BOTTOM_LEFT: return cv::Point(margin, (r.h - margin));

                    // Top left.
                    default: return cv::Point(margin, (textSize.height + margin));
                }
            })();

            if (bgColorType != BG_TRANSPARENT)
            {
                cv::Scalar bgColor;
                const cv::Point bgRectTopLeft = cv::Point(cornerPos.x - margin, cornerPos.y + margin);
                const cv::Point bgRectBottomRight = cv::Point((cornerPos.x + textSize.width + margin), (cornerPos.y - textSize.height - margin));

                switch (bgColorType)
                {
                    case BG_BLACK: bgColor = cv::Scalar(0, 0, 0); break;
                    case BG_WHITE: bgColor = cv::Scalar(255, 255, 255); break;
                }

                cv::rectangle(output, bgRectTopLeft, bgRectBottomRight, bgColor, cv::FILLED);
            }

            cv::Scalar textColor;

            switch (textColorType)
            {
                case TEXT_YELLOW: textColor = cv::Scalar(0, 255, 255); break;
                case TEXT_PURPLE: textColor = cv::Scalar(255, 0, 255); break;
                case TEXT_BLACK:  textColor = cv::Scalar(0, 0, 0); break;
                case TEXT_WHITE:  textColor = cv::Scalar(255, 255, 255); break;
            }

            cv::putText(output, counterString, cornerPos, cv::FONT_HERSHEY_DUPLEX, 1, textColor, 2, cv::LINE_AA);
        }
    #endif

    return;
}
