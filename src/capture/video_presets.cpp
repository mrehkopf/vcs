/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <algorithm>
#include <vector>
#include "capture/video_presets.h"
#include "capture/capture_api.h"
#include "capture/capture.h"

static std::vector<video_preset_s*> PRESETS;

// The id of the preset that's currently being applied; or -1 if no preset is
// currently active.
static int ACTIVE_PRESET_ID;

void kvideopreset_release(void)
{
    for (auto preset: PRESETS)
    {
        delete preset;
    }

    return;
}

bool kvideopreset_remove_preset(const unsigned presetId)
{
    const auto targetPreset = std::find_if(PRESETS.begin(), PRESETS.end(), [=](video_preset_s *p){return p->id == presetId;});

    if (targetPreset == PRESETS.end())
    {
        return false;
    }

    DEBUG(("Deleting %s", (*targetPreset)->name.c_str()));

    PRESETS.erase(targetPreset);

    return true;
}

static video_preset_s* strongest_activator(void)
{
    const resolution_s resolution = kc_capture_api().get_resolution();
    const refresh_rate_s refreshRate = kc_capture_api().get_refresh_rate();

    std::vector<std::pair<unsigned/*preset id*/,
                          int/*preset activation level*/>> activationLevels;

    ACTIVE_PRESET_ID = -1;
    int highestActivationLevel = 0;

    for (auto preset: PRESETS)
    {
        const int activationLevel = preset->activation_level(resolution, refreshRate);

        if (activationLevel > highestActivationLevel)
        {
            highestActivationLevel = activationLevel;
            ACTIVE_PRESET_ID = int(preset->id);
        }
    }

    // If no presets activated strongly enough.
    if ((highestActivationLevel <= 0) ||
        (ACTIVE_PRESET_ID < 0))
    {
        ACTIVE_PRESET_ID = -1;
        return nullptr;
    }
    else
    {
        return kvideopreset_get_preset(unsigned(ACTIVE_PRESET_ID));
    }
}

void kvideopreset_update_preset_parameters(const unsigned presetId, const video_signal_parameters_s &params)
{
    video_preset_s *targetPreset = kvideopreset_get_preset(presetId);
    const video_preset_s *activePreset = strongest_activator();

    targetPreset->videoParameters = params;

    if (targetPreset == activePreset)
    {
        kc_capture_api().set_video_signal_parameters(params);
    }

    return;
}

video_preset_s *kvideopreset_get_preset(const unsigned presetId)
{
    for (auto preset: PRESETS)
    {
        if (preset->id == presetId)
        {
            return preset;
        }
    }

    k_assert(0, "Unknown video preset id.");
}

video_preset_s* kvideopreset_create_preset(void)
{
    video_preset_s *preset = new video_preset_s();

    preset->id = unsigned(PRESETS.size());
    preset->name = ("Preset #" + std::to_string(preset->id + 1));
    preset->activatesWithResolution = true;
    preset->activationResolution = {640, 480, 32};

    PRESETS.push_back(preset);

    return preset;
}

video_signal_parameters_s kvideopreset_current_video_parameters(void)
{
    const auto activePreset = strongest_activator();

    if (!activePreset)
    {
        return {};
    }
    else
    {
        return activePreset->videoParameters;
    }
}
