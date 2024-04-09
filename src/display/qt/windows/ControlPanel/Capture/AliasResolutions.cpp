/*
 * 2018-2023 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
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
#include "common/command_line/command_line.h"
#include "display/qt/persistent_settings.h"
#include "display/display.h"
#include "capture/capture.h"
#include "capture/alias.h"
#include "AliasResolutions.h"
#include "ui_AliasResolutions.h"

// Custom Qt::ItemDataRole values.
enum data_role
{
    // An alias's source/target resolution.
    width = 0x101,
    height = 0x102,
};

control_panel::capture::AliasResolutions::AliasResolutions(QWidget *parent) :
    DialogFragment(parent),
    ui(new Ui::AliasResolutions)
{
    this->ui->setupUi(this);

    this->set_name("Alias resolutions");
    this->set_minimize_when_disabled(true);

    // Initialize GUI controls to their starting values.
    {
        // This button is enabled when there are items selected, disabled otherwise.
        this->ui->pushButton_deleteSelectedAliases->setEnabled(false);
    }

    // Connect GUI controls to consequences for changing their values.
    {
        connect(this->ui->pushButton_addAlias, &QPushButton::clicked, this, [this]
        {
            this->add_new_alias();
            emit this->data_changed();
        });

        connect(this->ui->pushButton_deleteSelectedAliases, &QPushButton::clicked, this, [this]
        {
            if (this->ui->treeWidget_knownAliases->selectedItems().count() > 0)
            {
                this->remove_selected_aliases();
                emit this->data_changed();
            }
        });

        connect(this, &DialogFragment::data_changed, this, [this]
        {
            const QStringList aliases = this->aliases_as_stringlist();
            kpers_set_value(INI_GROUP_CAPTURE, "AliasResolutions", aliases);
        });

        connect(this->ui->treeWidget_knownAliases, &QTreeWidget::itemSelectionChanged, this, [this]
        {
            this->ui->pushButton_deleteSelectedAliases->setEnabled(this->ui->treeWidget_knownAliases->selectedItems().count() > 0);
        });

        connect(this, &DialogFragment::enabled_state_set, this, [this](const bool isEnabled)
        {
            kpers_set_value(INI_GROUP_CAPTURE, "AliasResolutionsEnabled", isEnabled);
            this->ui->groupBox->setChecked(isEnabled);
            ka_set_aliases_enabled(isEnabled);
        });

        connect(this->ui->groupBox, &QGroupBox::toggled, this, [this](const bool isEnabled)
        {
            this->set_enabled(isEnabled);

            if (isEnabled)
            {
                this->broadcast_aliases();
            }
        });
    }

    // Restore persistent aliases.
    {
        // Aliases will be in the form {"1x1@640x480", "2x1@640x480", ...}.
        QStringList aliases = kpers_value_of(INI_GROUP_CAPTURE, "AliasResolutions", QStringList{}).toStringList();

        this->set_enabled(kpers_value_of(INI_GROUP_CAPTURE, "AliasResolutionsEnabled", false).toBool());

        // If the AliasResolutions setting is defined but empty, we'll get a QStringList
        // made up of a single empty string. But in that case what we really want is an
        // empty QStringList.
        aliases.removeAll("");

        for (const auto &string: aliases)
        {
            const QStringList alias = string.split("@");
            const QStringList from = alias.at(0).split("x");
            const QStringList to = alias.at(1).split("x");
            resolution_alias_s a = {
                .from = {.w = from.at(0).toUInt(), .h = from.at(1).toUInt()},
                .to = {.w = to.at(0).toUInt(), .h = to.at(1).toUInt()}
            };
            add_alias_to_list(a);
        }

        this->broadcast_aliases();
    }

    return;
}

control_panel::capture::AliasResolutions::~AliasResolutions(void)
{
    delete ui;
    ui = nullptr;
    return;
}

QStringList control_panel::capture::AliasResolutions::aliases_as_stringlist(void)
{
    QStringList aliases;

    auto items = this->ui->treeWidget_knownAliases->findItems("*", Qt::MatchWildcard);
    for (auto *item: items)
    {
        aliases.push_back(QString("%1x%2@%3x%4")
            .arg(item->data(0, data_role::width).toUInt())
            .arg(item->data(0, data_role::height).toUInt())
            .arg(item->data(1, data_role::width).toUInt())
            .arg(item->data(1, data_role::height).toUInt()));
    }

    return aliases;
}

// Send the user-defined alias resolutions from the GUI to the capture subsystem,
// which puts them into use.
void control_panel::capture::AliasResolutions::broadcast_aliases(void)
{
    std::vector<resolution_alias_s> aliases;

    auto items = this->ui->treeWidget_knownAliases->findItems("*", Qt::MatchWildcard);
    for (auto *item: items)
    {
        resolution_alias_s a;
        a.from.w = item->data(0, data_role::width).toUInt();
        a.from.h = item->data(0, data_role::height).toUInt();
        a.to.w = item->data(1, data_role::width).toUInt();
        a.to.h = item->data(1, data_role::height).toUInt();
        aliases.push_back(a);
    }

    ka_set_aliases(aliases);

    return;
}

void control_panel::capture::AliasResolutions::adjust_treewidget_column_size(void)
{
    this->ui->treeWidget_knownAliases->setColumnWidth(0, (this->ui->treeWidget_knownAliases->width() / 2));
    return;
}

void control_panel::capture::AliasResolutions::resizeEvent(QResizeEvent *)
{
    this->adjust_treewidget_column_size();
    return;
}

void control_panel::capture::AliasResolutions::showEvent(QShowEvent *event)
{
    this->adjust_treewidget_column_size();
    QWidget::showEvent(event);
    return;
}

void control_panel::capture::AliasResolutions::add_alias_to_list(const resolution_alias_s &a)
{
    // Returns a widget containing two QSpinBoxes, corresponding to a resolution's
    // width and height. In other words, with the widget, the user can specify a resolution.
    // This widget is meant to be embedded into a column on a QTreeWidgetItem; the
    // spinboxes' values are stored in the tree item with custom data roles.
    auto create_resolution_widget = [this](
        QTreeWidgetItem *parentItem,
        const uint column,
        const uint width,
        const uint height
    )->QWidget*
    {
        QWidget *container = new QWidget;
        QHBoxLayout *layout = new QHBoxLayout;
        container->setLayout(layout);

        QSpinBox *x = new QSpinBox;
        QSpinBox *y = new QSpinBox;

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
        layout->addWidget(y);

        return container;
    };

    QTreeWidgetItem *entry = new QTreeWidgetItem;
    this->ui->treeWidget_knownAliases->addTopLevelItem(entry);
    this->ui->treeWidget_knownAliases->setItemWidget(entry, 0, create_resolution_widget(entry, 0, a.from.w, a.from.h));
    this->ui->treeWidget_knownAliases->setItemWidget(entry, 1, create_resolution_widget(entry, 1, a.to.w, a.to.h));

    this->adjust_treewidget_column_size();

    return;
}

void control_panel::capture::AliasResolutions::clear_known_aliases(void)
{
    this->ui->treeWidget_knownAliases->clear();
    return;
}

void control_panel::capture::AliasResolutions::add_new_alias(void)
{
    resolution_alias_s newAlias;
    newAlias.from = {1, 1};
    newAlias.to = {640, 480};
    this->add_alias_to_list(newAlias);
    this->broadcast_aliases();
    return;
}

void control_panel::capture::AliasResolutions::remove_aliases(const QList<QTreeWidgetItem*> &aliasTreeItems)
{
    for (auto item: aliasTreeItems)
    {
        emit this->data_changed();
        delete item;
    }

    this->broadcast_aliases();

    return;
}

void control_panel::capture::AliasResolutions::remove_all_aliases(void)
{
    this->remove_aliases(this->ui->treeWidget_knownAliases->findItems("*", Qt::MatchWildcard));
    return;
}

void control_panel::capture::AliasResolutions::remove_selected_aliases(void)
{
    this->remove_aliases(this->ui->treeWidget_knownAliases->selectedItems());
    return;
}

void control_panel::capture::AliasResolutions::assign_aliases(const std::vector<resolution_alias_s> &aliases)
{
    this->ui->treeWidget_knownAliases->clear();

    for (const auto a: aliases)
    {
        this->add_alias_to_list(a);
    }

    return;
}
