/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTERS_FILTERS_H
#define VCS_FILTER_FILTERS_FILTERS_H

#include "filter/filters/blur/filter_blur.h"
#include "filter/filters/crop/filter_crop.h"
#include "filter/filters/decimate/filter_decimate.h"
#include "filter/filters/denoise_nonlocal_means/filter_denoise_nonlocal_means.h"
#include "filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h"
#include "filter/filters/flip/filter_flip.h"
#include "filter/filters/source_fps_estimate/filter_source_fps_estimate.h"
#include "filter/filters/median/filter_median.h"
#include "filter/filters/rotate/filter_rotate.h"
#include "filter/filters/output_scaler/filter_output_scaler.h"
#include "filter/filters/render_text/filter_render_text.h"
#include "filter/filters/sharpen/filter_sharpen.h"
#include "filter/filters/unsharp_mask/filter_unsharp_mask.h"
#include "filter/filters/kernel_3x3/filter_kernel_3x3.h"
#include "filter/filters/anti_tear/filter_anti_tear.h"
#include "filter/filters/color_depth/filter_color_depth.h"
#include "filter/filters/input_gate/filter_input_gate.h"
#include "filter/filters/output_gate/filter_output_gate.h"
#include "filter/filters/line_copy/filter_line_copy.h"
#include "filter/filters/unknown/filter_unknown.h"
#include "filter/filters/crt/filter_crt.h"

#endif
