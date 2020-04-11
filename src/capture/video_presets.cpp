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

static unsigned RUNNING_PRESET_ID = 0;

void kvideopreset_release(void)
{
    kvideopreset_remove_all_presets();

    return;
}

bool kvideopreset_remove_preset(const unsigned presetId)
{
    const auto targetPreset = std::find_if(PRESETS.begin(), PRESETS.end(), [=](video_preset_s *p){return p->id == presetId;});

    if (targetPreset == PRESETS.end())
    {
        return false;
    }

    PRESETS.erase(targetPreset);

    return true;
}

static video_preset_s* strongest_activating_preset(void)
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

void kvideopreset_remove_all_presets(void)
{
    for (auto preset: PRESETS)
    {
        delete preset;
    }

    PRESETS.clear();
    RUNNING_PRESET_ID = 0;

    return;
}

void kvideoparam_preset_video_params_changed(const unsigned presetId)
{
    const video_preset_s *thisPreset = kvideopreset_get_preset(presetId);
    const video_preset_s *activePreset = strongest_activating_preset();

    if (thisPreset == activePreset)
    {
        kc_capture_api().set_video_signal_parameters(thisPreset->videoParameters);
    }
}

void kvideopreset_apply_current_active_preset(void)
{
    const video_preset_s *activePreset = strongest_activating_preset();

    if (activePreset)
    {
        kc_capture_api().set_video_signal_parameters(activePreset->videoParameters);
    }
    else
    {
        kc_capture_api().set_video_signal_parameters(kc_capture_api().get_default_video_signal_parameters());
    }

    return;
}

const std::vector<video_preset_s*>& kvideopreset_all_presets(void)
{
    return PRESETS;
}

video_preset_s* kvideopreset_get_preset(const unsigned presetId)
{
    for (auto preset: PRESETS)
    {
        if (preset->id == presetId)
        {
            return preset;
        }
    }

    return nullptr;
}

void kvideopreset_assign_presets(const std::vector<video_preset_s*> &presets)
{
    kvideopreset_remove_all_presets();

    PRESETS = presets;

    return;
}

video_preset_s* kvideopreset_create_preset(void)
{
    video_preset_s *preset = new video_preset_s();

    preset->id = ++RUNNING_PRESET_ID;
    preset->name = ("Video preset #" + std::to_string(preset->id));
    preset->activatesWithResolution = true;
    preset->activationResolution = {640, 480, 32};

    preset->videoParameters = kc_capture_api().get_default_video_signal_parameters();

    PRESETS.push_back(preset);

    return preset;
}

video_signal_parameters_s kvideopreset_current_video_parameters(void)
{
    const auto activePreset = strongest_activating_preset();

    if (!activePreset)
    {
        return {};
    }
    else
    {
        return activePreset->videoParameters;
    }
}
