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

bool file_reader::filter_graph::version_a::read(const std::string &filename,
                                                std::vector<abstract_filter_graph_node_s> *const graphNodes)
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
        k_assert((fileVersion == "a"), "Mismatched file version for reading.");

        row++;
        FAIL_IF_FIRST_CELL_IS_NOT("filterCount");
        const unsigned numFilters = rowData.at(row).at(1).toUInt();

        // Load the filter data.
        for (unsigned i = 0; i < numFilters; i++)
        {
            abstract_filter_graph_node_s node;

            row++;
            FAIL_IF_FIRST_CELL_IS_NOT("id");
            node.typeUuid = rowData.at(row).at(1).toStdString();

            row++;
            FAIL_IF_FIRST_CELL_IS_NOT("parameterData");
            const unsigned numParameters = rowData.at(row).at(1).toUInt();

            for (unsigned paramIdx = 0; paramIdx < numParameters; paramIdx++)
            {
                const double paramValue = rowData.at(row).at(2 + paramIdx).toDouble();
                node.initialParameters.push_back({paramIdx, paramValue});
            }

            graphNodes->push_back(node);
        }

        // Load the node data.
        {
            row++;
            FAIL_IF_FIRST_CELL_IS_NOT("nodeCount");
            const unsigned nodeCount = rowData.at(row).at(1).toUInt();

            for (unsigned i = 0; i < nodeCount; i++)
            {
                row++;
                FAIL_IF_FIRST_CELL_IS_NOT("scenePosition");
                graphNodes->at(i).initialPosition = {rowData.at(row).at(1).toDouble(),
                                              rowData.at(row).at(2).toDouble()};

                row++;
                FAIL_IF_FIRST_CELL_IS_NOT("connections");
                const unsigned numConnections = rowData.at(row).at(1).toUInt();

                for (unsigned c = 0; c < numConnections; c++)
                {
                    const int dstNodeId = rowData.at(row).at(2+c).toInt();
                    graphNodes->at(i).connectedTo.push_back(dstNodeId);
                }
            }
        }
    }

    #undef FAIL_IF_FIRST_CELL_IS_NOT
    
    return true;

    fail:
    return false;
}
