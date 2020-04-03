/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS disk access
 *
 * Provides functions for loading and saving data to and from files on disk.
 *
 */

#include <QTextStream>
#include <QFileInfo>
#include <QString>
#include <QDebug>
#include <QFile>
#include "display/qt/subclasses/InteractibleNodeGraphNode_filter_graph_nodes.h"
#include "display/qt/widgets/filter_widgets.h"
#include "common/disk/file_writer.h"
#include "common/disk/file_writer_filter_graph.h"
#include "common/propagate/propagate.h"
#include "capture/capture.h"
#include "capture/alias.h"
#include "filter/filter.h"
#include "filter/filter_legacy.h"
#include "common/disk/disk.h"
#include "common/disk/disk_legacy.h"
#include "common/disk/csv.h"

bool kdisk_save_video_signal_parameters(const std::vector<video_signal_parameters_s> &params,
                                        const std::string &targetFilename)
{
    file_writer_c outFile(targetFilename);

    // Each mode params block consists of two values specifying the resolution
    // followed by a set of string-value pairs for the different parameters.
    for (const auto &p: params)
    {
        // Resolution.
        outFile << "resolution," << p.r.w << "," << p.r.h << "\n";

        // Video params.
        outFile << "vPos,"   << p.verticalPosition << "\n"
                << "hPos,"   << p.horizontalPosition << "\n"
                << "hScale," << p.horizontalScale << "\n"
                << "phase,"  << p.phase << "\n"
                << "bLevel," << p.blackLevel << "\n";

        // Color params.
        outFile << "bright,"  << p.overallBrightness << "\n"
                << "contr,"   << p.overallContrast << "\n"
                << "redBr,"   << p.redBrightness << "\n"
                << "redCn,"   << p.redContrast << "\n"
                << "greenBr," << p.greenBrightness << "\n"
                << "greenCn," << p.greenContrast << "\n"
                << "blueBr,"  << p.blueBrightness << "\n"
                << "blueCn,"  << p.blueContrast << "\n";

        // Separate the next block.
        outFile << "\n";
    }

    if (!outFile.is_valid() ||
        !outFile.save_and_close())
    {
        NBENE(("Failed to write mode params to file."));
        goto fail;
    }

    kpropagate_saved_video_signal_parameters_to_disk(params, targetFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the mode "
                                   "settings for saving. As a result, no data was saved. \n\nMore "
                                   "information about this error may be found in the terminal.");
    return false;
}

bool kdisk_load_video_signal_parameters(const std::string &sourceFilename)
{
    if (sourceFilename.empty())
    {
        DEBUG(("No mode settings file defined, skipping."));
        return true;
    }

    DEBUG(("Loading video mode parameters from %s...", sourceFilename.c_str()));

    std::vector<video_signal_parameters_s> videoModeParams;

    QList<QStringList> paramRows = csv_parse_c(QString::fromStdString(sourceFilename)).contents();

    // Each mode is saved as a block of rows, starting with a 3-element row defining
    // the mode's resolution, followed by several 2-element rows defining the various
    // video and color parameters for the resolution.
    for (int i = 0; i < paramRows.count();)
    {
        if ((paramRows.at(i).count() != 3) ||
            (paramRows.at(i).at(0) != "resolution"))
        {
            NBENE(("Expected a 3-parameter 'resolution' statement to begin a mode params block."));
            goto fail;
        }

        video_signal_parameters_s p;
        p.r.w = paramRows.at(i).at(1).toUInt();
        p.r.h = paramRows.at(i).at(2).toUInt();

        i++;    // Move to the next row to start fetching the params for this resolution.

        auto get_param = [&](const QString &name)->QString
                         {
                             if ((int)i >= paramRows.length())
                             {
                                 NBENE(("Error while loading video parameters: expected '%s' but found the data out of range.", name.toLatin1().constData()));
                                 throw 0;
                             }
                             else if (paramRows.at(i).at(0) != name)
                             {
                                 NBENE(("Error while loading video parameters: expected '%s' but got '%s'.",
                                        name.toLatin1().constData(), paramRows.at(i).at(0).toLatin1().constData()));
                                 throw 0;
                             }
                             return paramRows[i++].at(1);
                         };
        try
        {
            // Note: the order in which the params are fetched is fixed to the
            // order in which they were saved.
            p.verticalPosition   = get_param("vPos").toInt();
            p.horizontalPosition = get_param("hPos").toInt();
            p.horizontalScale    = get_param("hScale").toUInt();
            p.phase              = get_param("phase").toInt();
            p.blackLevel         = get_param("bLevel").toInt();
            p.overallBrightness  = get_param("bright").toInt();
            p.overallContrast    = get_param("contr").toInt();
            p.redBrightness      = get_param("redBr").toInt();
            p.redContrast        = get_param("redCn").toInt();
            p.greenBrightness    = get_param("greenBr").toInt();
            p.greenContrast      = get_param("greenCn").toInt();
            p.blueBrightness     = get_param("blueBr").toInt();
            p.blueContrast       = get_param("blueCn").toInt();
        }
        catch (int)
        {
            NBENE(("Failed to load mode params from disk."));
            goto fail;
        }

        videoModeParams.push_back(p);
    }

    // Sort the modes so they'll display more nicely in the GUI.
    std::sort(videoModeParams.begin(), videoModeParams.end(), [](const video_signal_parameters_s &a, const video_signal_parameters_s &b)
                                          { return (a.r.w * a.r.h) < (b.r.w * b.r.h); });

    kpropagate_loaded_video_signal_parameters_from_disk(videoModeParams, sourceFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading video parameters. "
                                   "No data was loaded.\n\nMore information about the error "
                                   "may be found in the terminal.");
    return false;
}

