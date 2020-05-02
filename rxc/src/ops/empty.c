#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>
#include <rxc/ops/core.h>

static void on_pull(struct rxc_source_logic *self, long n)
{}

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

static void connect(struct rxc_source *self, struct rxc_sink *sink)
{
    struct rxc_source_logic *source_logic = create_logic(self);
    struct rxc_sink_logic *sink_logic = sink->create_logic(sink);

    rxc_connection_create(source_logic, sink_logic);
    sink_logic->on_connect(sink_logic);
}

struct rxc_source * rxc_source_empty()
{
    struct rxc_source *source = malloc(sizeof(struct rxc_source));

    if (!source) {
        return NULL;
    }

    source->dealloc = dealloc;
    source->connect = connect;

    return source;
}
