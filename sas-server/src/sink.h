#ifndef SAS_INTERNAL_SINK_H
#define SAS_INTERNAL_SINK_H

#include <rxc/rxc.h>
#include <rxc/logic.h>

#include <sas/server.h>

#include "session.h"

/**
 * A sink that writes chunks to the client session.
 */
struct sas_server_session_sink {
    struct rxc_sink base;

    /**
     * The server to use.
     */
    struct sas_server *server;

    /**
     * The session of this sink.
     */
    struct sas_server_session *session;
};

/**
 * Create a sink that writes chunks to the client session.
 *
 * @param[in] server The server to use.
 * @param[in] session The session to write to.
 * @return The sink that has been created or <code>NULL</code> on allocation
 * failure.
 */
struct rxc_sink * sas_server_session_sink(struct sas_server *server,
                                          struct sas_server_session *session);

#endif /* SAS_INTERNAL_SERVER_H */
