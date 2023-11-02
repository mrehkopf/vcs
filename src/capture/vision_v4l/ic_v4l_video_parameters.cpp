/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <linux/videodev2.h>
#include "common/globals.h"
#include "capture/capture.h"
#include "capture/vision_v4l/ic_v4l_video_parameters.h"

#define INCLUDE_VISION
#include <rgb133control.h>
#include <rgb133v4l2.h>

ic_v4l_controls_c::ic_v4l_controls_c(const int v4lDeviceFileHandle) :
    v4lDeviceFileHandle(v4lDeviceFileHandle)
{
    return;
}

bool ic_v4l_controls_c::set_value(const int newValue, const ic_v4l_controls_c::type_e control)
{
    // If there's no signal, we assume V4L doesn't make any properties available
    // for modification, se we don't bother trying.
    if (!kc_has_signal())
    {
        return false;
    }

    if (this->v4l_id(control) < 0)
    {
        DEBUG(("Asked to modify a V4L control that isn't available. Ignoring this."));
        return false;
    }

    v4l2_control v4lc = {};
    v4lc.id = this->v4l_id(control);
    v4lc.value = newValue;

    if (ioctl(this->v4lDeviceFileHandle, VIDIOC_S_CTRL, &v4lc) != 0)
    {
        if (kc_has_signal())
        {
            DEBUG(("V4L control: Unsupported value %d for %s.", newValue, this->name(control).c_str()));
        }

        return false;
    }

    return true;
}

int ic_v4l_controls_c::value(const ic_v4l_controls_c::type_e control) const
{
    try
    {
        return this->controls.at(control).currentValue;
    }
    catch(...)
    {
        return 0;
    }
}

int ic_v4l_controls_c::default_value(const ic_v4l_controls_c::type_e control) const
{
    try
    {
        return this->controls.at(control).defaultValue;
    }
    catch(...)
    {
        return 0;
    }
}

int ic_v4l_controls_c::minimum_value(const ic_v4l_controls_c::type_e control) const
{
    try
    {
        return this->controls.at(control).minimumValue;
    }
    catch(...)
    {
        return 0;
    }
}

int ic_v4l_controls_c::maximum_value(const ic_v4l_controls_c::type_e control) const
{
    try
    {
        return this->controls.at(control).maximumValue;
    }
    catch(...)
    {
        return 0;
    }
}

int ic_v4l_controls_c::v4l_id(const ic_v4l_controls_c::type_e control) const
{
    try
    {
        return this->controls.at(control).v4lId;
    }
    catch(...)
    {
        return -1;
    }
}

std::string ic_v4l_controls_c::name(const ic_v4l_controls_c::type_e control) const
{
    try
    {
        return this->controls.at(control).name;
    }
    catch(...)
    {
        return "unknown";
    }
}

void ic_v4l_controls_c::update()
{
    this->controls.clear();

    const auto enumerate_parameters = [this](const unsigned startID, const unsigned endID)
    {
        for (unsigned i = startID; i < endID; i++)
        {
            v4l2_queryctrl query = {};
            query.id = i;

            if (ioctl(this->v4lDeviceFileHandle, VIDIOC_QUERYCTRL, &query) == 0)
            {
                if (!(query.flags & V4L2_CTRL_FLAG_DISABLED) &&
                    !(query.flags & V4L2_CTRL_FLAG_READ_ONLY) &&
                    !(query.flags & V4L2_CTRL_FLAG_INACTIVE) &&
                    !(query.flags & V4L2_CTRL_FLAG_HAS_PAYLOAD) &&
                    !(query.flags & V4L2_CTRL_FLAG_WRITE_ONLY))
                {
                    control_data_s parameter;

                    parameter.name = (char*)query.name;
                    parameter.v4lId = i;
                    parameter.minimumValue = query.minimum;
                    parameter.maximumValue = query.maximum;
                    parameter.defaultValue = query.default_value;
                    parameter.stepSize = query.step;

                    // Attempt to get the control's current value.
                    {
                        v4l2_control v4lc = {};
                        v4lc.id = i;

                        if (ioctl(this->v4lDeviceFileHandle, VIDIOC_G_CTRL, &v4lc) == 0)
                        {
                            parameter.currentValue = v4lc.value;
                        }
                        else
                        {
                            parameter.currentValue = 0;
                        }
                    }

                    // Standardize the control names.
                    for (auto &chr: parameter.name)
                    {
                        chr = ((chr == ' ')? '_' : (char)std::tolower(chr));
                    }

                    this->controls[this->type_for_name(parameter.name)] = parameter;
                }
            }
        }
    };

    enumerate_parameters(V4L2_CID_BASE, V4L2_CID_LASTP1);
    enumerate_parameters(V4L2_CID_PRIVATE_BASE, V4L2_CID_PRIVATE_LASTP1);

    return;
}

ic_v4l_controls_c::type_e ic_v4l_controls_c::type_for_name(const std::string &name)
{
    if (name == "signal_type")         return ic_v4l_controls_c::type_e::signal_type;
    if (name == "horizontal_size")     return ic_v4l_controls_c::type_e::horizontal_size;
    if (name == "horizontal_position") return ic_v4l_controls_c::type_e::horizontal_position;
    if (name == "vertical_position")   return ic_v4l_controls_c::type_e::vertical_position;
    if (name == "phase")               return ic_v4l_controls_c::type_e::phase;
    if (name == "black_level")         return ic_v4l_controls_c::type_e::black_level;
    if (name == "brightness")          return ic_v4l_controls_c::type_e::brightness;
    if (name == "contrast")            return ic_v4l_controls_c::type_e::contrast;
    if (name == "red_brightness")      return ic_v4l_controls_c::type_e::red_brightness;
    if (name == "red_contrast")        return ic_v4l_controls_c::type_e::red_contrast;
    if (name == "green_brightness")    return ic_v4l_controls_c::type_e::green_brightness;
    if (name == "green_contrast")      return ic_v4l_controls_c::type_e::green_contrast;
    if (name == "blue_brightness")     return ic_v4l_controls_c::type_e::blue_brightness;
    if (name == "blue_contrast")       return ic_v4l_controls_c::type_e::blue_contrast;

    return type_e::unknown;
}
