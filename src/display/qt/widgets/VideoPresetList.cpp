/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "display/qt/widgets/VideoPresetList.h"
#include "capture/video_presets.h"
#include "common/globals.h"

VideoPresetList::VideoPresetList(QWidget *parent) : QComboBox(parent)
{
    connect(this, QOverload<int>::of(&VideoPresetList::currentIndexChanged), this, [this](const int idx)
    {
        if (idx < 0)
        {
            return;
        }

        const unsigned presetId = this->currentData().toUInt();
        emit this->preset_selected(presetId);
    });

    return;
}

VideoPresetList::~VideoPresetList(void)
{
    return;
}

void VideoPresetList::remove_all_presets(void)
{
    if (this->count() <= 0)
    {
        return;
    }

    this->clear();

    emit this->list_became_empty();

    return;
}

void VideoPresetList::sort_alphabetically(void)
{
    this->model()->sort(0);

    return;
}

void VideoPresetList::add_preset(const unsigned presetId,
                                 const bool blockSignal)
{
    const auto *const preset = kvideopreset_get_preset_ptr(presetId);

    if (!preset)
    {
        return;
    }

    {
        const QSignalBlocker block(blockSignal? this : nullptr);
        this->addItem(this->make_preset_item_label(preset), presetId);
    }

    emit this->preset_added(presetId);

    if (this->count() == 1)
    {
        emit this->list_became_populated();
    }

    // Select the item we added.
    {
        const int presetListIdx = this->find_preset_idx_in_list(presetId);

        k_assert((presetListIdx >= 0), "Failed to add a preset to the video preset list.");

        this->setCurrentIndex(presetListIdx);
    }

    return;
}

void VideoPresetList::remove_preset(const unsigned presetId)
{
    const int presetListIdx = this->find_preset_idx_in_list(presetId);

    // Prevent the removeItem() call from changing the list index. We'll assign
    // a new index manually, later.
    {
        const QSignalBlocker block(this);
        this->removeItem(presetListIdx);
        this->setCurrentIndex(-1);
    }

    emit this->preset_removed(presetId);

    if (this->count() <= 0)
    {
        emit this->list_became_empty();
    }
    else
    {
        this->setCurrentIndex(0);
    }

    return;
}

int VideoPresetList::find_preset_idx_in_list(const int presetId)
{
    if (presetId < 0)
    {
        return -1;
    }

    for (int i = 0; i < this->count(); i++)
    {
        if (this->itemData(i).toInt() == presetId)
        {
            return i;
        }
    }

    return -1;
}

void VideoPresetList::update_preset_item_label(const unsigned presetId)
{
    const auto *const preset = kvideopreset_get_preset_ptr(presetId);

    if (!preset)
    {
        return;
    }

    const int presetListIdx = this->find_preset_idx_in_list(presetId);

    if (presetListIdx < 0)
    {
        return;
    }

    const QString prevText = this->itemText(presetListIdx);
    const QString newText = this->make_preset_item_label(preset);

    if (prevText != newText)
    {
        this->setItemText(presetListIdx, newText);
        emit this->preset_label_changed(presetId, newText);
    }

    return;
}

analog_video_preset_s *VideoPresetList::current_preset(void) const
{
    if (this->count() <= 0)
    {
        return nullptr;
    }

    return kvideopreset_get_preset_ptr(this->currentData().toUInt());
}

QString VideoPresetList::make_preset_item_label(const analog_video_preset_s *const preset)
{
    QStringList text;

    if (preset->activatesWithShortcut)
    {
        text << QString("[%1]").arg(QString::fromStdString(preset->activationShortcut));
    }

    if (preset->activatesWithResolution)
    {
        text << QString("%1 \u00d7 %2").arg(QString::number(preset->activationResolution.w))
                                       .arg(QString::number(preset->activationResolution.h));
    }

    if (preset->activatesWithRefreshRate)
    {
        if (preset->refreshRateComparator == analog_video_preset_s::refresh_rate_comparison_e::equals)
        {
            text << QString("%1 Hz").arg(QString::number(preset->activationRefreshRate.value<double>(), 'f', 3));
        }
        else
        {
            text << QString("%1 Hz").arg(QString::number(preset->activationRefreshRate.value<double>()));
        }
    }

    if (!preset->name.empty())
    {
        text << QString("\"%1\"").arg(QString::fromStdString(preset->name));
    }

    QString combinedText = text.join(" \u00b7 ");

    if (combinedText.isEmpty())
    {
        combinedText = "(Empty preset)";
    }

    return combinedText;
}
