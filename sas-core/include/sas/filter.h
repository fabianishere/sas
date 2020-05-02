#ifndef SAS_FILTER_H
#define SAS_FILTER_H

#include <rxc/rxc.h>

/**
 * An audio stream filter.
 */
struct sas_filter {
    /**
     * Deallocate the specified filter.
     *
     * @param[in] filter The filter to deallocate.
     */
    void (*dealloc)(struct sas_filter *filter);

    /**
     * Initialize the specified filter.
     *
     * @param[in] filter The filter to initialize.
    */
    void (*init)(struct sas_filter *filter);

    /**
     * The flow that acts as the filter.
     */
    struct rxc_flow *flow;
};

#endif /* SAS_FILTER_H */
