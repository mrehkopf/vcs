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
    return refresh_rate_s(kc_device_property("refresh rate") / double(refresh_rate_s::numDecimalsPrecision));
}

void refresh_rate_s::to_capture_device_properties(const refresh_rate_s &rate)
{
    kc_set_device_property("refresh rate", unsigned(refresh_rate_s::numDecimalsPrecision * rate.value<double>()));
}

unsigned refresh_rate_s::internal_value() const
{
    return this->internalValue;
}

void refresh_rate_s::operator=(const unsigned hz)
{
    *this = double(hz);
}

bool refresh_rate_s::operator==(const double hz) const
{
    return (this->internal_value() == refresh_rate_s(hz).internal_value());
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
    return (this->internal_value() == other.internal_value());
}

void refresh_rate_s::operator=(const double hz)
{
    this->internalValue = unsigned(round(hz * pow(10, numDecimalsPrecision)));
}
