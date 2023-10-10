/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter
 *
 * Manages the image filters that're available to the user to filter captured frames
 * with.
 *
 */

#include "filter/abstract_filter.h"
#include "common/globals.h"

abstract_filter_c::abstract_filter_c(
    const filter_params_t &parameters,
    const filter_params_t &overrideParamValues
){
    this->parameterValues.resize(std::max(parameters.size(), overrideParamValues.size()));

    for (const auto &parameter: parameters)
    {
        this->set_parameter(parameter.first, parameter.second);
    }

    for (const auto &parameter: overrideParamValues)
    {
        this->set_parameter(parameter.first, parameter.second);
    }

    return;
}

abstract_filter_c::~abstract_filter_c(void)
{
    delete this->gui;

    return;
}

unsigned abstract_filter_c::num_parameters(void) const
{
    return this->parameterValues.size();
}

double abstract_filter_c::parameter(const unsigned offset) const
{
    return this->parameterValues.at(offset);
}

void abstract_filter_c::set_parameter(const unsigned offset, const double value)
{
    if (offset < this->parameterValues.size())
    {
        this->parameterValues.at(offset) = value;
    }

    return;
}

void abstract_filter_c::set_parameter_string(const unsigned offset, const std::string &string, const std::size_t maxLength)
{
    std::size_t i = 0;

    for (i = 0; i < std::min(maxLength, string.length()); i++)
    {
        this->set_parameter((i + offset), string.at(i));
    }

    // Null terminator.
    this->set_parameter((i + offset), 0);

    return;
}

void abstract_filter_c::set_parameters(const filter_params_t &parameters)
{
    this->parameterValues.resize(std::max(parameters.size(), this->parameterValues.size()));

    for (const auto &parameter: parameters)
    {
        this->set_parameter(parameter.first, parameter.second);
    }

    return;
}

filter_params_t abstract_filter_c::parameters(void) const
{
    auto params = filter_params_t{};

    for (std::size_t i = 0; i < this->parameterValues.size(); i++)
    {
        params.push_back({i, this->parameterValues.at(i)});
    }

    return params;
}

std::vector<char> abstract_filter_c::parameter_data_block(const std::size_t startIdx, const std::size_t count) const
{
    auto values = std::vector<char>{};

    for (std::size_t i = std::max(std::size_t(0), startIdx); i < std::min(count, this->parameterValues.size()); i++)
    {
        values.push_back(this->parameterValues.at(i));
    }

    return values;
}


std::string abstract_filter_c::string_parameter(std::size_t offset) const
{
    std::string string = "";

    // Copy characters until we reach a null terminator.
    while (this->parameterValues.at(offset))
    {
        string += char(this->parameterValues.at(offset++));
    }

    return string;
}
