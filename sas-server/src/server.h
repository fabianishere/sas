#ifndef SAS_INTERNAL_SERVER_H
#define SAS_INTERNAL_SERVER_H

#include <sas/server.h>

#include "session.h"
#include "timeout.h"

struct sas_server {
    /**
     * The file descriptor of the socket created for the server.
     */
    int fd;

    /**
     * The table of active sessions in the server.
     */
    struct sas_server_session_table sessions;

    /**
     * The timeout heap for the active sessions.
     */
    struct sas_server_timeout_heap timeouts;
};

#endif /* SAS_INTERNAL_SERVER_H */
