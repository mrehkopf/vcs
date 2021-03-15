/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS alias resolutions dialog
 *
 */

#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QMenuBar>
#include <QSpinBox>
#include <QDebug>
#include <QLabel>
#include "display/qt/subclasses/QMenu_dialog_file_menu.h"
#include "common/command_line/command_line.h"
#include "display/qt/dialogs/alias_dialog.h"
#include "display/qt/persistent_settings.h"
#include "display/display.h"
#include "capture/capture.h"
#include "capture/alias.h"
#include "common/disk/disk.h"
#include "ui_alias_dialog.h"

// Custom Qt::ItemDataRole values.
enum data_role
{
    // An alias's source/target resolution.
    width = 0x101,
    height = 0x102,
};

AliasDialog::AliasDialog(QWidget *parent) :
    VCSBaseDialog(parent),
    ui(new Ui::AliasDialog)
{
    ui->setupUi(this);

    this->set_name("Alias resolutions");

    // Create the dialog's menu bar.
    {
        this->menuBar = new QMenuBar(this);

        // File...
        {
            auto *const file = new DialogFileMenu(this);

            this->menuBar->addMenu(file);

            connect(file, &DialogFileMenu::save, this, [=](const QString &filename)
            {
                this->save_aliases_to_file(filename);
            });

            connect(file, &DialogFileMenu::open, this, [=]
            {
                QString filename = QFileDialog::getOpenFileName(this,
                                                                "Select the file to load aliases from",
                                                                "",
                                                                "Alias files (*.vcs-alias);;"
                                                                "All files(*.*)");

                this->load_aliases_from_file(filename);
            });

            connect(file, &DialogFileMenu::save_as, this, [=](const QString &originalFilename)
            {
                QString filename = QFileDialog::getSaveFileName(this,
                                                                "Select the file to save the aliases into",
                                                                originalFilename,
                                                                "Alias files (*.vcs-alias);;"
                                                                "All files(*.*)");

                this->save_aliases_to_file(filename);
            });

            connect(file, &DialogFileMenu::close, this, [=]
            {
                this->remove_all_aliases();
                this->set_data_filename("");
            });
        }

        this->layout()->setMenuBar(this->menuBar);
    }

    // Initialize GUI controls to their starting values.
    {
        // This button is enabled when there are items selected, disabled otherwise.
        ui->pushButton_deleteSelectedAliases->setEnabled(false);
    }

    // Connect GUI controls to consequences for changing their values.
    {
        connect(ui->pushButton_addAlias, &QPushButton::clicked, this, [=]
        {
            this->add_new_alias();
            emit this->data_changed();
        });

        connect(ui->pushButton_deleteSelectedAliases, &QPushButton::clicked, this, [=]
        {
            if ((ui->treeWidget_knownAliases->selectedItems().count() > 0) &&
                (QMessageBox::question(this, "Confirm alias deletion",
                                       "Delete the selected alias(es)?",
                                       (QMessageBox::No | QMessageBox::Yes)) == QMessageBox::Yes))
            {
                this->remove_selected_aliases();
            }
        });

        connect(ui->treeWidget_knownAliases, &QTreeWidget::itemSelectionChanged, this, [=]
        {
            ui->pushButton_deleteSelectedAliases->setEnabled(ui->treeWidget_knownAliases->selectedItems().count() > 0);
        });

        connect(this, &VCSBaseDialog::data_filename_changed, this, [=](const QString &newFilename)
        {
            this->set_unsaved_changes(false);
            kpers_set_value(INI_GROUP_ALIAS_RESOLUTIONS, "aliases_source_file", newFilename);
        });
    }

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_GEOMETRY, "aliases", this->size()).toSize());

        if (kcom_aliases_file_name().empty())
        {
            const QString aliasesSourceFilename = kpers_value_of(INI_GROUP_ALIAS_RESOLUTIONS, "aliases_source_file", QString()).toString();
            kcom_override_aliases_file_name(aliasesSourceFilename.toStdString());
        }
    }

    return;
}

AliasDialog::~AliasDialog(void)
{
    // Save persistent settings.
    {
        kpers_set_value(INI_GROUP_GEOMETRY, "aliases", this->size());
    }

    delete ui;
    ui = nullptr;

    return;
}

// Send the user-defined alias resolutions from the GUI to the capturer,
// which puts them into use.
void AliasDialog::broadcast_aliases(void)
{
    std::vector<mode_alias_s> aliases;

    auto items = ui->treeWidget_knownAliases->findItems("*", Qt::MatchWildcard);
    for (auto *item: items)
    {
        mode_alias_s a;

        a.from.w = item->data(0, data_role::width).toUInt();
        a.from.h = item->data(0, data_role::height).toUInt();
        a.to.w = item->data(1, data_role::width).toUInt();
        a.to.h = item->data(1, data_role::height).toUInt();

        aliases.push_back(a);
    }

    ka_set_aliases(aliases);

    return;
}

