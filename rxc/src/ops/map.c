#include <stdlib.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>
#include <rxc/ops/core.h>

struct rxc_sink_map {
    struct rxc_sink base;
    struct rxc_sink *inner;

    void * (*mapping)(void *element);
};

struct rxc_sink_logic_map {
    struct rxc_sink_logic base;
    struct rxc_sink_logic *inner;
};

static void sink_logic_dealloc(struct rxc_sink_logic *logic)
{
    struct rxc_sink_logic_map *self = (struct rxc_sink_logic_map *) logic;

    self->inner->dealloc(self->inner);
    free(self);
}

static void sink_logic_on_connect(struct rxc_sink_logic *self)
{
    struct rxc_sink_logic *inner = ((struct rxc_sink_logic_map *) self)->inner;
    inner->in = self->in;
    inner->on_connect(inner);
}

static void sink_logic_on_push(struct rxc_sink_logic *logic, void *element)
{
    struct rxc_sink_logic_map *self = (struct rxc_sink_logic_map *) logic;
    struct rxc_sink_logic *inner = self->inner;

    inner->on_push(inner, ((struct rxc_sink_map *) self->base.sink)->mapping(element));
}


static void sink_logic_on_upstream_finish(struct rxc_sink_logic *self)
{
    struct rxc_sink_logic *inner = ((struct rxc_sink_logic_map *) self)->inner;
    inner->on_upstream_finish(inner);
}

static void sink_logic_on_upstream_failure(struct rxc_sink_logic *self, void *failure)
{
    struct rxc_sink_logic *inner = ((struct rxc_sink_logic_map *) self->sink)->inner;
    inner->on_upstream_failure(inner, failure);
}

static struct rxc_sink_logic * sink_create_logic(struct rxc_sink *sink)
{
    struct rxc_sink_map *self = (struct rxc_sink_map *) sink;
    struct rxc_sink_logic_map *logic = malloc(sizeof(struct rxc_sink_logic_map));

    if (!logic) {
        return NULL;
    }

    logic->inner = self->inner->create_logic(self->inner);
    logic->base.sink = sink;
    logic->base.dealloc = sink_logic_dealloc;
    logic->base.on_connect = sink_logic_on_connect;
    logic->base.on_push = sink_logic_on_push;
    logic->base.on_upstream_finish = sink_logic_on_upstream_finish;
    logic->base.on_upstream_failure = sink_logic_on_upstream_failure;

    return &logic->base;
}

static void sink_dealloc(struct rxc_sink *sink, int shallow)
{
    struct rxc_sink_map *self = ((struct rxc_sink_map *) sink);

    if (!shallow) {
        self->inner->dealloc(self->inner, shallow);
    }

    free(self);
}

struct map_ctx {
    void * (*mapping)(void *element);
};

static struct rxc_sink * sink_wrap(struct rxc_sink *sink, void *ctx)
{
    struct rxc_sink_map *wrapper_sink = malloc(sizeof(struct rxc_sink_map));

    if (!wrapper_sink) {
        return NULL;
    }

    wrapper_sink->inner = sink;
    wrapper_sink->mapping = ((struct map_ctx *) ctx)->mapping;
    wrapper_sink->base.dealloc = sink_dealloc;
    wrapper_sink->base.create_logic = sink_create_logic;

    return &wrapper_sink->base;
}

struct rxc_flow * rxc_flow_map(void * (*mapping)(void *))
{
    struct map_ctx *ctx = malloc(sizeof(struct map_ctx));
    ctx->mapping = mapping;
    return rxc_flow_wrapper(sink_wrap, ctx);
}