bool kdisk_load_aliases(const std::string &sourceFilename)
{
    if (sourceFilename.empty())
    {
        DEBUG(("No alias file defined, skipping."));
        return true;
    }

    DEBUG(("Loading aliases from %s...", sourceFilename.c_str()));

    std::vector<mode_alias_s> aliases;

    QList<QStringList> rowData = csv_parse_c(QString::fromStdString(sourceFilename)).contents();
    for (const auto &row: rowData)
    {
        if (row.count() != 4)
        {
            NBENE(("Expected a 4-parameter row in the alias file."));
            goto fail;
        }

        mode_alias_s a;
        a.from.w = row.at(0).toUInt();
        a.from.h = row.at(1).toUInt();
        a.to.w = row.at(2).toUInt();
        a.to.h = row.at(3).toUInt();

        aliases.push_back(a);
    }

    // Sort the parameters so they display more nicely in the GUI.
    std::sort(aliases.begin(), aliases.end(), [](const mode_alias_s &a, const mode_alias_s &b)
                                              { return (a.to.w * a.to.h) < (b.to.w * b.to.h); });

    kpropagate_loaded_aliases_from_disk(aliases, sourceFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading aliases. No data was "
                                   "loaded.\n\nMore information about the error may be found in "
                                   "the terminal.");
    return false;
}

bool kdisk_save_aliases(const std::vector<mode_alias_s> &aliases,
                        const std::string &targetFilename)
{
    file_writer_c outFile(targetFilename);

    for (const auto &a: aliases)
    {
        outFile << a.from.w << "," << a.from.h << ","
                << a.to.w << "," << a.to.h << ",\n";
    }

    if (!outFile.is_valid() ||
        !outFile.save_and_close())
    {
        NBENE(("Failed to write aliases to file."));
        goto fail;
    }

    kpropagate_saved_aliases_to_disk(aliases, targetFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the alias "
                                   "resolutions for saving. As a result, no data was saved. \n\nMore "
                                   "information about this error may be found in the terminal.");
    return false;
}

bool kdisk_save_filter_graph(std::vector<FilterGraphNode*> &nodes,
                             std::vector<filter_graph_option_s> &graphOptions,
                             const std::string &targetFilename)
{
    if (!file_writer::filter_graph::version_b::write(targetFilename, nodes, graphOptions))
    {
        goto fail;
    }

    kpropagate_saved_filter_graph_to_disk(targetFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing filter graph data for saving. "
                                   "As a result, no data was saved. \n\nMore information about this "
                                   "error may be found in the terminal.");
    return false;
}

