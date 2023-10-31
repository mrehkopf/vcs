/*
 * 2018-2023 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_WINDOWS_CONTROL_PANEL_CAPTURE_ALIAS_RESOLUTIONS_H
#define VCS_DISPLAY_QT_WINDOWS_CONTROL_PANEL_CAPTURE_ALIAS_RESOLUTIONS_H

#include <QList>
#include "display/qt/widgets/DialogFragment.h"

class QMenuBar;
class QTreeWidgetItem;
struct resolution_alias_s;

namespace control_panel::capture
{
    namespace Ui { class AliasResolutions; }

    class AliasResolutions : public DialogFragment
    {
        Q_OBJECT

    public:
        explicit AliasResolutions(QWidget *parent = 0);
        ~AliasResolutions(void);
        void clear_known_aliases(void);
        void add_alias_to_list(const resolution_alias_s &a);

    private:
        Ui::AliasResolutions *ui;

        void remove_aliases(const QList<QTreeWidgetItem*> &aliasTreeItems);
        void remove_selected_aliases(void);
        void remove_all_aliases(void);
        void add_new_alias(void);
        void broadcast_aliases();
        void adjust_treewidget_column_size();
        void resizeEvent(QResizeEvent *);
        void showEvent(QShowEvent *event);
        void assign_aliases(const std::vector<resolution_alias_s> &aliases);
        QStringList aliases_as_stringlist(void);
    };
}

#endif
