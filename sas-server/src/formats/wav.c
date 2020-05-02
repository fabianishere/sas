#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>

#include <sas/chunk.h>
#include <sas/formats/wav.h>

#define swap_short(x)		(x)
#define swap_long(x)		(x)

/* Definitions for the WAVE format */

#define RIFF		"RIFF"
#define WAVEFMT		"WAVEfmt"
#define DATA		0x61746164
#define PCM_CODE	1

typedef struct _waveheader {
    char		main_chunk[4];	/* 'RIFF' */
    uint32_t	length;		/* filelen */
    char		chunk_type[7];	/* 'WAVEfmt' */
    uint32_t	sc_len;		/* length of sub_chunk, =16 */
    uint16_t	format;		/* should be 1 for PCM-code */
    uint16_t	chans;		/* 1 Mono, 2 Stereo */
    uint32_t	sample_fq;	/* frequence of sample */
    uint32_t	byte_p_sec;
    uint16_t	byte_p_spl;	/* samplesize; 1 or 2 bytes */
    uint16_t	bit_p_spl;	/* 8, 12 or 16 bit */

    uint32_t	data_chunk;	/* 'data' */
    uint32_t	data_length;	/* samplecount */
} WaveHeader;

static void chunk_dealloc(struct sas_chunk *chunk)
{
    free(chunk->buffer);
    free(chunk);
}

static void on_pull(struct rxc_source_logic *self, long n)
{
    struct sas_formats_wav_source *source = (void *) self->source;

    if (source->finished) {
        rxc_outlet_complete(self->out);
        return;
    }

    for (; n > 0; n--) {
        size_t buffer_size = 1024;
        struct sas_chunk *chunk = malloc(sizeof(struct sas_chunk));
        uint8_t *buffer = malloc(buffer_size);

        ssize_t nread = read(source->fd, buffer, buffer_size);

        chunk->dealloc = chunk_dealloc;
        chunk->buffer = buffer;
        chunk->size = nread;
        chunk->sample_rate = source->sample_rate;
        chunk->sample_size = source->sample_size;
        chunk->channels = source->channels;
        chunk->codec = 0;

        if (nread == 0) {
            source->finished = 1;
            close(source->fd);
            chunk_dealloc(chunk);
            rxc_outlet_complete(self->out);
            return;
        } else if (nread < 0) {
            rxc_outlet_fail(self->out, NULL);

            source->finished = 1;
            close(source->fd);

            chunk_dealloc(chunk);
            return;
        }

        rxc_outlet_emit(self->out, chunk);
    }
}

static void on_downstream_finish(struct rxc_source_logic *logic)
{}

static struct rxc_source_logic * create_logic(struct rxc_source *source)
{
    struct rxc_source_logic *logic = malloc(sizeof(struct rxc_source_logic));

    if (!logic) {
        return NULL;
    }

    logic->source = source;
    logic->dealloc = (void (*)(struct rxc_source_logic *)) free;
    logic->on_pull = on_pull;
    logic->on_downstream_finish = on_downstream_finish;

    return logic;
}

static void dealloc(struct rxc_source *source, int shallow)
{
    struct sas_formats_wav_source *self = (void *) source;

    if (self->fd >= 0) {
        close(self->fd);
    }

    free(self);
}

static void connect(struct rxc_source *self, struct rxc_sink *sink)
{
    struct rxc_source_logic *source_logic = create_logic(self);
    struct rxc_sink_logic *sink_logic = sink->create_logic(sink);

    rxc_connection_create(source_logic, sink_logic);
    sink_logic->on_connect(sink_logic);
}

struct rxc_source * sas_formats_wav(int fd)
{
    /* Sets up a descriptor to read from a wave (RIFF).
     * Returns file descriptor if successful*/
    WaveHeader wh;

    read (fd, &wh, sizeof(wh));

    if (0 != bcmp(wh.main_chunk, RIFF, sizeof(wh.main_chunk)) ||
        0 != bcmp(wh.chunk_type, WAVEFMT, sizeof(wh.chunk_type)) ) {
        fprintf(stderr, "not a WAVE-file\n");
        return NULL;
    }
    if (swap_short(wh.format) != PCM_CODE) {
        fprintf(stderr, "can't play non PCM WAVE-files\n");
        return NULL;
    }
    if (swap_short(wh.chans) > 2) {
        fprintf(stderr, "can't play WAVE-files with %d tracks\n", wh.chans);
        return NULL;
    }

    struct sas_formats_wav_source *source = malloc(sizeof(struct sas_formats_wav_source));

    if (!source) {
        return NULL;
    }

    source->finished = 0;
    source->fd = fd;
    source->sample_rate = (unsigned int) swap_long(wh.sample_fq);
    source->sample_size = (unsigned int) swap_short(wh.bit_p_spl);
    source->channels =  (unsigned int) swap_short(wh.chans);
    source->base.dealloc = dealloc;
    source->base.connect = connect;

    fprintf (stderr, "chan=%u, freq=%u bitrate=%u format=%hu\n",
             source->channels, source->sample_rate, source->sample_size, wh.format);

    return &source->base;
}

