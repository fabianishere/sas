#ifndef RXC_PIPELINE_H
#define RXC_PIPELINE_H

#include <rxc/scheduler.h>

/**
 * A closed, runnable pipeline through which elements can flow.
 */
struct rxc_pipeline {
    /**
     * The source of the pipeline.
     */
    struct rxc_source *source;

    /**
     * The sink of the pipeline.
     */
    struct rxc_sink *sink;
};

/**
 * An opaque interface (for API consumers) for the event receiving logic of
 * a sink.
 */
struct rxc_sink_logic;

/**
 * An interface for a reactive, back-pressured source of elements.
 */
struct rxc_source {
    /**
     * Deallocate this source.
     *
     * @param[in] self The reference to this source object.
     * @param[in] shallow A flag to indicate whether this object should not
     * deallocate its nested stages.
     */
    void (*dealloc)(struct rxc_source *self, int shallow);

    /**
     * Subscribe to this source with the specified sink.
     *
     * @param[in] self The reference to this source object.
     * @param[in] sink The sink to connect to this source.
     */
    void (*connect)(struct rxc_source *self, struct rxc_sink *sink);
};

/**
 * An interface for a reactive consumer of elements, that applies back-pressure
 * to upstream.
 */
struct rxc_sink {
    /**
     * Deallocate this source.
     *
     * @param[in] self The reference to this source object.
     * @param[in] shallow A flag to indicate whether this object should not
     * deallocate its nested stages.
     */
    void (*dealloc)(struct rxc_sink *self, int shallow);

    /**
     * Create the logic of this sink stage.
     *
     * @param[in] self The reference to this sink object.
     * @return The logic that processes the elements sent to the sink.
     */
    struct rxc_sink_logic * (*create_logic)(struct rxc_sink *self);
};

/**
 * An interface for a reactive, back-pressured transformer of elements.
 */
struct rxc_flow {
    /**
     * Deallocate this flow.
     *
     * @param[in] self The reference to this flow object.
     * @param[in] shallow A flag to indicate whether this object should not
     * deallocate its nested stages.
     */
    void (*dealloc)(struct rxc_flow *self, int shallow);

    /**
     * Connect this flow to the specified source.
     *
     * @param[in] self The reference to this flow object.
     * @param[in] source The source to connect this flow to.
     * @return The resulting source or <code>NULL</code> on failure.
     */
    struct rxc_source * (*connect_source)(struct rxc_flow *self,
                                          struct rxc_source *source);

    /**
     * Connect this flow to the specified sink.
     *
     * @param[in] self The reference to this flow object.
     * @param[in] sink The sink to connect this flow to.
     * @return The resulting sink or <code>NULL</code> on failure.
     */
    struct rxc_sink * (*connect_sink)(struct rxc_flow *self,
                                      struct rxc_sink *sink);
};

/**
 * Deallocate the specified pipeline object including all its stages.
 *
 * @param[in] pipeline The pipeline to deallocate.
 */
void rxc_pipeline_dealloc(struct rxc_pipeline *pipeline);

/**
 * Start the specified pipeline using the given scheduler.
 *
 * @param[in] pipeline The pipeline to run.
 * @param[in] scheduler The scheduler to run the pipeline with.
 * @return <code>RXC_EOK</code> on success, an error code otherwise.
 */
int rxc_pipeline_start(struct rxc_pipeline *pipeline,
                       struct rxc_scheduler *scheduler);

/**
 * Deallocate the given source object.
 *
 * @param[in] source The source object to deallocate.
 */
void rxc_source_dealloc(struct rxc_source *source);

/**
 * Connect the specified source to the given sink, forming a runnable graph.
 *
 * @param[in] source The source to connect.
 * @param[in] sink The sink to connect the source to.
 * @return The runnable graph that was built or <code>NULL</code>.
 */
struct rxc_pipeline * rxc_source_to(struct rxc_source *source,
                                    struct rxc_sink *sink);

/**
 * Connect the specified source to a flow, concatenating the processing steps of
 * both.
 *
 * @param[in] source The source to connect.
 * @param[in] flow The flow to connect the source to.
 * @return The source that was built or <code>NULL</code>.
 */
struct rxc_source * rxc_source_via(struct rxc_source *source,
                                   struct rxc_flow *flow);

/**
 * Deallocate the given sink object.
 *
 * @param[in] sink The sink to deallocate.
 */
void rxc_sink_dealloc(struct rxc_sink *sink);

/**
 * Deallocate the given flow object.
 *
 * @param[in] flow The flow to deallocate.
 */
void rxc_flow_dealloc(struct rxc_flow *flow);

/**
 * Connect the specified flows together to form a single flow, concatenating the
 * processing steps of both.
 *
 * @param[in] a The initial flow object.
 * @param[in] b The flow object to attach to the end of the first object.
 * @return The flow that was built or <code>NULL</code>.
 */
struct rxc_flow * rxc_flow_via(struct rxc_flow *a, struct rxc_flow *b);

/**
 * Connect the specified flow to a sink, concatenating the processing steps of
 * both.
 *
 * @param[in] flow The flow to connect.
 * @param[in] sink The sink to connect the flow to.
 * @return The sink that was built or <code>NULL</code>.
 */
struct rxc_sink * rxc_flow_to(struct rxc_flow *flow, struct rxc_sink *sink);

#endif /* RXC_PIPELINE_H */
