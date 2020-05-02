#ifndef SAS_INTERNAL_SOURCE_H
#define SAS_INTERNAL_SOURCE_H

#include <rxc/rxc.h>
#include <rxc/logic.h>

#include <sas/client.h>

#include "client.h"

/**
 * A source that writes chunks to the audio sink.
 */
struct sas_client_session_source {
    struct rxc_source base;

    /**
     * The client to use.
     */
    struct sas_client *client;
};

/**
 * Create a source that writes chunks to the audio sink.
 *
 * @param[in] client The client to use.
 * @return The source that has been created or <code>NULL</code> on allocation
 * failure.
 */
struct rxc_source * sas_client_session_source(struct sas_client *client);

#endif /* SAS_INTERNAL_SOURCE_H */
