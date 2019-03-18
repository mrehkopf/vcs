/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef RECORD_H
#define RECORD_H

bool krecord_start_recording(const char *const filename, const uint width, const uint height, const uint frameRate);

bool krecord_is_recording(void);

void krecord_record_new_frame(void);

void krecord_stop_recording(void);

#endif
