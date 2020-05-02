#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sas/log.h>
#include <sas/transport.h>
#include <sas/server.h>

#include <sas/formats/wav.h>

#include "server.h"

#define SAS_SERVER_SESSION_TABLE_INITIAL_CAPACITY 32
#define SAS_SERVER_TIMEOUT_HEAP_INITIAL_CAPACITY  32
#define SAS_SERVER_SESSION_TIMEOUT                30 /* 5 seconds to timeout session */

struct sas_server * sas_server_alloc(void)
{
    struct sas_server *server = malloc(sizeof(struct sas_server));
    server->fd = -1;

    server->sessions.count = 0;
    server->sessions.capacity = SAS_SERVER_SESSION_TABLE_INITIAL_CAPACITY;
    server->sessions.table = calloc(server->sessions.capacity, sizeof(struct sas_server_session *));

    server->timeouts.count = 0;
    server->timeouts.capacity = SAS_SERVER_TIMEOUT_HEAP_INITIAL_CAPACITY;
    server->timeouts.heap = calloc(server->timeouts.capacity, sizeof(struct sas_server_session *));

    return server;
}

void sas_server_dealloc(struct sas_server *server)
{
    if (server->fd >= 0) {
        close(server->fd);
    }

    free(server->sessions.table);
    free(server->timeouts.heap);
    free(server);
}

int sas_server_init(struct sas_server *server)
{
    server->fd = -1;

    int fd = socket(AF_INET6, SOCK_DGRAM, 0);

    if (fd < 0) {
        return errno;
    }

    /* Enable local address reuse as to prevent address already bound errors */
    int option_value = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value))) {
        close(fd);
        return errno;
    }

    server->fd = fd;
    return 0;
}

int sas_server_bind(struct sas_server *server, struct sockaddr_in6 *addr)
{
    if (bind(server->fd, (const struct sockaddr *) addr, sizeof(struct sockaddr_in6))) {
        return errno;
    }
    return 0;
}

int sas_server_run(struct sas_server *server)
{
    int fd = server->fd, nb;
    fd_set fds;

    struct timeval timeval;
    struct timeval *timeout;

    timeval.tv_usec = 0;

    /* Allocate buffer on heap to prevent stack overflow on small stacks */
    size_t buffer_size = 4096;
    char *buffer = malloc(buffer_size);

    while (1) {
        /* Find session that will time out first */
        struct sas_server_session *timeout_session = sas_server_timeout_pop(server);
        time_t now = time(NULL);

        /* Check if the sessions have already expired */
        while (timeout_session && now >= timeout_session->timeout) {
            sas_server_timeout(server, timeout_session);
            timeout_session = sas_server_timeout_pop(server);
        }

        /* If no session is available, block indefinitely */
        if (timeout_session) {
            timeval.tv_sec = timeout_session->timeout - now;
            timeout = &timeval;
        } else {
            timeout = NULL;
        }

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        nb = select(fd + 1, &fds, NULL, NULL, timeout);

        if (nb < 0) {
            free(buffer);
            return errno;
        } else if (nb == 0) {
            /* Possibly time out session */
            sas_server_timeout(server, timeout_session);
        } else if (FD_ISSET(fd, &fds)) {
            /* Re-add session that was waiting for timeout */
            if (timeout_session) {
                sas_server_timeout_add(server, timeout_session);
            }

            socklen_t clientlen = sizeof(struct sockaddr_in6);
            struct sockaddr_in6 clientaddr;

            /* Receive packet header */
            ssize_t count = recvfrom(fd, buffer, buffer_size, 0,
                    (struct sockaddr *) &clientaddr, &clientlen);

            /* Ignore failed request */
            if (count == -1) {
                continue;
            }

            /* Get active session or create a new one */
            struct sas_server_session *session = sas_server_session_get(server, &clientaddr);

            sas_transport_packet_header_decode((struct sas_transport_packet_header *) buffer);
            sas_server_session_state_machine(server, session, (struct sas_transport_packet_header *) buffer, count);
            session->timeout = time(NULL) + SAS_SERVER_SESSION_TIMEOUT;
            sas_server_timeout_add(server, session);
        }
    }
}
