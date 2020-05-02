#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>
#include <rxc/ops/core.h>

struct rxc_sink_foreach {
    struct rxc_sink base;

    void (*callback)(void *element);
};

static void dealloc(struct rxc_sink *self, int shallow)
{
    free(self);
}

static void on_connect(struct rxc_sink_logic *self)
{
    rxc_inlet_pull(self->in, 1);
}

static void on_push(struct rxc_sink_logic *self, void *element)
{
    struct rxc_sink_foreach *sink = (struct rxc_sink_foreach *) self->sink;
    sink->callback(element);
    rxc_inlet_pull(self->in, 1);
}


static void on_upstream_finish(struct rxc_sink_logic *self)
{}

static void on_upstream_failure(struct rxc_sink_logic *self, void *failure)
{
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

struct rxc_sink * rxc_sink_foreach(void (*callback)(void *element))
{
    struct rxc_sink_foreach *sink = malloc(sizeof(struct rxc_sink_foreach));

    if (!sink) {
        return NULL;
    }

    sink->callback = callback;
    sink->base.dealloc = dealloc;
    sink->base.create_logic = create_logic;

    return &sink->base;
}
