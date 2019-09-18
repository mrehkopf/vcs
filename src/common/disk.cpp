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
#include "common/file_writer.h"
#include "common/propagate.h"
#include "capture/capture.h"
#include "capture/alias.h"
#include "filter/filter.h"
#include "filter/filter_legacy.h"
#include "common/disk.h"
#include "common/disk_legacy.h"
#include "common/csv.h"

bool kdisk_save_video_mode_params(const std::vector<video_mode_params_s> &modeParams,
                                  const QString &targetFilename)
{
    file_writer_c outFile(targetFilename);

    // Each mode params block consists of two values specifying the resolution
    // followed by a set of string-value pairs for the different parameters.
    for (const auto &m: modeParams)
    {
        // Resolution.
        outFile << "resolution," << m.r.w << "," << m.r.h << "\n";

        // Video params.
        outFile << "vPos," << m.video.verticalPosition << "\n"
                << "hPos," << m.video.horizontalPosition << "\n"
                << "hScale," << m.video.horizontalScale << "\n"
                << "phase," << m.video.phase << "\n"
                << "bLevel," << m.video.blackLevel << "\n";

        // Color params.
        outFile << "bright," << m.color.overallBrightness << "\n"
                << "contr," << m.color.overallContrast << "\n"
                << "redBr," << m.color.redBrightness << "\n"
                << "redCn," << m.color.redContrast << "\n"
                << "greenBr," << m.color.greenBrightness << "\n"
                << "greenCn," << m.color.greenContrast << "\n"
                << "blueBr," << m.color.blueBrightness << "\n"
                << "blueCn," << m.color.blueContrast << "\n";

        // Separate the next block.
        outFile << "\n";
    }

    if (!outFile.is_valid() ||
        !outFile.save_and_close())
    {
        NBENE(("Failed to write mode params to file."));
        goto fail;
    }

    kpropagate_saved_mode_params_to_disk(modeParams, targetFilename.toStdString());

    return true;

    fail:
    kd_show_headless_error_message("Data was not saved",
                                   "An error was encountered while preparing the mode "
                                   "settings for saving. As a result, no data was saved. \n\nMore "
                                   "information about this error may be found in the terminal.");
    return false;
}

bool kdisk_load_video_mode_params(const std::string &sourceFilename)
{
    if (sourceFilename.empty())
    {
        DEBUG(("No mode settings file defined, skipping."));
        return true;
    }

    std::vector<video_mode_params_s> videoModeParams;

    QList<QStringList> paramRows = csv_parse_c(QString::fromStdString(sourceFilename)).contents();

    // Each mode is saved as a block of rows, starting with a 3-element row defining
    // the mode's resolution, followed by several 2-element rows defining the various
    // video and color parameters for the resolution.
    for (int i = 0; i < paramRows.count();)
    {
        if ((paramRows[i].count() != 3) ||
            (paramRows[i].at(0) != "resolution"))
        {
            NBENE(("Expected a 3-parameter 'resolution' statement to begin a mode params block."));
            goto fail;
        }

        video_mode_params_s p;
        p.r.w = paramRows[i].at(1).toUInt();
        p.r.h = paramRows[i].at(2).toUInt();

        i++;    // Move to the next row to start fetching the params for this resolution.

        auto get_param = [&](const QString &name)->QString
                         {
                             if (paramRows[i].at(0) != name)
                             {
                                 NBENE(("Error while loading video parameters: expected '%s' but got '%s'.",
                                        name.toLatin1().constData(), paramRows[i].at(0).toLatin1().constData()));
                                 throw 0;
                             }
                             return paramRows[i++].at(1);
                         };
        try
        {
            // Note: the order in which the params are fetched is fixed to the
            // order in which they were saved.
            p.video.verticalPosition = get_param("vPos").toInt();
            p.video.horizontalPosition = get_param("hPos").toInt();
            p.video.horizontalScale = get_param("hScale").toInt();
            p.video.phase = get_param("phase").toInt();
            p.video.blackLevel = get_param("bLevel").toInt();
            p.color.overallBrightness = get_param("bright").toInt();
            p.color.overallContrast = get_param("contr").toInt();
            p.color.redBrightness = get_param("redBr").toInt();
            p.color.redContrast = get_param("redCn").toInt();
            p.color.greenBrightness = get_param("greenBr").toInt();
            p.color.greenContrast = get_param("greenCn").toInt();
            p.color.blueBrightness = get_param("blueBr").toInt();
            p.color.blueContrast = get_param("blueCn").toInt();
        }
        catch (int)
        {
            NBENE(("Failed to load mode params from disk."));
            goto fail;
        }

        videoModeParams.push_back(p);
    }

    // Sort the modes so they'll display more nicely in the GUI.
    std::sort(videoModeParams.begin(), videoModeParams.end(), [](const video_mode_params_s &a, const video_mode_params_s &b)
                                          { return (a.r.w * a.r.h) < (b.r.w * b.r.h); });

    kpropagate_loaded_mode_params_from_disk(videoModeParams, sourceFilename);

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

    // Signal a new input mode to force the program to re-evaluate the mode
    // parameters, in case one of the newly-loaded aliases applies to the
    // current mode.
    kpropagate_news_of_new_capture_video_mode();

    return true;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading aliases. No data was "
                                   "loaded.\n\nMore information about the error may be found in "
                                   "the terminal.");
    return false;
}

