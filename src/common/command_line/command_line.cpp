/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include <unistd.h>
#include "common/globals.h"

/*
 * TODOS:
 *
 * - a few global variables need to be walled off.
 *
 */

// Which input channel on the capture hardware we want to receive frames from.
uint INPUT_CHANNEL_IDX = 0;

// How many frames the capture card should drop between captures.
uint FRAME_SKIP = 0;

// Name of (and path to) the capture parameter file on disk.
static std::string VIDEO_PRESETS_FILE_NAME = "";

// Name of (and path to) the alias file on disk.
static std::string ALIAS_FILE_NAME = "";

// Name of (and path to) the filter set file on disk.
static std::string FILTER_GRAPH_FILE_NAME = "";

bool kcom_parse_command_line(const int argc, char *const argv[])
{
    const char parseFailMsg[] = "VCS has to exit because it found unexpected values "
                                "while parsing the command line. More information "
                                "will have been printed into the console. If a "
                                "console window was not already open, run VCS "
                                "again from the command line.";

    int c = 0;
    while ((c = getopt(argc, argv, "i:m:v:a:f:")) != -1)
    {
        switch (c)
        {
            case 'i':   // Capture input channel.
            {
                INPUT_CHANNEL_IDX = strtol(optarg, NULL, 10);
                
                // Validate.
                {
                    const unsigned minChannelIdx = 1;    // Values must be 1-indexed.
                    const unsigned maxChannelIdx = 8192; // Sanity check.

                    if ((INPUT_CHANNEL_IDX < minChannelIdx) ||
                        (INPUT_CHANNEL_IDX > maxChannelIdx))
                    {
                        NBENE(("Input channel index out of bounds. Expected range: %u-%u.",
                               minChannelIdx, maxChannelIdx));

                        kd_show_headless_error_message("", parseFailMsg);

                        return false;
                    }

                    // VCS expects the channel index to be 0-indexed, so let's convert.
                    INPUT_CHANNEL_IDX--;
                }

                break;
            }
            case 'v':   // Location of the video presets file.
            case 'm':   // ('m' provided for legacy compatibility)
            {
                VIDEO_PRESETS_FILE_NAME = optarg;

                break;
            }
            case 'a':   // Location of the alias file.
            {
                ALIAS_FILE_NAME = optarg;

                break;
            }
            case 'f':   // Location of the filter graph file.
            {
                FILTER_GRAPH_FILE_NAME = optarg;

                break;
            }
        }
    }

    return true;
}

void kcom_override_filter_graph_file_name(const std::string newFilename)
{
    FILTER_GRAPH_FILE_NAME = newFilename;

    return;
}

void kcom_override_aliases_file_name(const std::string newFilename)
{
    ALIAS_FILE_NAME = newFilename;

    return;
}

void kcom_override_video_presets_file_name(const std::string newFilename)
{
    VIDEO_PRESETS_FILE_NAME = newFilename;

    return;
}

const std::string& kcom_aliases_file_name(void)
{
    return ALIAS_FILE_NAME;
}

const std::string& kcom_filter_graph_file_name(void)
{
    return FILTER_GRAPH_FILE_NAME;
}

const std::string& kcom_video_presets_file_name(void)
{
    return VIDEO_PRESETS_FILE_NAME;
}
