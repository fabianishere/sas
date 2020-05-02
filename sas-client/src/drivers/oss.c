#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/soundcard.h>
#include <fcntl.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>

#include <sas/chunk.h>
#include <sas/transport.h>
#include <sas/drivers/oss.h>

#define AUDIODEV	"/dev/dsp"

struct oss_sink_logic {
    struct rxc_sink_logic base;

    int fd, finished;
};

static void dealloc(struct rxc_sink_logic *logic) {
    struct oss_sink_logic *self = (void *) logic;

    if (self->fd >= 0 || self->finished) {
        close(self->fd);
    }

    free(self);
}

static void on_connect(struct rxc_sink_logic *self)
{
    rxc_inlet_pull(self->in, 1);
}

static int open_audio_fd(int sample_rate, int sample_size, int channels)
{
    /* Sets up the audio device params.
    * Returns device file descriptor if successful*/
    int audio_fd, error;
    char *devicename;

    printf("requested chans=%d, sample rate=%d sample size=%d\n",
           channels, sample_rate, sample_size);

    if (NULL == (devicename = getenv("AUDIODEV")))
        devicename = AUDIODEV;

    if ((audio_fd = open (devicename, O_WRONLY, 0)) < 0) {
        perror ("setparams : open ") ;
        return -1;
    }
    if (ioctl (audio_fd, SNDCTL_DSP_RESET, 0) != 0) {
        perror ("setparams : reset ") ;
        close(audio_fd);
        return -1;
    }

    if ((error = ioctl (audio_fd, SNDCTL_DSP_SAMPLESIZE, &sample_size)) != 0) {
        perror ("setparams : bitwidth ") ;
        close(audio_fd);
        return -1;
    }

    if (ioctl (audio_fd, SNDCTL_DSP_CHANNELS, &channels) != 0) {
        perror ("setparams : channels ") ;
        close(audio_fd);
        return -1;
    }

    if ((error = ioctl (audio_fd, SNDCTL_DSP_SPEED, &sample_rate)) != 0) {
        perror ("setparams : sample rate ") ;
        close(audio_fd);
        return -1;
    }

    if ((error = ioctl (audio_fd, SNDCTL_DSP_SYNC, 0)) != 0) {
        perror ("setparams : sync ") ;
        close(audio_fd);
        return -1;
    }
    printf("set chans=%d, sample rate=%d sample size=%d\n",
           channels,sample_rate, sample_size);
    return audio_fd;
}

static void on_push(struct rxc_sink_logic *logic, void *element)
{
    struct oss_sink_logic *self = (void *) logic;
    struct sas_chunk *chunk = (void *) element;

    if (self->fd <= 0 && !self->finished) {
        self->fd = open_audio_fd(chunk->sample_rate, chunk->sample_size, chunk->channels);
        rxc_inlet_pull(logic->in, 1);
        return;
    }

    ssize_t n = write(self->fd, chunk->buffer, chunk->size);

    sas_chunk_dealloc(chunk);

    if (n <= 0) {
        self->finished = 1;
        close(self->fd);
        rxc_inlet_cancel(logic->in);
        return;
    }

    rxc_inlet_pull(logic->in, 1);
}


static void on_upstream_finish(struct rxc_sink_logic *self)
{}

static void on_upstream_failure(struct rxc_sink_logic *self, void *failure)
{
    free(failure);
}

static struct rxc_sink_logic * create_logic(struct rxc_sink *self)
{
    struct oss_sink_logic *logic = malloc(sizeof(struct oss_sink_logic));

    if (!logic) {
        return NULL;
    }

    logic->fd = -1;
    logic->finished = 0;
    logic->base.sink = self;
    logic->base.dealloc = dealloc;
    logic->base.on_connect = on_connect;
    logic->base.on_push = on_push;
    logic->base.on_upstream_finish = on_upstream_finish;
    logic->base.on_upstream_failure = on_upstream_failure;

    return &logic->base;
}

static void sink_dealloc(struct rxc_sink *self, int shallow)
{
    free(self);
}

struct rxc_sink * sas_drivers_oss(void)
{
    struct rxc_sink *sink = malloc(sizeof(struct rxc_sink));

    if (!sink) {
        return NULL;
    }

    sink->dealloc = sink_dealloc;
    sink->create_logic = create_logic;

    return sink;
}



