/*
 * Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include "display/display.h"
#include "capture/capture.h"

vcs_event_c<void> kd_ev_dirty;

resolution_s resolution_s::from_capture_device(const std::string &nameSpace)
{
    return resolution_s{
        .w = unsigned(kc_device_property("width" + nameSpace)),
        .h = unsigned(kc_device_property("height" + nameSpace)),
        .bpp = 32
    };
}

void resolution_s::to_capture_device(const resolution_s &resolution)
{
    kc_set_device_property("width", resolution.w);
    kc_set_device_property("height", resolution.h);
}

bool resolution_s::operator==(const resolution_s &other) const
{
    return (
        (this->w == other.w) &&
        (this->h == other.h) &&
        (this->bpp == other.bpp)
    );
}

bool resolution_s::operator!=(const resolution_s &other) const
{
    return !(*this == other);
}
