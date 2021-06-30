/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QList> /// TODO: Break dependency on Qt.
#include "common/disk/file_readers/file_reader_aliases.h"
#include "common/disk/csv.h"

bool file_reader::aliases::version_a::read(const std::string &filename,
                                           std::vector<resolution_alias_s> *const aliases)
{
    // Bails out if the value (string) of the given cell on the current row
    // doesn't match the given one.
    #define FAIL_IF_CELL_IS_NOT(cellIdx, string) if ((int)row >= rowData.length())\
                                                 {\
                                                     NBENE(("Error while loading the alias file: expected '%s' on line "\
                                                             "#%d but found the data out of range.", string, (row+1)));\
                                                     goto fail;\
                                                 }\
                                                 else if (rowData.at(row).at(cellIdx) != string)\
                                                 {\
                                                     NBENE(("Error while loading the alias file: expected '%s' on line "\
                                                             "#%d but found '%s' instead.",\
                                                             string, (row+1), rowData.at(row).at(cellIdx).toStdString().c_str()));\
                                                     goto fail;\
                                                 }

    QList<QStringList> rowData = csv_parse_c(QString::fromStdString(filename)).contents();

    if (rowData.isEmpty())
    {
        NBENE(("Empty filter aliases file."));
        goto fail;
    }

    // Load the data.
    {
        unsigned row = 0;

        FAIL_IF_CELL_IS_NOT(0, "fileType");
        //const QString fileType = rowData.at(row).at(1);

        row++;
        FAIL_IF_CELL_IS_NOT(0, "fileVersion");
        const QString fileVersion = rowData.at(row).at(1);
        k_assert((fileVersion == "a"), "Mismatched file version for reading.");

        row++;
        FAIL_IF_CELL_IS_NOT(0, "aliasCount");
        const unsigned numAliases = rowData.at(row).at(1).toUInt();

        for (unsigned i = 0; i < numAliases; i++)
        {
            resolution_alias_s alias;

            row++;
            FAIL_IF_CELL_IS_NOT(0, "alias");
            FAIL_IF_CELL_IS_NOT(3, "to");

            alias.from.w = rowData.at(row).at(1).toUInt();
            alias.from.h = rowData.at(row).at(2).toUInt();
            alias.to.w = rowData.at(row).at(4).toUInt();
            alias.to.h = rowData.at(row).at(5).toUInt();

            aliases->push_back(alias);
        }
    }

    #undef FAIL_IF_FIRST_CELL_IS_NOT
    
    return true;

    fail:
    return false;
}
