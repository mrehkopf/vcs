/*
 * 2018 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Handles storing and doling out capture video signal parameters (like phase,
 * horizontal position, color balance, etc.).
 * 
 * The entirety of parameters for a given reolution is called a parameter set.
 * 
 * 
 * Usage:
 * 
 *   1. Call kvideoparam_assign_parameter_sets() to establish a list of video
 *      parameters per capture resolution.
 * 
 *   2. Call kvideoparam_parameters_for_resolution() to get the parameters for
 *      a particular resolution, or kvideoparam_all_parameter_sets() for all
 *      resolutions. The parameters will be picked from among the sets passed
 *      to kvideoparam_assign_parameter_sets().
 * 
 *   3. Call kvideoparam_update_parameters_for_resolution() to update a given
 *      parameter set.
 *
 */


#ifndef VIDEO_SIGNAL_PARAMETERS_H
#define VIDEO_SIGNAL_PARAMETERS_H

#include "common/globals.h"
#include "capture/capture.h"

video_signal_parameters_s kvideoparam_parameters_for_resolution(const resolution_s r);

void kvideoparam_assign_parameter_sets(const std::vector<video_signal_parameters_s> &modeParams);

const std::vector<video_signal_parameters_s>& kvideoparam_all_parameter_sets(void);

void kvideoparam_update_parameters_for_resolution(const resolution_s r, const video_signal_parameters_s &newParams);

#endif
