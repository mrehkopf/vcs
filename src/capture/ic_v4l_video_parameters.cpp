/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifdef CAPTURE_API_VIDEO4LINUX

#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <linux/videodev2.h>
#include "capture/ic_v4l_video_parameters.h"

#define INCLUDE_VISION
#include <visionrgb/include/rgb133v4l2.h>

ic_v4l_video_parameters_c::ic_v4l_video_parameters_c(const int v4lDeviceFileHandle) :
    v4lDeviceFileHandle(v4lDeviceFileHandle)
{
    return;
}

bool ic_v4l_video_parameters_c::set_value(const int newValue, const ic_v4l_video_parameters_c::parameter_type_e parameterType)
{
    // If we don't recognize the given parameter.
    if (this->v4l_id(parameterType) < 0)
    {
        return false;
    }

    v4l2_control v4lc = {};
    v4lc.id = this->v4l_id(parameterType);
    v4lc.value = newValue;

    return bool(ioctl(this->v4lDeviceFileHandle, VIDIOC_S_CTRL, &v4lc) == 0);
}

int ic_v4l_video_parameters_c::value(const ic_v4l_video_parameters_c::parameter_type_e parameterType) const
{
    try
    {
        return this->parameters.at(parameterType).currentValue;
    }
    catch(...)
    {
        return 0;
    }
}

int ic_v4l_video_parameters_c::default_value(const ic_v4l_video_parameters_c::parameter_type_e parameterType) const
{
    try
    {
        return this->parameters.at(parameterType).defaultValue;
    }
    catch(...)
    {
        return 0;
    }
}

int ic_v4l_video_parameters_c::minimum_value(const ic_v4l_video_parameters_c::parameter_type_e parameterType) const
{
    try
    {
        return this->parameters.at(parameterType).minimumValue;
    }
    catch(...)
    {
        return 0;
    }
}

int ic_v4l_video_parameters_c::maximum_value(const ic_v4l_video_parameters_c::parameter_type_e parameterType) const
{
    try
    {
        return this->parameters.at(parameterType).maximumValue;
    }
    catch(...)
    {
        return 0;
    }
}

int ic_v4l_video_parameters_c::v4l_id(const ic_v4l_video_parameters_c::parameter_type_e parameterType) const
{
    try
    {
        return this->parameters.at(parameterType).v4lId;
    }
    catch(...)
    {
        return -1;
    }
}

std::string ic_v4l_video_parameters_c::name(const ic_v4l_video_parameters_c::parameter_type_e parameterType) const
{
    try
    {
        return this->parameters.at(parameterType).name;
    }
    catch(...)
    {
        return "unknown";
    }
}

void ic_v4l_video_parameters_c::update()
{
    this->parameters.clear();

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
                    video_parameter_s parameter;

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

                    this->parameters[this->type_for_name(parameter.name)] = parameter;
                }
            }
        }
    };

    enumerate_parameters(V4L2_CID_BASE, V4L2_CID_LASTP1);
    enumerate_parameters(V4L2_CID_PRIVATE_BASE, V4L2_CID_PRIVATE_LASTP1);

    return;
}

ic_v4l_video_parameters_c::parameter_type_e ic_v4l_video_parameters_c::type_for_name(const std::string &name)
{
    if (name == "horizontal_size")     return parameter_type_e::horizontal_size;
    if (name == "horizontal_position") return parameter_type_e::horizontal_position;
    if (name == "vertical_position")   return parameter_type_e::vertical_position;
    if (name == "phase")               return parameter_type_e::phase;
    if (name == "black_level")         return parameter_type_e::black_level;
    if (name == "brightness")          return parameter_type_e::brightness;
    if (name == "contrast")            return parameter_type_e::contrast;
    if (name == "red_brightness")      return parameter_type_e::red_brightness;
    if (name == "red_contrast")        return parameter_type_e::red_contrast;
    if (name == "green_brightness")    return parameter_type_e::green_brightness;
    if (name == "green_contrast")      return parameter_type_e::green_contrast;
    if (name == "blue_brightness")     return parameter_type_e::blue_brightness;
    if (name == "blue_contrast")       return parameter_type_e::blue_contrast;

    return parameter_type_e::unknown;
}

#endif
