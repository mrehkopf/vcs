/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <QString>
#include <QList>
#include "common/disk/file_reader.h"
#include "common/disk/csv.h"

std::string file_reader::file_version(const std::string &filename)
{
    QList<QStringList> rowData = csv_parse_c(QString::fromStdString(filename)).contents();

    for (const auto &row : rowData)
    {
        if (row.at(0) == "fileVersion")
        {
            return row.at(1).toStdString();
        }
    }

    return "";
}

std::string file_reader::file_type(const std::string &filename)
{
    QList<QStringList> rowData = csv_parse_c(QString::fromStdString(filename)).contents();

    for (const auto &row : rowData)
    {
        if (row.at(0) == "fileType")
        {
            return row.at(1).toStdString();
        }
    }

    return "";
}
