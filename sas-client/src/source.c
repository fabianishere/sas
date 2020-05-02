#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>

#include <sas/transport.h>

#include "source.h"
#include "client.h"

static void on_pull(struct rxc_source_logic *self, long n)
{
    struct sas_client *client = ((struct sas_client_session_source *) self->source)->client;

    client->session.receive_window = SAS_CLIENT_SESSION_WINDOW_SIZE;
    sas_client_session_ack(client);
}

static void on_downstream_finish(struct rxc_source_logic *self)
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

static void dealloc(struct rxc_source *self, int shallow)
{
    free(self);
}

static void connect_sink(struct rxc_source *self, struct rxc_sink *sink)
{
    struct sas_client *client = ((struct sas_client_session_source *) self)->client;
    struct rxc_source_logic *source_logic = create_logic(self);
    struct rxc_sink_logic *sink_logic = sink->create_logic(sink);

    rxc_connection_create(source_logic, sink_logic);
    sink_logic->on_connect(sink_logic);

    client->session.sink_logic = sink_logic;
}

struct rxc_source * sas_client_session_source(struct sas_client *client)
{
    struct sas_client_session_source *source = malloc(
            sizeof(struct sas_client_session_source));

    if (!source) {
        return NULL;
    }

    source->client = client;
    source->base.dealloc = dealloc;
    source->base.connect = connect_sink;

    return &source->base;
}
