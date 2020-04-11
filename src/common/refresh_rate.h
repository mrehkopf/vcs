/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef REFRESH_RATE_H
#define REFRESH_RATE_H

#include <cmath>
#include <type_traits>

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
        static_assert((std::is_same<T, double>::value ||
                       std::is_same<T, unsigned>::value ||
                       std::is_same<T, int>::value),
                      "Invalid type.");

        if (std::is_same<T, unsigned>::value ||
            std::is_same<T, int>::value)
        {
            return round(this->value<double>());
        }
        else if (std::is_same<T, double>::value)
        {
            return (this->internalValue / pow(10.0, double(numDecimalsPrecision)));
        }
        else if (std::is_same<T, float>::value)
        {
            return (this->internalValue / powf(10.0f, float(numDecimalsPrecision)));
        }
    }

    unsigned internal_value(void) const
    {
        return this->internalValue;
    }

    void operator=(const double hz)
    {
        this->internalValue = unsigned(hz * pow(10, numDecimalsPrecision));
    }

    void operator=(const unsigned hz)
    {
        *this = double(hz);
    }

    bool operator==(const refresh_rate_s &other) const
    {
        return (this->internal_value() == other.internal_value());
    }

    bool operator==(const double hz) const
    {
        return (this->internal_value() == refresh_rate_s(hz).internal_value());
    }

    bool operator!=(const refresh_rate_s &other) const
    {
        return !(*this == other);
    }

    bool operator!=(const double hz) const
    {
        return !(*this == hz);
    }

private:
    // The refresh rate multiplied by 10^num_decimals.
    unsigned internalValue = 0;
};

#endif
