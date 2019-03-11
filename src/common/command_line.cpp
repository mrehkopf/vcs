/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include <QString>
#include <string>
#include <unistd.h>
#include "../common/common.h"
#include "../main.h"

/*
 * TODOS:
 *
 * - a few global variables need to be walled off.
 *
 */

unsigned int INPUT_CHANNEL_IDX = 1;         // Which input channel on the capture card we want to receive frames from.
unsigned int FRAME_SKIP = 0;                // How many frames the capture card should drop between captures.
static QString PARAMS_FILE_NAME = "";      // Name of (and path to) the capture parameter file on disk.
static QString ALIAS_FILE_NAME = "";       // Name of (and path to) the alias file on disk.
static QString FILTERS_FILE_NAME = "";     // Name of (and path to) the filter set file on disk.

bool kcom_parse_command_line(const int argc, char *const argv[])
{
    int c = 0;
    while ((c = getopt(argc, argv, "i:m:a:f:")) != -1)
    {
        switch (c)
        {
            case 'i':   // Capture input channel (>0).
            {
                INPUT_CHANNEL_IDX = strtol(optarg, NULL, 10);

                break;
            }
            case 'm':   // Location of the mode parameter file.
            {
                PARAMS_FILE_NAME = optarg;

                break;
            }
            case 'a':   // Location of the alias file.
            {
                ALIAS_FILE_NAME = optarg;

                break;
            }
            case 'f':   // Location of the filter set file.
            {
                FILTERS_FILE_NAME = optarg;

                break;
            }
        }
    }

    // Expect to get the input channel as 1-indexed on the command line.
    if (INPUT_CHANNEL_IDX == 0)
    {
        NBENE(("Detected an invalid input channel (0). The first capture channel "
               "is expected to be given as 1."));

        kd_show_headless_error_message("",
                                       "VCS has to exit because it found unexpected data while "
                                       "parsing the command line.\n\nMore information "
                                       "will have been printed into the console. If a "
                                       "console window was not already open, run VCS "
                                       "again from the command line.");

        return false;
    }

    // Convert to 0-indexed.
    INPUT_CHANNEL_IDX--;

    return true;
}

const QString& kcom_alias_file_name(void)
{
    return ALIAS_FILE_NAME;
}

const QString& kcom_filters_file_name(void)
{
    return FILTERS_FILE_NAME;
}

const QString& kcom_params_file_name(void)
{
    return PARAMS_FILE_NAME;
}
