/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QList> /// TODO: Break dependency on Qt.
#include "common/disk/file_readers/file_reader_filter_graph.h"
#include "common/disk/csv.h"
#include "filter/filter.h"

bool file_reader::filter_graph::version_b::read(const std::string &filename,
                                                std::vector<FilterGraphNode*> *const graphNodes,
                                                std::vector<filter_graph_option_s> *const graphOptions)
{
    // Bails out if the value (string) of the first cell on the current row doesn't match
    // the given one.
    #define FAIL_IF_FIRST_CELL_IS_NOT(string) if ((int)row >= rowData.length())\
                                              {\
                                                  NBENE(("Error while loading the filter graph file: expected '%s' on line "\
                                                         "#%d but found the data out of range.", string, (row+1)));\
                                                  goto fail;\
                                              }\
                                              else if (rowData.at(row).at(0) != string)\
                                              {\
                                                  NBENE(("Error while loading filter graph file: expected '%s' on line "\
                                                         "#%d but found '%s' instead.",\
                                                          string, (row+1), rowData.at(row).at(0).toStdString().c_str()));\
                                                  goto fail;\
                                              }

    QList<QStringList> rowData = csv_parse_c(QString::fromStdString(filename)).contents();

    if (rowData.isEmpty())
    {
        NBENE(("Empty filter graph file."));
        goto fail;
    }

    // Load the data.
    {
        unsigned row = 0;

        FAIL_IF_FIRST_CELL_IS_NOT("fileType");
        //const QString fileType = rowData.at(row).at(1);

        row++;
        FAIL_IF_FIRST_CELL_IS_NOT("fileVersion");
        const QString fileVersion = rowData.at(row).at(1);
        k_assert((fileVersion == "b"), "Mismatched file version for reading.");

        row++;
        FAIL_IF_FIRST_CELL_IS_NOT("filterCount");
        const unsigned numFilters = rowData.at(row).at(1).toUInt();

        // Load the filter data.
        for (unsigned i = 0; i < numFilters; i++)
        {
            row++;
            FAIL_IF_FIRST_CELL_IS_NOT("id");
            const filter_type_enum_e filterType = kf_filter_type_for_id(rowData.at(row).at(1).toStdString());

            row++;
            FAIL_IF_FIRST_CELL_IS_NOT("parameterData");
            const unsigned numParameters = rowData.at(row).at(1).toUInt();

            std::vector<u8> params;
            params.reserve(numParameters);
            for (unsigned p = 0; p < numParameters; p++)
            {
                params.push_back(rowData.at(row).at(2+p).toUInt());
            }

            const auto newNode = kd_add_filter_graph_node(filterType, (const u8*)params.data());
            graphNodes->push_back(newNode);
        }

        // Load the node data.
        {
            row++;
            FAIL_IF_FIRST_CELL_IS_NOT("nodeCount");
            const unsigned nodeCount = rowData.at(row).at(1).toUInt();

            for (unsigned i = 0; i < nodeCount; i++)
            {
                row++;
                FAIL_IF_FIRST_CELL_IS_NOT("nodeParameterCount");
                const unsigned nodeParamCount = rowData.at(row).at(1).toInt();

                for (unsigned p = 0; p < nodeParamCount; p++)
                {
                    row++;
                    const auto paramName = rowData.at(row).at(0);

                    if (paramName == "scenePosition")
                    {
                        graphNodes->at(i)->setPos(QPointF(rowData.at(row).at(1).toDouble(), rowData.at(row).at(2).toDouble()));
                    }
                    else if (paramName == "isEnabled")
                    {
                        graphNodes->at(i)->set_enabled(rowData.at(row).at(1).toInt());
                    }
                    else if (paramName == "connections")
                    {
                        const unsigned numConnections = rowData.at(row).at(1).toUInt();

                        for (unsigned c = 0; c < numConnections; c++)
                        {
                            node_edge_s *const sourceEdge = graphNodes->at(i)->output_edge();
                            node_edge_s *const targetEdge = graphNodes->at(rowData.at(row).at(2+c).toUInt())->input_edge();

                            k_assert((sourceEdge && targetEdge), "Invalid source or target edge for connecting.");

                            sourceEdge->connect_to(targetEdge);
                        }
                    }
                    else if (paramName == "backgroundColor")
                    {
                        graphNodes->at(i)->set_background_color(rowData.at(row).at(1));
                    }
                    else
                    {
                        NBENE(("Encountered an unknown filter graph node parameter name. Ignoring it."));
                    }
                }
            }
        }

        // Load the graph options.
        {
            row++;
            FAIL_IF_FIRST_CELL_IS_NOT("graphOptionsCount");
            const unsigned graphOptionsCount = rowData.at(row).at(1).toUInt();

            for (unsigned i = 0; i < graphOptionsCount; i++)
            {
                row++;
                graphOptions->push_back(filter_graph_option_s(rowData.at(row).at(0).toStdString(), rowData.at(row).at(1).toInt()));
            }
        }
    }

    #undef FAIL_IF_FIRST_CELL_IS_NOT
    
    return true;

    fail:
    return false;
}
