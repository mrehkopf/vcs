#include <QPushButton>
#include <QVBoxLayout>
#include <QSplitter>
#include <QLabel>
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

    QVBoxLayout *const mainNaviLayout = dynamic_cast<QVBoxLayout*>(ui->buttonsContainer->layout());
    QVBoxLayout *const splitNaviLayout = dynamic_cast<QVBoxLayout*>(ui->buttonsContainer_2->layout());

    // Moves the given dialog entry from one split pane (layout) to another.
    const auto split = [this, mainNaviLayout, splitNaviLayout](
        DialogFragment *const dialog,
        QVBoxLayout *const to
    ){
        QPushButton *srcNaviButton = this->ui->buttonsContainer->findChild<QPushButton*>(dialog->name());
        QPushButton *dstNaviButton = this->ui->buttonsContainer_2->findChild<QPushButton*>(dialog->name());
        k_assert((srcNaviButton && dstNaviButton), "Malformed GUI: Unable to find matching dialog button.");
        if (to == mainNaviLayout)
        {
            std::swap(srcNaviButton, dstNaviButton);
        }

        bool doesSrcStillHaveContent = true;
        if (srcNaviButton->property("isSelected").toBool())
        {
            doesSrcStillHaveContent = false;
            auto *srcButtonsContainer = ((to == mainNaviLayout)? ui->buttonsContainer_2 : ui->buttonsContainer);
            for (auto *button: srcButtonsContainer->findChildren<QPushButton*>())
            {
                if (
                    !button->property("isSelected").toBool() &&
                    button->isEnabled()
                ){
                    doesSrcStillHaveContent = true;
                    button->click();
                    break;
                }
            }
        }

        auto *srcSection = ((to == ui->buttonsContainer->layout())? ui->splitSection : ui->mainSection);
        auto *dstSection = ((to == ui->buttonsContainer->layout())? ui->mainSection : ui->splitSection);
        if (!doesSrcStillHaveContent)
        {
            auto *srcButtonsContainer = ((to == mainNaviLayout)? ui->buttonsContainer_2 : ui->buttonsContainer);
            auto *dstButtonsContainer = ((to == mainNaviLayout)? ui->buttonsContainer : ui->buttonsContainer_2);
            auto *srcContentContainer = ((to == mainNaviLayout)? ui->scrollArea_2 : ui->scrollArea);
            auto *dstContentContainer = ((to == mainNaviLayout)? ui->scrollArea : ui->scrollArea_2);

            if (to == splitNaviLayout)
            {
                std::swap(srcButtonsContainer, dstButtonsContainer);
                std::swap(srcContentContainer, dstContentContainer);
                std::swap(srcSection, dstSection);
            }

            for (auto *button: srcButtonsContainer->findChildren<QPushButton*>())
            {
                button->setVisible(false);
                button->setEnabled(false);
            }
            for (auto *button: dstButtonsContainer->findChildren<QPushButton*>())
            {
                button->setVisible(true);
                button->setEnabled(true);
            }

            srcContentContainer->takeWidget();
            srcSection->setVisible(false);
        }
        else
        {
            dstSection->setVisible(true);
            srcNaviButton->setVisible(false);
            srcNaviButton->setEnabled(false);
            dstNaviButton->setVisible(true);
            dstNaviButton->setEnabled(true);

            if (to == splitNaviLayout)
            {
                dstNaviButton->click();
            }
        }
    };

    const auto add_navi_button = [this, split, mainNaviLayout, splitNaviLayout](DialogFragment *const dialog)
    {
        for (auto *layout: {mainNaviLayout, splitNaviLayout})
        {
            auto *const naviButton = new QPushButton(dialog->name());
            naviButton->setObjectName(dialog->name());
            naviButton->setFocusPolicy(Qt::NoFocus);
            naviButton->setEnabled(true);
            naviButton->setFlat(true);
            layout->addWidget(naviButton);

            connect(naviButton, &QPushButton::pressed, this, [this, layout, splitNaviLayout, mainNaviLayout, split, naviButton, dialog]
            {
                auto *const contentArea = ((layout == splitNaviLayout)? ui->scrollArea_2 : ui->scrollArea);
                auto *const buttonArea = ((layout == splitNaviLayout)? ui->buttonsContainer_2 : ui->buttonsContainer);

                if (
                    naviButton->underMouse() &&
                    (qApp->keyboardModifiers() & Qt::AltModifier)
                ){
                    split(dialog, ((layout == mainNaviLayout)? splitNaviLayout : mainNaviLayout));
                    return;
                }

                contentArea->takeWidget();
                contentArea->setWidget(dialog);
                dialog->show();

                for (auto *const childButton: buttonArea->findChildren<QPushButton*>())
                {
                    childButton->setProperty("isSelected", ((childButton == naviButton)? "true" : "false"));
                    this->style()->polish(dynamic_cast<QWidget*>(childButton));
                }
            });

            // The split view is hidden by default.
            if (layout == splitNaviLayout)
            {
                naviButton->setVisible(false);
                naviButton->setEnabled(false);
            }
        }
    };

    // Populate the side navi.
    {
        add_navi_button(this->captureDialog);
        add_navi_button(this->outputDialog);
        if (kc_device_property("supports video presets"))
        {
            add_navi_button(this->videoPresetsDialog);
        }
        add_navi_button(this->filterGraphDialog);
        add_navi_button(this->aboutDialog);

        // Push the buttons against the top edge of the container.
        mainNaviLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
        splitNaviLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));

        dynamic_cast<QPushButton*>(ui->buttonsContainer->findChildren<QPushButton*>().at(0))->click();
    }

    // Restore persistent settings.
    {
        this->resize(kpers_value_of(INI_GROUP_CONTROL_PANEL_WINDOW, "Size", QSize(792, 712)).toSize());
    }

    // Note: The wheel blocker has to be created prior to setting up the view
    // splitter, otherwise the wheel blocking doesn't apply to all widgets.
    this->wheelBlocker = new WheelBlocker(this);

    // Wire up splitter functionality.
    {
        QSplitter *const splitter = new QSplitter(this);
        splitter->addWidget(ui->mainSection);
        splitter->addWidget(ui->splitSection);
        splitter->setOrientation(Qt::Vertical);
        splitter->setStretchFactor(1, 1);
        splitter->setHandleWidth(1);
        this->layout()->addWidget(splitter);

        // The split is empty and hidden by default.
        this->ui->splitSection->setVisible(false);
    }

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
