/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/output_gate/filter_output_gate.h"

void filter_output_gate_c::apply(u8 *const pixels, const resolution_s &r)
{
    /// Output gates do not modify pixel data.

    (void)pixels;
    (void)r;

    return;
}
