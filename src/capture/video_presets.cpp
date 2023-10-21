/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <algorithm>
#include <vector>
#include "capture/video_presets.h"
#include "capture/capture.h"

vcs_event_c<const analog_video_preset_s*> kc_ev_video_preset_params_changed;

static std::vector<analog_video_preset_s*> PRESETS;

// Incremented for each new preset added, and used as the id for that preset.
static unsigned RUNNING_PRESET_ID = 0;

static const analog_video_preset_s *MOST_RECENT_ACTIVE_PRESET = nullptr;

static analog_video_preset_s* strongest_activating_preset(void)
{
    if (!kc_has_signal())
    {
        return nullptr;
    }

    const auto resolution = resolution_s::from_capture_device_properties();
    const auto refreshRate = refresh_rate_s::from_capture_device_properties();

    std::vector<std::pair<unsigned/*preset id*/,
                          int/*preset activation level*/>> activationLevels;

    int strongestPresetId = -1;
    int highestActivationLevel = 0;

    for (auto *preset: PRESETS)
    {
        const int activationLevel = preset->activation_level(resolution, refreshRate);

        if (activationLevel > highestActivationLevel)
        {
            highestActivationLevel = activationLevel;
            strongestPresetId = int(preset->id);
        }
    }

    // If no presets activated strongly enough.
    if ((highestActivationLevel <= 0) ||
        (strongestPresetId < 0))
    {
        return nullptr;
    }
    else
    {
        return kvideopreset_get_preset_ptr(unsigned(strongestPresetId));
    }
}

subsystem_releaser_t kvideopreset_initialize(void)
{
    DEBUG(("Initializing the video presets subsystem."));

    // Listen for app events.
    {
        ev_new_video_mode.listen(kvideopreset_apply_current_active_preset);

        kc_ev_video_preset_params_changed.listen([](const analog_video_preset_s *preset)
        {
            k_assert(preset, "Expected a non-null preset.");

            if (preset == MOST_RECENT_ACTIVE_PRESET)
            {
                analog_properties_s::to_capture_device_properties(preset->properties);
            }
        });

        ev_video_preset_activated.listen([](const analog_video_preset_s *preset)
        {
            MOST_RECENT_ACTIVE_PRESET = preset;
        });
    }

    return []{};
}

bool kvideopreset_is_preset_active(const analog_video_preset_s *const preset)
{
    return (
        MOST_RECENT_ACTIVE_PRESET &&
        (preset == MOST_RECENT_ACTIVE_PRESET)
    );
}

bool kvideopreset_remove_preset(const unsigned presetId)
{
    const auto targetPreset = std::find_if(PRESETS.begin(), PRESETS.end(), [=](analog_video_preset_s *p){return p->id == presetId;});

    if (targetPreset == PRESETS.end())
    {
        return false;
    }

    if ((*targetPreset) == MOST_RECENT_ACTIVE_PRESET)
    {
        MOST_RECENT_ACTIVE_PRESET = nullptr;
    }

    delete (*targetPreset);
    PRESETS.erase(targetPreset);

    return true;
}

void kvideopreset_remove_all_presets(void)
{
    for (auto *preset: PRESETS)
    {
        delete preset;
    }

    PRESETS.clear();
    RUNNING_PRESET_ID = 0;
    MOST_RECENT_ACTIVE_PRESET = nullptr;

    return;
}

void kvideopreset_apply_current_active_preset(void)
{
    if (!kc_has_signal())
    {
        return;
    }

    const analog_video_preset_s *activePreset = strongest_activating_preset();

    if (activePreset)
    {
        analog_properties_s::to_capture_device_properties(activePreset->properties);
    }
    else
    {
        analog_properties_s::to_capture_device_properties(analog_properties_s::from_capture_device_properties(": default"));
    }

    ev_video_preset_activated.fire(activePreset);

    return;
}

const std::vector<analog_video_preset_s*>& kvideopreset_all_presets(void)
{
    return PRESETS;
}

analog_video_preset_s* kvideopreset_get_preset_ptr(const unsigned presetId)
{
    for (auto *preset: PRESETS)
    {
        if (preset->id == presetId)
        {
            return preset;
        }
    }

    return nullptr;
}

void kvideopreset_activate_keyboard_shortcut(const std::string &shortcutString)
{
    if (!kc_has_signal())
    {
        return;
    }

    for (auto *preset: PRESETS)
    {
        if (preset->activates_with_shortcut(shortcutString))
        {
            analog_properties_s::to_capture_device_properties(preset->properties);
            ev_video_preset_activated.fire(preset);
            return;
        }
    }

    return;
}

void kvideopreset_assign_presets(const std::vector<analog_video_preset_s*> &presets)
{
    kvideopreset_remove_all_presets();

    PRESETS = presets;
    kvideopreset_apply_current_active_preset();

    RUNNING_PRESET_ID = (1 + (*std::max_element(PRESETS.begin(), PRESETS.end(), [](const analog_video_preset_s *a, const analog_video_preset_s *b){return a->id < b->id;}))->id);

    return;
}

analog_video_preset_s* kvideopreset_create_new_preset(const analog_video_preset_s *const duplicateDataSrc)
{
    analog_video_preset_s *const preset = new analog_video_preset_s;
    const std::string baseName = (duplicateDataSrc? "Duplicate preset" : "Preset");

    if (duplicateDataSrc)
    {
        *preset = *duplicateDataSrc;
    }
    else
    {
        preset->activatesWithResolution = false;
        preset->activationResolution = {.w = 640, .h = 480};
        preset->properties = analog_properties_s::from_capture_device_properties(": default");
    }

    preset->id = RUNNING_PRESET_ID++;
    preset->name = (baseName + " " + std::to_string(RUNNING_PRESET_ID));

    PRESETS.push_back(preset);

    return preset;
}

const analog_video_preset_s* kvideopreset_current_active_preset()
{
    return MOST_RECENT_ACTIVE_PRESET;
}

properties_map_t analog_properties_s::from_capture_device_properties(const std::string &nameSpace)
{
    properties_map_t properties;

    for (const std::string propName: kc_supported_video_preset_properties())
    {
        properties[propName] = kc_device_property(propName + nameSpace);
    }

    return properties;
}

void analog_properties_s::to_capture_device_properties(
    const properties_map_t &properties,
    const std::string &nameSpace
)
{
    for (const auto &[propName, value]: properties)
    {
        kc_set_device_property((propName + nameSpace), value);
    }

    return;
}
