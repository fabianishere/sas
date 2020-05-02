#ifndef SAS_CHUNK_H
#define SAS_CHUNK_H

#include <stdlib.h>
#include <stdint.h>

/**
 * A chunk of audio data with using the specified configuration.
 */
struct sas_chunk {
    /**
     * Deallocate this chunk.
     */
    void (*dealloc)(struct sas_chunk *chunk);

    /**
     * The size of the chunk.
     */
    size_t size;

    /**
     * The sample rate that is used.
     */
    int32_t sample_rate;

    /**
     * The sample size that is used.
     */
    int8_t sample_size;

    /**
     * The channels used.
     */
    int8_t channels;

    /**
     * The codec that is used.
     */
    uint8_t codec;

    /**
     * The buffer of this chunk.
     */
    uint8_t *buffer;
};

/**
 * Deallocate the specified chunk.
 *
 * @param[in] chunk The chunk to deallocate.
 */
void sas_chunk_dealloc(struct sas_chunk *chunk);

#endif /* SAS_CHUNK_H */