bool kdisk_load_filter_graph(const std::string &sourceFilename)
{
    if (sourceFilename.empty())
    {
        DEBUG(("No filter graph file defined, skipping."));
        return true;
    }

    DEBUG(("Loading filter graph data from %s...", sourceFilename.c_str()));

    std::vector<FilterGraphNode*> graphNodes;
    std::vector<filter_graph_option_s> graphOptions;

    kd_clear_filter_graph();

    // Load from a legacy (prior to VCS v1.5) filters file.
    if (QFileInfo(QString::fromStdString(sourceFilename)).suffix() == "vcs-filtersets")
    {
        // Used to position successive nodes horizontally.
        int nodeXOffset = 0;
        int nodeYOffset = 0;
        unsigned tallestNode = 0;

        u8 *const filterParamsTmp = new u8[FILTER_PARAMETER_ARRAY_LENGTH];
        const std::vector<legacy14_filter_set_s*> legacyFiltersets = kdisk_legacy14_load_filter_sets(sourceFilename);

        const auto add_node = [&](const filter_type_enum_e type, const u8 *const params)
        {
            FilterGraphNode *const newNode = kd_add_filter_graph_node(type, params);
            newNode->setPos(nodeXOffset, nodeYOffset);
            graphNodes.push_back(newNode);

            nodeXOffset += (newNode->width + 50);

            if (newNode->height > tallestNode)
            {
                tallestNode = newNode->height;
            }

            return;
        };

        for (const legacy14_filter_set_s *const filterSet: legacyFiltersets)
        {
            // Create the filter nodes.
            {
                // Add the input gate node.
                {
                    memset(filterParamsTmp, 0, FILTER_PARAMETER_ARRAY_LENGTH);
                    *(u16*)&(filterParamsTmp[0]) = filterSet->inRes.w;
                    *(u16*)&(filterParamsTmp[2]) = filterSet->inRes.h;
                    add_node(filter_type_enum_e::input_gate, filterParamsTmp);
                }

                // Add the regular filter nodes.
                {
                    for (const legacy14_filter_s &filter: filterSet->preFilters)
                    {
                        add_node(kf_filter_type_for_id(filter.uuid), filter.data);
                    }

                    for (const legacy14_filter_s &filter: filterSet->postFilters)
                    {
                        add_node(kf_filter_type_for_id(filter.uuid), filter.data);
                    }
                }

                // Add the output gate node.
                {
                    memset(filterParamsTmp, 0, FILTER_PARAMETER_ARRAY_LENGTH);
                    *(u16*)&(filterParamsTmp[0]) = filterSet->outRes.w;
                    *(u16*)&(filterParamsTmp[2]) = filterSet->outRes.h;
                    add_node(filter_type_enum_e::output_gate, filterParamsTmp);
                }
            }

            // Connect the filter nodes.
            {
                for (unsigned i = 0; i < (graphNodes.size() - 1); i++)
                {
                    auto sourceEdge = graphNodes.at(i)->output_edge();
                    auto targetEdge = graphNodes.at(i+1)->input_edge();

                    k_assert((sourceEdge && targetEdge), "A filter node is missing a required edge.");

                    sourceEdge->connect_to(targetEdge);
                }
            }

            graphNodes.clear();

            nodeYOffset += (tallestNode + 50);
            nodeXOffset = 0;
        }

        delete [] filterParamsTmp;
    }
    // Load from a normal filters file.
    else
    {
        #define verify_first_element_on_row_is(name) if ((int)row >= rowData.length())\
                                                     {\
                                                         NBENE(("Error while loading the filter graph file: expected '%s' on line "\
                                                                "#%d but found the data out of range.", name, (row+1)));\
                                                         goto fail;\
                                                     }\
                                                     else if (rowData.at(row).at(0) != name)\
                                                     {\
                                                        NBENE(("Error while loading filter graph file: expected '%s' on line "\
                                                               "#%d but found '%s' instead.",\
                                                               name, (row+1), rowData.at(row).at(0).toStdString().c_str()));\
                                                        goto fail;\
                                                     }

        QList<QStringList> rowData = csv_parse_c(QString::fromStdString(sourceFilename)).contents();

        if (!rowData.length())
        {
            goto fail;
        }

        // Load the data.
        {
            unsigned row = 0;

            verify_first_element_on_row_is("fileType");
            //const QString fileType = rowData.at(row).at(1);

            row++;
            verify_first_element_on_row_is("fileVersion");
            const QString fileVersion = rowData.at(row).at(1);
            if (QList<QString>{"a", "b"}.indexOf(fileVersion) < 0)
            {
                kd_show_headless_error_message("", "Unsupported graph file format.");
                return false;
            }

            row++;
            verify_first_element_on_row_is("filterCount");
            const unsigned numFilters = rowData.at(row).at(1).toUInt();

            // Load the filter data.
            for (unsigned i = 0; i < numFilters; i++)
            {
                row++;
                verify_first_element_on_row_is("id");
                const filter_type_enum_e filterType = kf_filter_type_for_id(rowData.at(row).at(1).toStdString());

                row++;
                verify_first_element_on_row_is("parameterData");
                const unsigned numParameters = rowData.at(row).at(1).toUInt();

                std::vector<u8> params;
                params.reserve(numParameters);
                for (unsigned p = 0; p < numParameters; p++)
                {
                    params.push_back(rowData.at(row).at(2+p).toUInt());
                }

                const auto newNode = kd_add_filter_graph_node(filterType, (const u8*)params.data());
                graphNodes.push_back(newNode);
            }

            // Load the node data.
            {
                row++;
                verify_first_element_on_row_is("nodeCount");
                const unsigned nodeCount = rowData.at(row).at(1).toUInt();

                for (unsigned i = 0; i < nodeCount; i++)
                {
                    if (fileVersion == "a")
                    {
                        row++;
                        verify_first_element_on_row_is("scenePosition");
                        graphNodes.at(i)->setPos(QPointF(rowData.at(row).at(1).toDouble(), rowData.at(row).at(2).toDouble()));

                        row++;
                        verify_first_element_on_row_is("connections");
                        const unsigned numConnections = rowData.at(row).at(1).toUInt();

                        for (unsigned p = 0; p < numConnections; p++)
                        {
                            node_edge_s *const sourceEdge = graphNodes.at(i)->output_edge();
                            node_edge_s *const targetEdge = graphNodes.at(rowData.at(row).at(2+p).toUInt())->input_edge();

                            k_assert((sourceEdge && targetEdge), "Invalid source or target edge for connecting.");

                            sourceEdge->connect_to(targetEdge);
                        }
                    }
                    /// TODO: Reduce code repetition.
                    else if (fileVersion == "b")
                    {
                        row++;
                        verify_first_element_on_row_is("nodeParameterCount");
                        const unsigned nodeParamCount = rowData.at(row).at(1).toInt();

                        for (unsigned p = 0; p < nodeParamCount; p++)
                        {
                            row++;
                            const auto paramName = rowData.at(row).at(0);

                            if (paramName == "scenePosition")
                            {
                                graphNodes.at(i)->setPos(QPointF(rowData.at(row).at(1).toDouble(), rowData.at(row).at(2).toDouble()));
                            }
                            else if (paramName == "connections")
                            {
                                const unsigned numConnections = rowData.at(row).at(1).toUInt();

                                for (unsigned c = 0; c < numConnections; c++)
                                {
                                    node_edge_s *const sourceEdge = graphNodes.at(i)->output_edge();
                                    node_edge_s *const targetEdge = graphNodes.at(rowData.at(row).at(2+c).toUInt())->input_edge();

                                    k_assert((sourceEdge && targetEdge), "Invalid source or target edge for connecting.");

                                    sourceEdge->connect_to(targetEdge);
                                }
                            }
                            else if (paramName == "backgroundColor")
                            {
                                graphNodes.at(i)->set_background_color(rowData.at(row).at(1));
                            }
                            else
                            {
                                NBENE(("Encountered an unknown filter graph node parameter name. Ignoring it."));
                            }
                        }
                    }
                }
            }

            // Load the graph options.
            {
                row++;
                verify_first_element_on_row_is("graphOptionsCount");
                const unsigned graphOptionsCount = rowData.at(row).at(1).toUInt();

                for (unsigned i = 0; i < graphOptionsCount; i++)
                {
                    row++;
                    graphOptions.push_back(filter_graph_option_s(rowData.at(row).at(0).toStdString(), rowData.at(row).at(1).toInt()));
                }
            }
        }

        #undef verify_first_element_on_row_is
    }

    kpropagate_loaded_filter_graph_from_disk(graphNodes, graphOptions, sourceFilename);

    return true;

    fail:
    kd_clear_filter_graph();
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading the filter graph. No data was "
                                   "loaded.\n\nMore information about the error may be found in "
                                   "the terminal.");
    return false;
}