void AliasDialog::adjust_treewidget_column_size(void)
{
    ui->treeWidget_knownAliases->setColumnWidth(0, (ui->treeWidget_knownAliases->width() / 2));

    return;
}

void AliasDialog::resizeEvent(QResizeEvent *)
{
    this->adjust_treewidget_column_size();

    return;
}

void AliasDialog::showEvent(QShowEvent *event)
{
    this->adjust_treewidget_column_size();

    QDialog::showEvent(event);

    return;
}

void AliasDialog::add_alias_to_list(const mode_alias_s a)
{
    // Returns a widget containing two QSpinBoxes, corresponding to a resolution's
    // width and height. In other words, with the widget, the user can specify a resolution.
    // This widget is meant to be embedded into a column on a QTreeWidgetItem; the
    // spinboxes' values are stored in the tree item with custom data roles.
    auto create_resolution_widget = [this](QTreeWidgetItem *parentItem,
                                           const uint column,
                                           const uint width,
                                           const uint height)->QWidget*
    {
        QWidget *container = new QWidget;
        QHBoxLayout *layout = new QHBoxLayout;
        container->setLayout(layout);

        QSpinBox *x = new QSpinBox;
        QSpinBox *y = new QSpinBox;

        QLabel *separator = new QLabel("&times;");

        separator->setTextFormat(Qt::RichText);
        separator->setStyleSheet("margin-bottom: .25em;");
        separator->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        separator->setAlignment((Qt::AlignVCenter | Qt::AlignHCenter));

        x->setMinimum(1);
        y->setMinimum(1);

        x->setMaximum(MAX_OUTPUT_WIDTH);
        y->setMaximum(MAX_OUTPUT_HEIGHT);

        x->setValue(width);
        y->setValue(height);

        parentItem->setData(column, data_role::width, x->value());
        parentItem->setData(column, data_role::height, y->value());

        connect(x, &QSpinBox::editingFinished, [this, parentItem, x, column]
        {
            parentItem->setData(column, data_role::width, x->value());
            emit this->data_changed();
            this->broadcast_aliases();
        });

        connect(y, &QSpinBox::editingFinished, [this, parentItem, y, column]
        {
            parentItem->setData(column, data_role::height, y->value());
            emit this->data_changed();
            this->broadcast_aliases();
        });

        connect(x, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]
        {
            emit this->data_changed();
        });

        connect(y, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]
        {
            emit this->data_changed();
        });

        layout->addWidget(x);
        layout->addWidget(separator);
        layout->addWidget(y);

        return container;
    };

    QTreeWidgetItem *entry = new QTreeWidgetItem;
    ui->treeWidget_knownAliases->addTopLevelItem(entry);
    ui->treeWidget_knownAliases->setItemWidget(entry, 0, create_resolution_widget(entry, 0, a.from.w, a.from.h));
    ui->treeWidget_knownAliases->setItemWidget(entry, 1, create_resolution_widget(entry, 1, a.to.w, a.to.h));

    this->adjust_treewidget_column_size();

    return;
}

void AliasDialog::clear_known_aliases(void)
{
    ui->treeWidget_knownAliases->clear();

    return;
}

void AliasDialog::add_new_alias(void)
{
    mode_alias_s newAlias;
    newAlias.from = {1, 1, 0};
    newAlias.to = {640, 480, 0};

    this->add_alias_to_list(newAlias);

    this->broadcast_aliases();

    return;
}

void AliasDialog::remove_all_aliases(void)
{
    const auto allItems = ui->treeWidget_knownAliases->findItems("*", Qt::MatchWildcard);

    for (auto item: allItems)
    {
        emit this->data_changed();
        delete item;
    }

    this->broadcast_aliases();

    return;
}

void AliasDialog::remove_selected_aliases(void)
{
    const auto selectedItems = ui->treeWidget_knownAliases->selectedItems();

    for (auto item: selectedItems)
    {
        emit this->data_changed();
        delete item;
    }

    this->broadcast_aliases();

    return;
}

void AliasDialog::load_aliases_from_file(const QString &filename)
{
    if (filename.isEmpty())
    {
        return;
    }

    const auto aliases = kdisk_load_aliases(filename.toStdString());

    if (!aliases.empty())
    {
        ka_set_aliases(aliases);
        this->set_data_filename(filename);
        this->assign_aliases(aliases);

        /// TODO: remove_unsaved_changes_flag();
    }

    return;
}

void AliasDialog::save_aliases_to_file(QString filename)
{
    if (filename.isEmpty())
    {
        return;
    }

    if (QFileInfo(filename).suffix().isEmpty())
    {
        filename += ".vcs-alias";
    }

    this->broadcast_aliases();

    if (kdisk_save_aliases(ka_aliases(), filename.toStdString()))
    {
        this->set_data_filename(filename);
    }

    return;
}

void AliasDialog::assign_aliases(const std::vector<mode_alias_s> &aliases)
{
    ui->treeWidget_knownAliases->clear();

    for (const auto a: aliases)
    {
        this->add_alias_to_list(a);
    }

    return;
}
