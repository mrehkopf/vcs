/*
 * 2018-2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_WIDGETS_RESOLUTION_QUERY_H
#define VCS_DISPLAY_QT_WIDGETS_RESOLUTION_QUERY_H

#include <QDialog>

namespace Ui {
    class ResolutionQuery;
}

struct resolution_s;

class ResolutionQuery : public QDialog
{
    Q_OBJECT

public:
    explicit ResolutionQuery(const QString title, resolution_s *const r, QWidget *parent = 0);

    ~ResolutionQuery(void);

private:
    Ui::ResolutionQuery *ui;

    // A pointer to the resolution object on which this dialog operates.
    resolution_s *const resolution;
};

#endif
