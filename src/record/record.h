/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef RECORD_H
#define RECORD_H

namespace cv
{
    class Mat;
}

void krecord_initialize_recording(void);

bool krecord_is_recording(void);

void krecord_record_new_frame(void);

void krecord_finalize_recording(void);

#endif
