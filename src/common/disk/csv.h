/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * A simple parser for a CSV-like format, where you have rows of char-delimited
 * strings. Used in VCS for storing user settings on disk.
 *
 */

#ifndef VCS_COMMON_DISK_CSV_H
#define VCS_COMMON_DISK_CSV_H

#include <QFile>
#include "common/globals.h"
#include "common/assert.h"

class csv_parse_c
{
public:
    csv_parse_c(const QString &filename)
    {
        file.setFileName(filename);
        file.open(QIODevice::ReadOnly | QIODevice::Text);   // We'll test later whether the file was opened successfully.

        return;
    }

    // Returns the contents of the CSV file as a list of row elements, where each
    // entry in the list corresponds to one row in the file and consists of the
    // individual elements on that row. Note that this requires the element delimiter
    // to be ','. To prevent the occurrence of the delimeter from breaking up a
    // set of elements, encase them in {}.
    //
    QList<QStringList> contents(void)
    {
        if (!file.isOpen() ||
            !(file.pos() == 0))
        {
            NBENE(("Invalid CSV file."));
            return {};
        }

        QString allData = file.readAll();
        if (file.error() != QFileDevice::NoError)
        {
            NBENE(("Invalid CSV file."));
            return {};
        }

        QStringList rows = allData.split('\n');

        QList<QStringList> splitRows;
        for (auto &row: rows)
        {
            QStringList elements = row.split(',');

            // Remove empty elements.
            elements.removeAll(QString(""));

            // Find and combine elements inside nests, i.e. blocks of text encased
            // in {}.
            /// TODO: Ideally, you'd account for nests when splitting, rather
            /// than afterwards like this.
            /// NOTE: The nesting characters {} must occur at the edges of the
            /// range of elements they join.
            for (int i = 0; i < elements.size(); i++)
            {
                QString elemStr = elements.at(i);

                if (elemStr.at(0) == '{')
                {
                    while (elemStr.at(elemStr.length() - 1) != '}')
                    {
                        // Put back the delimiter character that would've been removed while splitting.
                        elemStr += ',';

                        elemStr += elements.at(i + 1);
                        elements.removeAt(i);
                    }

                    k_assert((elemStr.at(0) == '{' && elemStr.at(elemStr.length() - 1) == '}'), "Failed to parse a CSV nest.");

                    // Remove the nesting characters {}.
                    elemStr.remove(0, 1);
                    elemStr.remove((elemStr.length() - 1), 1);
                }

                elements[i] = elemStr;
            }

            if (!elements.isEmpty()) splitRows.push_back(elements);
        }

        return splitRows;
    }

private:
    QFile file;
};

#endif
