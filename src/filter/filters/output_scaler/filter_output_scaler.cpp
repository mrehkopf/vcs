/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/output_scaler/filter_output_scaler.h"
#include "scaler/scaler.h"

void filter_output_scaler_c::apply(u8 *const pixels, const resolution_s &r)
{
    assert_input_validity(pixels, r);

    const unsigned width = this->parameter(filter_output_scaler_c::PARAM_WIDTH);
    const unsigned height = this->parameter(filter_output_scaler_c::PARAM_HEIGHT);

    const image_scaler_s *const scaler = ks_scaler_for_name_string("Linear");
    scaler->apply(pixels, r, {width, height, 32});

    return;
}
