#ifndef RXC_LOGIC_H
#define RXC_LOGIC_H

#include <rxc/rxc.h>

/**
 * An inlet represents a one-to-one lifecycle of a sink connecting to a source
 * from the sink's viewpoint and allows the sink to control the stream.
 */
struct rxc_inlet {
    /**
     * Pull a specified amount of elements from the specified source.
     *
     * @param[in] self The inlet to pull from.
     * @param[in] n The amount of elements to pull.
     */
    void (*pull)(struct rxc_inlet *self, long n);

    /**
     * Request to stop receiving elements from the specified source.
     *
     * Data may still be sent to meet previously signalled demand after calling
     * cancel.
     *
     * @param[in] self The inlet to stop receiving elements from.
     */
    void (*cancel)(struct rxc_inlet *self);
};

/**
 * Pull a specified amount of elements from the specified source.
 *
 * @param[in] in The inlet to pull from.
 * @param[in] n The amount of elements to pull.
 */
void rxc_inlet_pull(struct rxc_inlet *in, long n);

/**
 * Request to stop receiving elements from the specified source.
 *
 * @param[in] in The inlet to stop receiving elements from.
 */
void rxc_inlet_cancel(struct rxc_inlet *in);

/**
 * An interface for handling events on an input port.
 */
struct rxc_sink_logic {
    /**
     * The sink of this handler.
     */
    struct rxc_sink *sink;

    /**
     * The inlet through which the elements enter the sink.
     */
    struct rxc_inlet *in;

    /**
     * Deallocate this handler.
     *
     * @param[in] self The reference to this handler.
     */
    void (*dealloc)(struct rxc_sink_logic *self);

    /**
     * This function is invoked when the sink has been connected to a source.
     *
     * @param[in] self The reference to this handler.
     */
    void (*on_connect)(struct rxc_sink_logic *self);

    /**
     * This function is invoked when a new element is available from the source.
     *
     * @param[in] self The reference to this handler.
     * @param[in] element The element that has been pushed.
     */
    void (*on_push)(struct rxc_sink_logic *self, void *element);

    /**
     * This function is invoked when the source has finished.
     *
     * @param[in] self The reference to this handler.
     */
    void (*on_upstream_finish)(struct rxc_sink_logic *self);

    /**
     * This function is invoked when the source failed due to some error.
     *
     * @param[in] self The reference to this handler.
     * @param[in] failure The reference to the failure object.
     */
    void (*on_upstream_failure)(struct rxc_sink_logic *self, void *failure);
};

/**
 * A helper structure that manages the stream control for a source and exposes
 * a simple outlet interface to communicate with the sink.
 */
struct rxc_connection {
    /**
     * The inlet this connection exposes.
     */
    struct rxc_inlet in;

    /**
     * The amount elements that have been requested over this connection.
     */
    long requested;

    /**
     * The source logic of this connection.
     */
    struct rxc_source_logic *source_logic;

    /**
     * The sink logic of this connection.
     */
    struct rxc_sink_logic *sink_logic;
};

/**
 * Create a connection between the specified source and sink logic to manage the
 * lifecycle of the two objects.
 *
 * @param[in] source The source to connect.
 * @param[in] sink The sink to connect.
 */
void rxc_connection_create(struct rxc_source_logic *source,
                           struct rxc_sink_logic *sink);

/**
 * An outlet represents a one-to-one lifecycle of a sink connecting to a source
 * from the source's viewpoint and allows the source to emit elements.
 */
struct rxc_outlet { struct rxc_connection conn; };

/**
 * An interface for handling events on the output port.
 */
struct rxc_source_logic {
    /**
     * The source of this handler.
     */
    struct rxc_source *source;

    /**
     * The outlet through which elements are emitted.
     */
    struct rxc_outlet *out;

    /**
     * Deallocate this handler.
     *
     * @param[in] self The reference to this handler.
     */
    void (*dealloc)(struct rxc_source_logic *self);

    /**
     * This function is invoked when the source has received a pull, and
     * therefore ready to emit an element.
     *
     * @param[in] self A reference to this handler.
     * @param[in] n The amount of elements to pull.
     */
    void (*on_pull)(struct rxc_source_logic *self, long n);

    /**
     * This function is invoked when the downstream sink will no longer accept
     * any new elements.
     *
     * @param[in] self The reference to this handler.
     */
    void (*on_downstream_finish)(struct rxc_source_logic *self);
};

/**
 * Determine whether the specified outlet is ready to be pushed.
 *
 * @param[in] out The outlet to check whether it is available.
 * @return <code>1</code> if available, otherwise <code>0</code>.
 */
int rxc_outlet_available(struct rxc_outlet *out);

/**
 * Emit the given element from the given source.
 *
 * @param[in] out The outlet to emit the element to.
 * @param[in] element The element to emit.
 * @return <code>RXC_EOK</code> on success, an error code otherwise.
 */
int rxc_outlet_emit(struct rxc_outlet *out, void *element);

/**
 * Signal a failure that occurred in the stage to the upstream sink.
 *
 * @param[in] out The outlet to signal the failure to.
 * @param[in] failure The failure that occurred.
 */
void rxc_outlet_fail(struct rxc_outlet *out, void *failure);

/**
 * Signal to the upstream sink that no more elements will be emitted.
 *
 * @param[in] out The outlet to signal to.
 */
void rxc_outlet_complete(struct rxc_outlet *out);

#endif /* RXC_LOGIC_H */
