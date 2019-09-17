#include "common/disk_legacy.h"
#include "common/file_writer.h"
#include "common/propagate.h"
#include "capture/capture.h"
#include "filter/filter.h"
#include "filter/filter_legacy.h"
#include "common/disk.h"
#include "common/csv.h"

bool kdisk_legacy14_save_filter_sets(const std::vector<legacy14_filter_set_s*> &filterSets,
                                     const QString &targetFilename)
{
    file_writer_c outFile(targetFilename);

    for (const auto *set: filterSets)
    {
        // Save the resolutions.
        {
            // Encode the filter set's activation in the resolution values, where 0
            // means the set activates for all resolutions.
            resolution_s inRes = set->inRes, outRes = set->outRes;
            if (set->activation & legacy14_filter_set_s::activation_e::all)
            {
                inRes = {0, 0};
                outRes = {0, 0};
            }
            else
            {
                if (!(set->activation & legacy14_filter_set_s::activation_e::in)) inRes = {0, 0};
                if (!(set->activation & legacy14_filter_set_s::activation_e::out)) outRes = {0, 0};
            }

            outFile << "inout,"
                    << inRes.w << "," << inRes.h << ","
                    << outRes.w << "," << outRes.h << "\n";
        }

        outFile << "description,{" << QString::fromStdString(set->description) << "}\n";
        outFile << "enabled," << set->isEnabled << "\n";
        outFile << "scaler,{" << QString::fromStdString(set->scaler->name) << "}\n";

        // Save the filters.
        auto save_filter_data = [&](std::vector<legacy14_filter_s> filters, const QString &filterType)
        {
            for (auto &filter: filters)
            {
                outFile << filterType << ",{" << QString::fromStdString(filter.uuid) << "}," << FILTER_PARAMETER_ARRAY_LENGTH;
                for (uint q = 0; q < FILTER_PARAMETER_ARRAY_LENGTH; q++)
                {
                    outFile << "," << (u8)filter.data[q];
                }
                outFile << "\n";
            }
        };
        outFile << "preFilters," << set->preFilters.size() << "\n";
        save_filter_data(set->preFilters, "pre");

        outFile << "postFilters," << set->postFilters.size() << "\n";
        save_filter_data(set->postFilters, "post");

        outFile << "\n";
    }

    if (!outFile.is_valid() ||
        !outFile.save_and_close())
    {
        NBENE(("Failed to write filter sets to file."));
        goto fail;
    }

    kpropagate_saved_filter_sets_to_disk(filterSets, targetFilename.toStdString());

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the filter "
                                   "sets for saving. No data was saved. \n\nMore "
                                   "information about the error may be found in the terminal.");
    return false;
}

