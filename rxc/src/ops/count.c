#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>
#include <rxc/ops/core.h>

struct rxc_source_count {
    struct rxc_source base;

    long from, to;
};

struct rxc_source_logic_count {
    struct rxc_source_logic base;

    long count;
};

static void on_pull(struct rxc_source_logic *logic, long n)
{
    struct rxc_source_logic_count *self = (struct rxc_source_logic_count *) logic;
    struct rxc_source_count *source = (struct rxc_source_count *) logic->source;

    for (; n > 0; n--) {
        if (self->count >= source->to) {
            rxc_outlet_complete(logic->out);
            return;
        }

        long *element = malloc(sizeof(long));
        *element = self->count++;

        rxc_outlet_emit(logic->out, element);
    }
}

static void on_downstream_finish(struct rxc_source_logic *self)
{}

static struct rxc_source_logic * create_logic(struct rxc_source *source)
{
    struct rxc_source_count *self = (struct rxc_source_count *) source;
    struct rxc_source_logic_count *logic = malloc(sizeof(struct rxc_source_logic_count));

    if (!logic) {
        return NULL;
    }

    logic->count = self->from;
    logic->base.source = source;
    logic->base.dealloc = (void (*)(struct rxc_source_logic *)) free;
    logic->base.on_pull = on_pull;
    logic->base.on_downstream_finish = on_downstream_finish;

    return &logic->base;
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

struct rxc_source * rxc_source_count(long from, long to)
{
    struct rxc_source_count *source = malloc(sizeof(struct rxc_source_count));

    if (!source) {
        return NULL;
    }

    source->from = from;
    source->to = to;

    source->base.dealloc = dealloc;
    source->base.connect = connect;

    return &source->base;
}
