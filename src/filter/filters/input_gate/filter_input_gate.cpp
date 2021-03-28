/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/input_gate/filter_input_gate.h"

void filter_input_gate_c::apply(FILTER_FUNC_PARAMS) const
{
    /// Input gates do not modify pixel data.

    (void)pixels;
    (void)r;

    return;
}
