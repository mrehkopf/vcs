/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 * Handles setting/getting video parameters for a VCS V4L capture input channel.
 *
 */

#ifdef CAPTURE_DEVICE_VISION_V4L

#ifndef VCS_CAPTURE_IC_V4L_VIDEO_PARAMETERS_H
#define VCS_CAPTURE_IC_V4L_VIDEO_PARAMETERS_H

#include <string>
#include <unordered_map>

class ic_v4l_device_controls_c
{
public:
    // Takes as an argument the value of open("/dev/videoX"), where "/dev/videoX"
    // is the input channel (capture device) that the video parameters are for.
    ic_v4l_device_controls_c(const int v4lDeviceFileHandle = -1);

    // These correspond to the video parameters recognized by VCS.
    enum class control_type_e
    {
        horizontal_size,
        horizontal_position,
        vertical_position,
        phase,
        black_level,
        brightness,
        contrast,
        red_brightness,
        red_contrast,
        green_brightness,
        green_contrast,
        blue_brightness,
        blue_contrast,

        unknown,
    };

    // Ask the capture device to change the given parameters's value. Returns true
    // on success; false otherwise.
    bool set_value(const int newValue, const control_type_e controlType);

    int value(const control_type_e control) const;

    int default_value(const control_type_e control) const;

    int minimum_value(const control_type_e control) const;

    int maximum_value(const control_type_e control) const;

    int v4l_id(const control_type_e control) const;

    std::string name(const control_type_e control) const;

    // Polls the capture device for the current video parameters' values.
    void update(void);

private:
    control_type_e type_for_name(const std::string &name);

    // Data mined from the v4l2_queryctrl struct.
    struct control_data_s
    {
        std::string name;
        int currentValue;
        int minimumValue;
        int maximumValue;
        int defaultValue;
        int stepSize;
        /// TODO: Implement v4l2_queryctrl flags.

        int v4lId;
    };

    std::unordered_map<control_type_e, control_data_s> controls;

    int v4lDeviceFileHandle = -1;
};

#endif

#endif
