#include <QPushButton>
#include <QVBoxLayout>
#include <QStyle>
#include "display/qt/persistent_settings.h"
#include "display/qt/windows/ControlPanel/Capture.h"
#include "display/qt/windows/ControlPanel/Output.h"
#include "display/qt/windows/ControlPanel/VideoPresets.h"
#include "display/qt/windows/ControlPanel/FilterGraph.h"
#include "display/qt/windows/ControlPanel/AboutVCS.h"
#include "display/qt/windows/OutputWindow.h"
#include "display/qt/wheel_blocker.h"
#include "capture/capture.h"
#include "ControlPanel.h"
#include "ui_ControlPanel.h"

ControlPanel::ControlPanel(OutputWindow *parent) :
    QWidget(parent),
    ui(new Ui::ControlPanel)
{
    ui->setupUi(this);

    this->setWindowTitle("Control panel - VCS");
    this->setWindowFlags(Qt::Window);

    this->captureDialog = new control_panel::Capture(this);
    this->outputDialog = new control_panel::Output(this);
    this->filterGraphDialog = new control_panel::FilterGraph(this);
    this->videoPresetsDialog = new control_panel::VideoPresets(this);
    this->aboutDialog = new control_panel::AboutVCS(this);

    // Populate the side navi.
    {
        QVBoxLayout *const layout = dynamic_cast<QVBoxLayout*>(ui->buttonsContainer->layout());

        const auto add_navi_button = [this, layout](const QString &label, DialogFragment *const dialog)->QPushButton*
        {
            auto *const naviButton = new QPushButton(label);
            naviButton->setFlat(true);
            naviButton->setFocusPolicy(Qt::NoFocus);
            layout->addWidget(naviButton);

            connect(naviButton, &QPushButton::pressed, this, [this, naviButton, dialog]
            {
                ui->contentsScroller->takeWidget();
                ui->contentsScroller->setWidget(dialog);
                dialog->show();

                foreach (auto *const childButton, this->ui->buttonsContainer->children())
                {
                    childButton->setProperty("isSelected", ((childButton == naviButton)? "true" : "false"));
                    this->style()->polish(dynamic_cast<QWidget*>(childButton));
                };
            });

            return naviButton;
        };

        auto *const defaultButton = add_navi_button("Capture", this->captureDialog);
        add_navi_button("Output", this->outputDialog);
        if (kc_device_property("supports video presets"))
        {
            add_navi_button("Video presets", this->videoPresetsDialog);
        }
        add_navi_button("Filter graph", this->filterGraphDialog);
        add_navi_button("About VCS", this->aboutDialog);

        // Push the buttons against the top edge of the container.
        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

        defaultButton->click();
    }

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_CONTROL_PANEL_WINDOW, "Size", QSize(792, 712)).toSize());
    }

    this->wheelBlocker = new WheelBlocker(this);

    return;
}

ControlPanel::~ControlPanel()
{
    delete this->ui;
    delete this->wheelBlocker;
    
    return;
}

void ControlPanel::resizeEvent(QResizeEvent *)
{
    kpers_set_value(INI_GROUP_CONTROL_PANEL_WINDOW, "Size", this->size());

    return;
}
