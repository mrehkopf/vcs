/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_REFRESH_RATE_H
#define VCS_COMMON_REFRESH_RATE_H

#include <cmath>
#include <type_traits>

typedef int fixedpoint_hz_t;

// Represents a floating-point refresh rate value with 3 decimals of precision
// (e.g. 60.123999 would be truncated and represented as 60.123).
struct refresh_rate_s
{
    static const unsigned numDecimalsPrecision = 3;

    refresh_rate_s(void)
    {
        *this = 0.0;
    }

    refresh_rate_s(const double hz)
    {
        *this = hz;
    }

    refresh_rate_s(const int hz)
    {
        *this = double(hz);
    }

    template<typename T>
    T value(void) const
    {
        static_assert(
            (std::is_same<T, double>::value ||
             std::is_same<T, unsigned>::value ||
             std::is_same<T, int>::value),
            "Invalid type."
        );

        if (std::is_same<T, unsigned>::value ||
            std::is_same<T, int>::value)
        {
            return round(this->value<double>());
        }
        else if (std::is_same<T, double>::value)
        {
            return (this->fixedpoint / std::pow(10, double(numDecimalsPrecision)));
        }
    }

    static refresh_rate_s from_capture_device_properties(void);
    static void to_capture_device_properties(const refresh_rate_s &rate);
    void operator=(const double hz);
    void operator=(const unsigned hz);
    bool operator==(const refresh_rate_s &other) const;
    bool operator==(const double hz) const;
    bool operator!=(const refresh_rate_s &other) const;
    bool operator!=(const double hz) const;

    // The refresh rate's internal integer representation.
    fixedpoint_hz_t fixedpoint = 0;
};

#endif
