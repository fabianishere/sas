#ifndef RXC_OPS_CORE_H
#define RXC_OPS_CORE_H

struct rxc_source * rxc_source_count(long from, long to);

/**
 * Create a source with no elements.
 *
 * @return The source that has been created or <code>NULL</code> on allocation
 * failure.
 */
struct rxc_source * rxc_source_empty();

/**
 * Create a sink that invokes the specified callback on each element.
 *
 * @param[in] callback The callback to invoke for each element.
 * @return The sink that has been created or <code>NULL</code> on allocation
 * failure.
 */
struct rxc_sink * rxc_sink_foreach(void (*callback)(void *element));

/**
 * Create a sink that ignores all elements that are pushed to it.
 *
 * @return The sink that has been created or <code>NULL</code> on allocation
 * failure.
 */
struct rxc_sink * rxc_sink_ignore();

/**
 * Create a sink that immediately cancels upstream.
 *
 * @return The sink that has been created or <code>NULL</code> on allocation
 * failure.
 */
struct rxc_sink * rxc_sink_canceled();

/**
 * Create a flow from the specified function that wraps a sink.
 *
 * @param[in] wrap The function to wrap the specified sink.
 * @param[in] ctx The context to pass to the wrapping function.
 * @return The flow or <code>NULL</code> on allocation failure.
 */
struct rxc_flow * rxc_flow_wrapper(struct rxc_sink * (*wrap)(struct rxc_sink *, void *),
                                   void *ctx);

/**
 * Apply the given mapping over each element in the flow.
 *
 * @param[in] mapping The mapping to apply.
 * @return The flow that applies the mapping.
 */
struct rxc_flow * rxc_flow_map(void * (*mapping)(void *));

#endif /* RXC_OPS_CORE_H */
