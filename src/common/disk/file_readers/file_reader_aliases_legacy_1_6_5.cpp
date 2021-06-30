/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QList> /// TODO: Break dependency on Qt.
#include "common/disk/file_readers/file_reader_aliases.h"
#include "common/disk/csv.h"

bool file_reader::aliases::legacy_1_6_5::read(const std::string &filename,
                                              std::vector<resolution_alias_s> *const aliases)
{
    QList<QStringList> rowData = csv_parse_c(QString::fromStdString(filename)).contents();

    if (rowData.isEmpty())
    {
        NBENE(("Empty filter aliases file."));
        goto fail;
    }

    for (const auto &row: rowData)
    {
        if (row.count() != 4)
        {
            NBENE(("Expected a 4-parameter row in the alias file."));
            goto fail;
        }

        resolution_alias_s a;
        a.from.w = row.at(0).toUInt();
        a.from.h = row.at(1).toUInt();
        a.to.w = row.at(2).toUInt();
        a.to.h = row.at(3).toUInt();

        aliases->push_back(a);
    }
    
    return true;

    fail:
    return false;
}
