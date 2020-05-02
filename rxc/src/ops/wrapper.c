#include <stdlib.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>
#include <rxc/ops/core.h>

struct rxc_flow_wrapper {
    struct rxc_flow base;
    void *ctx;
    struct rxc_sink * (*wrap)(struct rxc_sink *, void *);
};

static struct rxc_sink * flow_connect_sink(struct rxc_flow *flow,
                                           struct rxc_sink *sink)
{
    struct rxc_flow_wrapper *self = (struct rxc_flow_wrapper *) flow;
    return self->wrap(sink, self->ctx);
}

struct rxc_source_wrapper {
    struct rxc_source base;
    struct rxc_source *inner;
    struct rxc_flow_wrapper *flow;
};

static void source_dealloc(struct rxc_source *self, int shallow)
{
    struct rxc_flow_wrapper *flow = ((struct rxc_source_wrapper *) self)->flow;

    if (!shallow) {
        flow->base.dealloc(&flow->base, shallow);
    }

    free(self);
}

static void source_connect(struct rxc_source *self, struct rxc_sink *sink)
{
    struct rxc_flow_wrapper *flow = ((struct rxc_source_wrapper *) self)->flow;
    struct rxc_source *inner = ((struct rxc_source_wrapper *) self)->inner;
    struct rxc_sink *wrapper_sink = flow_connect_sink(&flow->base, sink);

    inner->connect(inner, wrapper_sink);
}

static struct rxc_source * flow_connect_source(struct rxc_flow *self,
                                               struct rxc_source *source)
{
    struct rxc_source_wrapper *wrapper = malloc(sizeof(struct rxc_source_wrapper));

    if (!wrapper) {
        return NULL;
    }

    wrapper->flow = (struct rxc_flow_wrapper *) self;
    wrapper->inner = source;

    wrapper->base.dealloc = source_dealloc;
    wrapper->base.connect = source_connect;

    return &wrapper->base;
}

static void flow_dealloc(struct rxc_flow *flow, int shallow)
{
    struct rxc_flow_wrapper *self = (struct rxc_flow_wrapper *) flow;
    free(self->ctx);
    free(self);
}

struct rxc_flow * rxc_flow_wrapper(struct rxc_sink * (*wrap)(struct rxc_sink *, void *),
                                   void *ctx)
{
    struct rxc_flow_wrapper *flow = malloc(sizeof(struct rxc_flow_wrapper));

    flow->ctx = ctx;
    flow->wrap = wrap;
    flow->base.dealloc = flow_dealloc;
    flow->base.connect_source = flow_connect_source;
    flow->base.connect_sink = flow_connect_sink;

    return &flow->base;
}
