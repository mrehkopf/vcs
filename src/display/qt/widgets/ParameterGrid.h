/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QGROUPBOX_PARAMETER_GRID_H
#define VCS_DISPLAY_QT_SUBCLASSES_QGROUPBOX_PARAMETER_GRID_H

#include <QGroupBox>
#include <vector>

class QSpinBox;
class QScrollBar;
class QPushButton;
class QComboBox;

class ParameterGrid : public QGroupBox
{
    Q_OBJECT

public:
    explicit ParameterGrid(QWidget *parent = 0);

    ~ParameterGrid();

    // A horizontal visual separator between grid elements; e.g. a line.
    void add_separator(void);

    // A drop-down combobox; for selecting from pre-set options.
    void add_combobox(
        const QString name,
        const std::list<QString> items
    );

    // A scroll bar with a synced spin box; for selecting a value on an interval.
    void add_scroller(
        const QString name,
        const int valueInitial = 1,
        const int valueMin = 0,
        const int valueMax = 999
    );

    int value(const QString &parameterName) const;
    int maximum_value(const QString &parameterName) const;
    int minimum_value(const QString &parameterName) const;

    void set_value(const QString &parameterName, const int newValue);
    void set_maximum_value(const QString &parameterName, const int newMax);
    void set_minimum_value(const QString &parameterName, const int newMin);

signals:
    // Emitted when a parameter's value is changed either by the user via the
    // GUI or by a call to this->set_value().
    void parameter_value_changed(const QString &parameterName, const int newValue);

    // Emitted when a parameter's value is changed by the user via the GUI.
    void parameter_value_changed_by_user(const QString &parameterName, const int newValue);

    // Emitted when a parameter's value is changed by a call to this->set_value().
    void parameter_value_changed_programmatically(const QString &parameterName, const int newValue);

    void parameter_minimum_value_changed_programmatically(const QString &parameterName, const int newMin);
    void parameter_maximum_value_changed_programmatically(const QString &parameterName, const int newMax);

private:
    enum class parameter_type_e
    {
        unknown,
        scroller,
        combobox,
    };

    struct parameter_meta_s
    {
        parameter_type_e type = parameter_type_e::unknown;
        QString name = "";
        int currentValue = 0;
        int minimumValue = 0;
        int maximumValue = 0;
    };

    ParameterGrid::parameter_meta_s* parameter(const QString &parameterName) const;

    std::vector<parameter_meta_s*> parameters;
};

#endif
