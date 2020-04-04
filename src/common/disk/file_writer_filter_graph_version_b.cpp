/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "common/disk/file_writer_filter_graph.h"
#include "common/disk/file_streamer.h"
#include "common/globals.h"
#include "filter/filter.h"

bool file_writer::filter_graph::version_b::write(const std::string &filename,
                                                 const std::vector<FilterGraphNode*> &nodes,
                                                 const std::vector<filter_graph_option_s> &graphOptions)
{   
    file_streamer_c outFile(filename);

    outFile << "fileType,{VCS filter graph}\n"
            << "fileVersion,b\n";

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
            // How many individual parameters (like scenePosition, backgroundColor, etc.)
            // we'll save.
            outFile << "nodeParameterCount," << 3 << "\n";

            outFile << "scenePosition," << QString("%1,%2").arg(node->pos().x()).arg(node->pos().y()) << "\n";

            outFile << "backgroundColor," << node->current_background_color_name() << "\n";

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

    // Save graph options.
    {
        outFile << "graphOptionsCount," << graphOptions.size() << "\n";

        for (const filter_graph_option_s &option: graphOptions)
        {
            outFile << QString::fromStdString(option.propertyName) << "," << (int)option.value << "\n";
        }
    }

    if (!outFile.is_valid() ||
        !outFile.save_and_close())
    {
        NBENE(("Failed to save the filter graph to disk."));
        goto fail;
    }

    return true;

    fail:
    return false;
}
