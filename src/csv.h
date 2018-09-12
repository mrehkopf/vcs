/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * A simple parser for a CSV-like format, where you have rows of char-delimited
 * strings. Used in VCS for storing user settings on disk.
 *
 */

#include <QFile>
#include "common.h"

#ifndef CSV_LIKE_H
#define CSV_LIKE_H

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
    // to be ',' and that elements joined by quotes will not be combined.
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
            elements.removeAll(QString(""));    // Remove empty strings.

            if (elements.count() != 0)
            {
                splitRows.push_back(elements);
            }
        }

        return splitRows;
    }

private:
    QFile file;
};

#endif
