/*
 * 2023 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "common/refresh_rate.h"
#include "capture/capture.h"

refresh_rate_s refresh_rate_s::from_capture_device_properties()
{
    refresh_rate_s r;
    r.fixedpoint = kc_device_property("refresh rate");
    return r;
}

void refresh_rate_s::to_capture_device_properties(const refresh_rate_s &rate)
{
    kc_set_device_property("refresh rate", rate.fixedpoint);
}

void refresh_rate_s::operator=(const unsigned hz)
{
    *this = double(hz);
}

void refresh_rate_s::operator=(const double hz)
{
    this->fixedpoint = unsigned(std::round(hz * std::pow(10, numDecimalsPrecision)));
}

bool refresh_rate_s::operator==(const double hz) const
{
    return (*this == refresh_rate_s(hz));
}

bool refresh_rate_s::operator!=(const double hz) const
{
    return !(*this == hz);
}

bool refresh_rate_s::operator!=(const refresh_rate_s &other) const
{
    return !(*this == other);
}

bool refresh_rate_s::operator==(const refresh_rate_s &other) const
{
    return (this->fixedpoint == other.fixedpoint);
}
