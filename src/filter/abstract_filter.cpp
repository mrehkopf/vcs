/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS filter
 *
 * Manages the image filters that're available to the user to filter captured frames
 * with.
 *
 */

#include "filter/abstract_filter.h"

abstract_filter_c::abstract_filter_c(const std::vector<std::pair<unsigned, double>> &parameters,
                                     const std::vector<std::pair<unsigned, double>> &overrideParameterValues)
{
    this->parameterValues.resize(std::max(parameters.size(), overrideParameterValues.size()));

    for (const auto &parameter: parameters)
    {
        this->set_parameter(parameter.first, parameter.second);
    }

    for (const auto &parameter: overrideParameterValues)
    {
        this->set_parameter(parameter.first, parameter.second);
    }

    return;
}

abstract_filter_c::~abstract_filter_c(void)
{
    delete this->guiDescription;

    return;
}

const std::vector<filtergui_field_s>& abstract_filter_c::gui_description(void) const
{
    return this->guiDescription->guiFields;
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
void abstract_filter_c::set_parameters(const std::vector<std::pair<unsigned, double>> &parameters)
{
    this->parameterValues.resize(std::max(parameters.size(), this->parameterValues.size()));

    for (const auto &parameter: parameters)
    {
        this->set_parameter(parameter.first, parameter.second);
    }

    return;
}

std::vector<std::pair<unsigned, double>> abstract_filter_c::parameters(void) const
{
    auto params = std::vector<std::pair<unsigned, double>>{};

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