/// TODO. Needs cleanup.
std::vector<legacy14_filter_set_s*> kdisk_legacy14_load_filter_sets(const std::string &sourceFilename)
{
    std::vector<legacy14_filter_set_s*> filterSets;

    if (sourceFilename.empty())
    {
        INFO(("No filter set file defined, skipping."));
        return {};
    }

    QList<QStringList> rowData = csv_parse_c(QString::fromStdString(sourceFilename)).contents();
    if (rowData.isEmpty())
    {
        goto fail;
    }

    // Each mode is saved as a block of rows, starting with a 3-element row defining
    // the mode's resolution, followed by several 2-element rows defining the various
    // video and color parameters for the resolution.
    for (int row = 0; row < rowData.count();)
    {
        legacy14_filter_set_s *set = new legacy14_filter_set_s;

        if ((rowData[row].count() != 5) ||
            (rowData[row].at(0) != "inout"))
        {
            NBENE(("Expected a 5-parameter 'inout' statement to begin a filter set block."));
            goto fail;
        }
        set->inRes.w = rowData[row].at(1).toUInt();
        set->inRes.h = rowData[row].at(2).toUInt();
        set->outRes.w = rowData[row].at(3).toUInt();
        set->outRes.h = rowData[row].at(4).toUInt();

        set->activation = 0;
        if (set->inRes.w == 0 && set->inRes.h == 0 &&
            set->outRes.w == 0 && set->outRes.h == 0)
        {
            set->activation |= legacy14_filter_set_s::activation_e::all;
        }
        else
        {
            if (set->inRes.w != 0 && set->inRes.h != 0) set->activation |= legacy14_filter_set_s::activation_e::in;
            if (set->outRes.w != 0 && set->outRes.h != 0) set->activation |= legacy14_filter_set_s::activation_e::out;
        }

        #define verify_first_element_on_row_is(name) if (rowData[row].at(0) != name)\
                                                     {\
                                                        NBENE(("Error while loading filter set file: expected '%s' but got '%s'.",\
                                                               name, rowData[row].at(0).toStdString().c_str()));\
                                                        goto fail;\
                                                     }

        row++;
        if (rowData[row].at(0) != "enabled") // Legacy support, 'description' was pushed in front of 'enabled' in later revisions.
        {
            verify_first_element_on_row_is("description");
            set->description = rowData[row].at(1).toStdString();

            row++;
        }
        else
        {
            set->description = "";
        }

        verify_first_element_on_row_is("enabled");
        set->isEnabled = rowData[row].at(1).toInt();

        row++;
        verify_first_element_on_row_is("scaler");
        set->scaler = ks_scaler_for_name_string(rowData[row].at(1).toStdString());

        // Takes in a proposed UUID string and returns a valid version of it. Provides backwards-
        // compatibility with older versions of VCS, which saved filters by name rather than by
        // UUID.
        const auto uuid_string = [](const QString proposedString)->std::string
        {
            // Replace legacy VCS filter names with their corresponding UUIDs.
            if (proposedString == "Delta Histogram") return "fc85a109-c57a-4317-994f-786652231773";
            if (proposedString == "Unique Count")    return "badb0129-f48c-4253-a66f-b0ec94e225a0";
            if (proposedString == "Unsharp Mask")    return "03847778-bb9c-4e8c-96d5-0c10335c4f34";
            if (proposedString == "Blur")            return "a5426f2e-b060-48a9-adf8-1646a2d3bd41";
            if (proposedString == "Decimate")        return "eb586eb4-2d9d-41b4-9e32-5cbcf0bbbf03";
            if (proposedString == "Denoise")         return "94adffac-be42-43ac-9839-9cc53a6d615c";
            if (proposedString == "Denoise (NLM)")   return "e31d5ee3-f5df-4e7c-81b8-227fc39cbe76";
            if (proposedString == "Sharpen")         return "1c25bbb1-dbf4-4a03-93a1-adf24b311070";
            if (proposedString == "Median")          return "de60017c-afe5-4e5e-99ca-aca5756da0e8";
            if (proposedString == "Crop")            return "2448cf4a-112d-4d70-9fc1-b3e9176b6684";
            if (proposedString == "Flip")            return "80a3ac29-fcec-4ae0-ad9e-bbd8667cc680";
            if (proposedString == "Rotate")          return "140c514d-a4b0-4882-abc6-b4e9e1ff4451";

            // Otherwise, we'll assume the string is already a valid UUID.
            return proposedString.toStdString();
        };

        row++;
        verify_first_element_on_row_is("preFilters");
        const uint numPreFilters = rowData[row].at(1).toUInt();
        for (uint i = 0; i < numPreFilters; i++)
        {
            row++;
            verify_first_element_on_row_is("pre");

            legacy14_filter_s filter;

            filter.uuid = uuid_string(rowData[row].at(1));
            filter.name = kf_filter_name_for_uuid(filter.uuid);

            const uint numParams = rowData[row].at(2).toUInt();
            if (numPreFilters >= FILTER_PARAMETER_ARRAY_LENGTH)
            {
                NBENE(("Too many parameters specified for a filter."));
                goto fail;
            }

            for (uint p = 0; p < numParams; p++)
            {
                uint datum = rowData[row].at(3 + p).toInt();
                if (datum > 255)
                {
                    NBENE(("A filter parameter had a value outside of the range allowed (0..255)."));
                    goto fail;
                }
                filter.data[p] = datum;
            }

            set->preFilters.push_back(filter);
        }

        /// TODO. Code duplication.
        row++;
        verify_first_element_on_row_is("postFilters");
        const uint numPostFilters = rowData[row].at(1).toUInt();
        for (uint i = 0; i < numPostFilters; i++)
        {
            row++;
            verify_first_element_on_row_is("post");

            legacy14_filter_s filter;

            filter.uuid = uuid_string(rowData[row].at(1));
            filter.name = kf_filter_name_for_uuid(filter.uuid);

            const uint numParams = rowData[row].at(2).toUInt();
            if (numPreFilters >= FILTER_PARAMETER_ARRAY_LENGTH)
            {
                NBENE(("Too many parameters specified for a filter."));
                goto fail;
            }

            for (uint p = 0; p < numParams; p++)
            {
                uint datum = rowData[row].at(3 + p).toInt();
                if (datum > 255)
                {
                    NBENE(("A filter parameter had a value outside of the range allowed (0..255)."));
                    goto fail;
                }
                filter.data[p] = datum;
            }

            set->postFilters.push_back(filter);
        }

        #undef verify_first_element_on_row_is

        row++;

        filterSets.push_back(set);
    }

    kf_clear_filters();
    for (auto *const set: filterSets) kf_add_filter_set(set);

    kpropagate_loaded_filter_sets_from_disk(filterSets, sourceFilename);

    return filterSets;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading filter "
                                   "sets. No data was loaded.\n\nMore "
                                   "information about the error may be found in the terminal.");
    return {};
}
