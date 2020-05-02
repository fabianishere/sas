#ifndef SAS_CODEC_H
#define SAS_CODEC_H

#include <rxc/rxc.h>

/**
 * A codec allows special encoding and decoding of the audio stream.
 */
struct sas_codec {
    /**
     * Deallocate the specified codec.
     *
     * @param[in] codec The codec to deallocate.
     */
    void (*dealloc)(struct sas_codec *);

    /**
     * Initialize the specified codec.
     *
     * @param[in] codec The codec to initialize.
     */
    void (*init)(struct sas_codec *);

    /**
     * The encoder that encodes messages to a specific codec.
     */
    struct rxc_flow *encoder;

    /**
     * The decoder that decodes messages into a raw PCM stream.
     */
    struct rxc_flow *decoder;
};

/**
 * This is an empty codec.
 */
extern struct sas_codec sas_codec_none;

#endif /* SAS_CODEC_H */
