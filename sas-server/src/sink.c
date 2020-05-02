#include <stdlib.h>
#include <errno.h>

#include <sas/chunk.h>

#include "sink.h"
#include "session.h"

static void dealloc(struct rxc_sink *self, int shallow)
{
    free(self);
}

static void on_connect(struct rxc_sink_logic *self)
{
    struct sas_server_session_sink *sink = (void *) self->sink;
    sink->session->sink_logic = self;
}

static void on_push(struct rxc_sink_logic *self, void *element)
{
    struct sas_server_session_sink *sink = (void *) self->sink;
    struct sas_chunk *chunk = element;
    sas_server_session_send_chunk(sink->server, sink->session, chunk);
    sas_chunk_dealloc(chunk);

}

static void on_upstream_finish(struct rxc_sink_logic *self)
{
    struct sas_server_session_sink *sink = (void *) self->sink;
    /* Reset the session and mark as finished */
    sas_server_session_reset(sink->server, sink->session, 1);
}

static void on_upstream_failure(struct rxc_sink_logic *self, void *failure)
{
    struct sas_server_session_sink *sink = (void *) self->sink;
    sas_server_session_error(sink->server, sink->session, errno);
    free(failure);
}

static struct rxc_sink_logic * create_logic(struct rxc_sink *self)
{
    struct rxc_sink_logic *logic = malloc(sizeof(struct rxc_sink_logic));

    if (!logic) {
        return NULL;
    }

    logic->sink = self;
    logic->dealloc = (void (*)(struct rxc_sink_logic *)) free;
    logic->on_connect = on_connect;
    logic->on_push = on_push;
    logic->on_upstream_finish = on_upstream_finish;
    logic->on_upstream_failure = on_upstream_failure;

    return logic;
}

struct rxc_sink * sas_server_session_sink(struct sas_server *server,
                                          struct sas_server_session *session)
{
    struct sas_server_session_sink *sink = malloc(sizeof(struct sas_server_session_sink));

    if (!sink) {
        return NULL;
    }

    sink->server = server;
    sink->session = session;
    sink->base.dealloc = dealloc;
    sink->base.create_logic = create_logic;

    return &sink->base;
}