bool kdisk_save_aliases(const std::vector<mode_alias_s> &aliases,
                        const QString &targetFilename)
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

    kpropagate_saved_aliases_to_disk(aliases, targetFilename.toStdString());

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
                             const QString &targetFilename)
{
    file_writer_c outFile(targetFilename);

    outFile << "fileType,{VCS filter graph}\n"
            << "fileVersion,a\n";

    // Save filter information.
    {
        outFile << "filterCount," << nodes.size() << "\n";

        for (FilterGraphNode *const node: nodes)
        {
            outFile << "id,{" << QString::fromStdString(kf_filter_id_for_type(node->associatedFilter->metaData.type)) << "}\n";

            outFile << "parameterData," << FILTER_PARAMETER_ARRAY_LENGTH;
            for (unsigned i = 0; i < FILTER_PARAMETER_ARRAY_LENGTH; i++)
            {
                outFile << QString(",%1").arg(node->associatedFilter->parameterData[i]);
            }
            outFile << "\n";
        }
    }

    // Save node information.
    {
        outFile << "nodeCount," << nodes.size() << "\n";

        for (FilterGraphNode *const node: nodes)
        {
            outFile << "scenePosition," << QString("%1,%2").arg(node->pos().x()).arg(node->pos().y()) << "\n";

            outFile << "connections,";
            if (node->output_edge())
            {
                outFile << node->output_edge()->connectedTo.size();
                for (const node_edge_s *const connection: node->output_edge()->connectedTo)
                {
                    const auto nodeIt = std::find(nodes.begin(), nodes.end(), connection->parentNode);
                    k_assert((nodeIt != nodes.end()), "Cannot find the target node of a connection.");
                    outFile << QString(",%1").arg(std::distance(nodes.begin(), nodeIt));
                }
                outFile << "\n";
            }
            else
            {
                outFile << "0\n";
            }
        }
    }

    // Save graph optins.
    {
        outFile << "graphOptions," << graphOptions.size() << "\n";

        for (const filter_graph_option_s &option: graphOptions)
        {
            outFile << QString::fromStdString(option.propertyName) << "," << (int)option.value << "\n";
        }
    }

    if (!outFile.is_valid() ||
        !outFile.save_and_close())
    {
        NBENE(("Failed to write aliases to file."));
        goto fail;
    }

    kpropagate_saved_filter_graph_to_disk(targetFilename.toStdString());

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
        INFO(("No filter graph file defined, skipping."));
        return true;
    }

    std::vector<FilterGraphNode*> nodes;
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
            nodes.push_back(newNode);

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
                for (unsigned i = 0; i < (nodes.size() - 1); i++)
                {
                    auto sourceEdge = nodes.at(i)->output_edge();
                    auto targetEdge = nodes.at(i+1)->input_edge();

                    k_assert((sourceEdge && targetEdge), "A filter node is missing a required edge.");

                    sourceEdge->connect_to(targetEdge);
                }
            }

            nodes.clear();

            nodeYOffset += (tallestNode + 50);
            nodeXOffset = 0;
        }

        delete [] filterParamsTmp;
    }
    // Load from a normal filters file.
    else
    {
        #define verify_first_element_on_row_is(name) if (rowData.at(row).at(0) != name)\
                                                     {\
                                                        NBENE(("Error while loading filter graph file: expected '%s' but got '%s'.",\
                                                               name, rowData.at(row).at(0).toStdString().c_str()));\
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
            //const QString fileType = rowData[row].at(1);

            row++;
            verify_first_element_on_row_is("fileVersion");
            //const QString fileVersion = rowData[row].at(1);

            row++;
            verify_first_element_on_row_is("filterCount");
            const unsigned numFilters = rowData[row].at(1).toUInt();

            // Load the filter data.
            for (unsigned i = 0; i < numFilters; i++)
            {
                row++;
                verify_first_element_on_row_is("id");
                const filter_type_enum_e filterType = kf_filter_type_for_id(rowData[row].at(1).toStdString());

                row++;
                verify_first_element_on_row_is("parameterData");
                const unsigned numParameters = rowData[row].at(1).toUInt();

                std::vector<u8> params;
                params.reserve(numParameters);
                for (unsigned p = 0; p < numParameters; p++)
                {
                    params.push_back(rowData[row].at(2+p).toUInt());
                }

                const auto newNode = kd_add_filter_graph_node(filterType, (const u8*)params.data());
                nodes.push_back(newNode);
            }

            // Load the node data.
            {
                row++;
                verify_first_element_on_row_is("nodeCount");
                const unsigned nodeCount = rowData[row].at(1).toUInt();

                for (unsigned i = 0; i < nodeCount; i++)
                {
                    row++;
                    verify_first_element_on_row_is("scenePosition");
                    nodes.at(i)->setPos(QPointF(rowData[row].at(1).toDouble(), rowData[row].at(2).toDouble()));

                    row++;
                    verify_first_element_on_row_is("connections");
                    const unsigned numConnections = rowData[row].at(1).toUInt();

                    for (unsigned p = 0; p < numConnections; p++)
                    {
                        node_edge_s *const sourceEdge = nodes.at(i)->output_edge();
                        node_edge_s *const targetEdge = nodes.at(rowData[row].at(2+p).toUInt())->input_edge();

                        k_assert((sourceEdge && targetEdge), "Invalid source or target edge for connecting.");

                        sourceEdge->connect_to(targetEdge);
                    }
                }
            }

            // Load the graph options.
            {
                row++;
                verify_first_element_on_row_is("graphOptionsCount");
                const unsigned graphOptionsCount = rowData[row].at(1).toUInt();

                for (unsigned i = 0; i < graphOptionsCount; i++)
                {
                    row++;
                    graphOptions.push_back(filter_graph_option_s(rowData.at(row).at(0).toStdString(), rowData.at(row).at(1).toInt()));
                }
            }
        }

        #undef verify_first_element_on_row_is
    }

    kpropagate_loaded_filter_graph_from_disk(nodes, graphOptions, sourceFilename);

    return true;

    fail:
    kd_show_headless_error_message("Data was not loaded",
                                   "An error was encountered while loading the filter graph. No data was "
                                   "loaded.\n\nMore information about the error may be found in "
                                   "the terminal.");
    return false;
}
