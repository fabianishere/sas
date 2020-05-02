#include <stdlib.h>
#include <string.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>

/* Stage operations */
void rxc_source_dealloc(struct rxc_source *source)
{
    source->dealloc(source, 1);
}

struct rxc_pipeline * rxc_source_to(struct rxc_source *source,
                                    struct rxc_sink *sink)
{
    struct rxc_pipeline *pipeline = malloc(sizeof(struct rxc_pipeline));

    if (!pipeline) {
        return NULL;
    }

    pipeline->source = source;
    pipeline->sink = sink;

    return pipeline;
}

struct rxc_source * rxc_source_via(struct rxc_source *source,
                                   struct rxc_flow *flow)
{
    return flow->connect_source(flow, source);
}

void rxc_sink_dealloc(struct rxc_sink *sink)
{
    sink->dealloc(sink, 1);
}

struct rxc_flow * rxc_flow_via(struct rxc_flow *a, struct rxc_flow *b)
{
    return NULL;
}

struct rxc_sink * rxc_flow_to(struct rxc_flow *flow, struct rxc_sink *sink)
{
    return flow->connect_sink(flow, sink);
}

/* Pipeline operations */
void rxc_pipeline_dealloc(struct rxc_pipeline *pipeline)
{
    struct rxc_source *source = pipeline->source;
    struct rxc_sink *sink = pipeline->sink;

    rxc_source_dealloc(source);
    rxc_sink_dealloc(sink);

    free(pipeline);
}

struct rxc_sink_scheduler {
    struct rxc_sink base;
    struct rxc_sink *inner;
    struct rxc_scheduler *scheduler;
};

struct rxc_sink_logic_scheduler {
    struct rxc_sink_logic base;
    struct rxc_sink_logic *inner;
    struct rxc_scheduler_worker *worker;
};

struct rxc_inlet_scheduler {
    struct rxc_inlet base;
    struct rxc_inlet *parent;
    struct rxc_scheduler_worker *worker;
};

struct rxc_request {
    struct rxc_inlet *in;
    long n;
};

static void pipeline_executor(void *ctx)
{
    struct rxc_request *req = ctx;
    req->in->pull(req->in, req->n);
    free(ctx);
}

static void pipeline_inlet_pull(struct rxc_inlet *self, long n)
{
    struct rxc_inlet *parent = ((struct rxc_inlet_scheduler *) self)->parent;

    if (parent == NULL) {
        return;
    }

    struct rxc_scheduler_worker *worker = ((struct rxc_inlet_scheduler *) self)->worker;

    struct rxc_request *req = malloc(sizeof(struct rxc_request));
    req->in = parent;
    req->n = n;

    worker->schedule(worker, pipeline_executor, req);
}

static void pipeline_inlet_cancel(struct rxc_inlet *in)
{
    struct rxc_inlet_scheduler *self = (struct rxc_inlet_scheduler *) in;
    struct rxc_inlet *parent = self->parent;

    self->parent = NULL;
    parent->cancel(parent);
}

static struct rxc_inlet * pipeline_inlet_create(struct rxc_inlet *parent,
                                                struct rxc_scheduler_worker *worker)
{
    struct rxc_inlet_scheduler *in = malloc(sizeof(struct rxc_inlet_scheduler));

    in->parent = parent;
    in->worker = worker;
    in->base.pull = pipeline_inlet_pull;
    in->base.cancel = pipeline_inlet_cancel;

    return &in->base;
}

static void pipeline_sink_logic_dealloc(struct rxc_sink_logic *logic)
{
    struct rxc_sink_logic_scheduler *self = (struct rxc_sink_logic_scheduler *) logic;
    self->inner->dealloc(self->inner);
    self->worker->dealloc(self->worker);
    free(self);
}

static void pipeline_sink_logic_on_connect(struct rxc_sink_logic *logic)
{
    struct rxc_sink_logic_scheduler *self = (struct rxc_sink_logic_scheduler *) logic;
    struct rxc_inlet *in = pipeline_inlet_create(logic->in, self->worker);
    self->inner->in = in;
    self->inner->on_connect(self->inner);
}

static void pipeline_sink_logic_on_push(struct rxc_sink_logic *self, void *element)
{
    struct rxc_sink_logic *logic = ((struct rxc_sink_logic_scheduler *) self)->inner;
    logic->on_push(logic, element);
}


static void pipeline_sink_logic_on_upstream_finish(struct rxc_sink_logic *self)
{
    struct rxc_sink_logic *logic = ((struct rxc_sink_logic_scheduler *) self)->inner;
    logic->on_upstream_finish(logic);
}

static void pipeline_sink_logic_on_upstream_failure(struct rxc_sink_logic *self,
                                              void *failure)
{
    struct rxc_sink_logic *logic = ((struct rxc_sink_logic_scheduler *) self)->inner;
    logic->on_upstream_failure(logic, failure);
}

static struct rxc_sink_logic * pipeline_sink_create_logic(struct rxc_sink *sink)
{
    struct rxc_sink_scheduler *self = (struct rxc_sink_scheduler *) sink;
    struct rxc_sink_logic_scheduler *logic = malloc(sizeof(struct rxc_sink_logic_scheduler));

    if (!logic) {
        return NULL;
    }

    logic->inner = self->inner->create_logic(self->inner);
    logic->worker = self->scheduler->create_worker(self->scheduler);
    logic->base.sink = sink;
    logic->base.dealloc = pipeline_sink_logic_dealloc;
    logic->base.on_connect = pipeline_sink_logic_on_connect;
    logic->base.on_push = pipeline_sink_logic_on_push;
    logic->base.on_upstream_finish = pipeline_sink_logic_on_upstream_finish;
    logic->base.on_upstream_failure = pipeline_sink_logic_on_upstream_failure;

    return &logic->base;
}

static void pipeline_sink_dealloc(struct rxc_sink *sink, int shallow)
{
    struct rxc_sink_scheduler *self = (struct rxc_sink_scheduler *) sink;

    if (!shallow) {
        self->inner->dealloc(self->inner, shallow);
        self->scheduler->dealloc(self->scheduler);
    }

    free(self);
}

static struct rxc_sink * pipeline_sink(struct rxc_sink *sink, struct rxc_scheduler *scheduler)
{
    struct rxc_sink_scheduler *wrapper_sink = malloc(sizeof(struct rxc_sink_scheduler));

    if (!wrapper_sink) {
        return NULL;
    }

    wrapper_sink->inner = sink;
    wrapper_sink->scheduler = scheduler;
    wrapper_sink->base.dealloc = pipeline_sink_dealloc;
    wrapper_sink->base.create_logic = pipeline_sink_create_logic;

    return &wrapper_sink->base;
}

int rxc_pipeline_start(struct rxc_pipeline *pipeline,
                       struct rxc_scheduler *scheduler)
{
    struct rxc_source *source = pipeline->source;
    struct rxc_sink *sink = pipeline_sink(pipeline->sink, scheduler);

    source->connect(source, sink);

    return RXC_EOK;
}
