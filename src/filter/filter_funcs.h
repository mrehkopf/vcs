/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_FILTER_FUNCS_H
#define VCS_FILTER_FILTER_FUNCS_H

#include "filter/filter.h"

// The parameters that each filter function must accept.
#define FILTER_FUNC_PARAMS u8 *const pixels /*32-bit BGRA*/, const resolution_s *const r, const u8 *const params/*filter parameters, like a blur's radius*/

void filter_func_blur(FILTER_FUNC_PARAMS);
void filter_func_unique_count(FILTER_FUNC_PARAMS);
void filter_func_unsharp_mask(FILTER_FUNC_PARAMS);
void filter_func_delta_histogram(FILTER_FUNC_PARAMS);
void filter_func_decimate(FILTER_FUNC_PARAMS);
void filter_func_denoise_temporal(FILTER_FUNC_PARAMS);
void filter_func_denoise_nonlocal_means(FILTER_FUNC_PARAMS);
void filter_func_sharpen(FILTER_FUNC_PARAMS);
void filter_func_median(FILTER_FUNC_PARAMS);
void filter_func_crop(FILTER_FUNC_PARAMS);
void filter_func_flip(FILTER_FUNC_PARAMS);
void filter_func_rotate(FILTER_FUNC_PARAMS);

#endif
