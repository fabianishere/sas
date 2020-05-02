#ifndef SAS_INTERNAL_TIMEOUT_H
#define SAS_INTERNAL_TIMEOUT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sas/server.h>

#include "session.h"

/**
 * The timeout heap for the active sessions.
 */
struct sas_server_timeout_heap {
    /**
     * The amount of valid elements in the heap.
     */
    int count;

    /**
     * The capacity of the heap.
     */
    int capacity;

    /**
     * The heap entries.
     */
    struct sas_server_session **heap;
};

/**
 * This function is triggered when a timeout has occurred and will possibly
 * timeout the specified session.
 *
 * @param[in] server The server at which the timeout occurred.
 * @param[in] session The session that might have timed out.
 */
void sas_server_timeout(struct sas_server *server, struct sas_server_session *session);

/**
 * Add the specified session to the timeout heap.
 *
 * @param[in] server The server to use the heap from.
 * @param[in] session The session to add to the timeout heap.
 */
void sas_server_timeout_add(struct sas_server *server, struct sas_server_session *session);

/**
 * Remove the session with the lowest timeout from the timeout heap.
 *
 * @param[in] server The server to use the heap from.
 * @return The session with the lowest timeout time or <code>NULL</code> if the
 * heap is empty.
 */
struct sas_server_session * sas_server_timeout_pop(struct sas_server *server);

#endif /* SAS_INTERNAL_TIMEOUT_H */
