#include <QPushButton>
#include <QVBoxLayout>
#include <QStyle>
#include "display/qt/windows/control_panel/capture.h"
#include "display/qt/windows/control_panel/output.h"
#include "display/qt/windows/control_panel/video_presets.h"
#include "display/qt/windows/control_panel/filter_graph.h"
#include "display/qt/windows/control_panel/about_vcs.h"
#include "display/qt/windows/control_panel/overlay.h"
#include "display/qt/windows/output_window.h"
#include "control_panel.h"
#include "ui_control_panel.h"

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
    this->overlayDialog = new control_panel::Overlay(this);
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
        add_navi_button("Filter graph", this->filterGraphDialog);
        add_navi_button("Video presets", this->videoPresetsDialog);
        add_navi_button("Overlay", this->overlayDialog);
        add_navi_button("About VCS", this->aboutDialog);

        // Push the buttons against the top edge of the container.
        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

        defaultButton->click();
    }
}

ControlPanel::~ControlPanel()
{
    delete ui;
}
