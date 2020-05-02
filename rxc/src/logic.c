#include <stdlib.h>
#include <limits.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>

/* Inlet */
void rxc_inlet_pull(struct rxc_inlet *in, long n)
{
    in->pull(in, n);
}

void rxc_inlet_cancel(struct rxc_inlet *in)
{
    in->cancel(in);
}

/* Connection */
static void rxc_connection_inlet_pull(struct rxc_inlet *self, long n)
{
    struct rxc_connection *conn = (void *) self;

    /* Check whether the connection is still open */
    if (conn->requested == LONG_MIN) {
        return;
    }

    conn->requested += n;

    /* Cap the requested amount at the maximum size of a long */
    if (conn->requested < 0) {
        conn->requested = LONG_MAX;
    }

    conn->source_logic->on_pull(conn->source_logic, n);
}

static void rxc_connection_inlet_cancel(struct rxc_inlet *self)
{
    struct rxc_connection *conn = (void *) self;

    /* Check if it is already closed */
    if (conn->requested == LONG_MIN) {
        return;
    }

    conn->requested = LONG_MIN;
    conn->source_logic->on_downstream_finish(conn->source_logic);

    return;
}

void rxc_connection_create(struct rxc_source_logic *source,
                           struct rxc_sink_logic *sink)
{
    struct rxc_connection *conn = malloc(sizeof(struct rxc_connection));

    conn->in.pull = rxc_connection_inlet_pull;
    conn->in.cancel = rxc_connection_inlet_cancel;
    conn->requested = 0;
    conn->source_logic = source;
    conn->sink_logic = sink;

    source->out = (void *) conn;
    sink->in = (void *) conn;
}

int rxc_outlet_available(struct rxc_outlet *out)
{
    return out->conn.requested > 0;
}

int rxc_outlet_emit(struct rxc_outlet *out, void *element)
{
    struct rxc_connection *conn = &out->conn;

    /* Check whether the connection is still open */
    if (conn->requested <= 0) {
        return -1;
    }

    /* LONG_MAX means unbounded */
    if (conn->requested != LONG_MAX) {
        conn->requested--;
    }
    conn->sink_logic->on_push(conn->sink_logic, element);

    return RXC_EOK;
}

void rxc_outlet_fail(struct rxc_outlet *out, void *failure)
{
    struct rxc_connection *conn = &out->conn;

    /* Check if it is already closed */
    if (conn->requested == LONG_MIN) {
        return;
    }

    conn->requested = LONG_MIN;
    conn->sink_logic->on_upstream_failure(conn->sink_logic, failure);
}

void rxc_outlet_complete(struct rxc_outlet *out)
{
    struct rxc_connection *conn = &out->conn;

    /* Check if it is already closed */
    if (conn->requested == LONG_MIN) {
        return;
    }

    conn->requested = LONG_MIN;
    conn->sink_logic->on_upstream_finish(conn->sink_logic);
}
