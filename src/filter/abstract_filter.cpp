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
    this->parameterValues.resize(parameters.size());

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

void abstract_filter_c::set_parameters(const std::vector<std::pair<unsigned, double>> &parameters)
{
    for (const auto &parameter: parameters)
    {
        this->set_parameter(parameter.first, parameter.second);
    }

    return;
}

std::vector<std::pair<unsigned, double>> abstract_filter_c::parameters(void) const
{
    auto params = std::vector<std::pair<unsigned, double>>{};

    for (unsigned i = 0; i < this->parameterValues.size(); i++)
    {
        params.push_back({i, this->parameterValues[i]});
    }

    return params;
}
