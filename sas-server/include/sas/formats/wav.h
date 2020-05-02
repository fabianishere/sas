/*
 * part of the Systems Programming assignment
 * (c) Vrije Universiteit Amsterdam, 2005-2015. BSD License applies
 * author  : wdb -_at-_ few.vu.nl
 * contact : arno@cs.vu.nl
 */

#ifndef SAS_FORMATS_WAV_H
#define SAS_FORMATS_WAV_H

#include <rxc/rxc.h>

/**
 * The source structure for a Wav audio stream.
 */
struct sas_formats_wav_source {
    struct rxc_source base;

    int fd, finished;
    int32_t sample_rate;
    int8_t sample_size, channels;
};

/**
 * Create a Wave audio stream.
 *
 * Note that not all WAV filetypes are
 * supported. Only the simplest uncompressed PCM streams can be read.
 *
 * the function writes metadata about the opened file in the integers pointed
 * to by the parameters.
 *
 * @param fd The file descriptor to the WAV file to read.
 * @return <code>0</code> on success, otherwise an error code.
 */
struct rxc_source * sas_formats_wav(int fd);

#endif /* SAS_FORMATS_WAV_H */
