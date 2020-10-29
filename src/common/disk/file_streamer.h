#ifndef VCS_COMMON_DISK_FILE_STREAMER_H
#define VCS_COMMON_DISK_FILE_STREAMER_H

#include <QTextStream>
#include <QString>
#include <QFile>

// A wrapper for Qt's file-saving functionality. Streams given strings via the
// << operator on the object into a file.
class file_streamer_c
{
public:
    file_streamer_c(std::string filename) :
        targetFilename(QString::fromStdString(filename)),
        tempFilename(targetFilename + ".temporary")
    {
        file.setFileName(tempFilename);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        stream.setDevice(&file);

        return;
    }

    // Saves the data from the temp file to the final target file.
    bool save_and_close(void)
    {
        if (QFile(targetFilename).exists() &&
            !QFile(targetFilename).remove())
        {
            return false;
        }

        file.close();

        if (!QFile::rename(tempFilename, targetFilename))
        {
            return false;
        }

        return true;
    }

    bool is_valid(void)
    {
        return isValid;
    }

    QTextStream& operator<<(const QString &string)
    {
        if (this->isValid)
        {
            stream << string;
            test_validity();
        }

        return stream;
    }

    QTextStream& operator<<(const unsigned unsignedInt)
    {
        return stream << QString::number(unsignedInt);
    }

private:
    bool isValid = true;

    QFile file;
    QTextStream stream;

    // The filename we want to save to.
    QString targetFilename;

    // The filename we'll first temporarily write the data to. If all goes well,
    // we'll rename this to the target filename, and remove the old target file,
    // if it exists.
    QString tempFilename;

    void test_validity(void)
    {
        if (!this->file.isWritable() ||
            !this->file.isOpen() ||
            this->file.error() != QFileDevice::NoError)
        {
            isValid = false;
        }

        return;
    }
};

#endif
