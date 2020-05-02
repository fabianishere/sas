#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <time.h>

#include <sas/transport.h>
#include <sas/server.h>

#include "server.h"
#include "session.h"
#include "timeout.h"

void sas_server_timeout(struct sas_server *server, struct sas_server_session *session)
{
    if (time(NULL) >= session->timeout) {
        sas_server_session_delete(server, session);
    }
}

void sas_server_timeout_add(struct sas_server *server, struct sas_server_session *session)
{
    /* Resize (double) heap on maximum size */
    if (server->timeouts.count == server->timeouts.capacity) {
        server->timeouts.capacity *= 2;
        server->timeouts.heap = realloc(server->timeouts.heap,
                                        server->timeouts.capacity * sizeof(struct sas_server_session *));
    }

    int index = server->timeouts.count++;
    int parent;

    /* Add element at the end of the heap */
    server->timeouts.heap[index] = session;

    /* Up-heap bubble to preserve heap properties */
    while (index > 0) {
        parent = (index - 1) / 2;

        if (server->timeouts.heap[parent]->timeout < server->timeouts.heap[index]->timeout) {
            /* Min-heap property: parent is smaller than child */
            break;
        }

        /* Swap elements */
        struct sas_server_session *tmp = server->timeouts.heap[index];
        server->timeouts.heap[index] = server->timeouts.heap[parent];
        server->timeouts.heap[parent] = tmp;

        index = parent;
    }
}

static void sas_server_timeout_heapify(struct sas_server *server, int index)
{
    int left = 2 * index + 1;
    int right = left + 1;
    int lowest = index;

    if (server->timeouts.count > left && server->timeouts.heap[left]->timeout <
                                         server->timeouts.heap[lowest]->timeout) {
        lowest = left;
    }

    if (server->timeouts.count > right && server->timeouts.heap[right]->timeout <
                                          server->timeouts.heap[lowest]->timeout) {
        lowest = left;
    }

    if (lowest != index) {
        /* Swap these elements */
        struct sas_server_session *tmp = server->timeouts.heap[lowest];
        server->timeouts.heap[lowest] = server->timeouts.heap[index];
        server->timeouts.heap[index] = tmp;

        sas_server_timeout_heapify(server, lowest);
    }
}

struct sas_server_session * sas_server_timeout_pop(struct sas_server *server)
{
    if (server->timeouts.count == 0) {
        /* Heap is empty */
        return NULL;
    }

    struct sas_server_session *min = server->timeouts.heap[0];
    server->timeouts.heap[0] = server->timeouts.heap[--server->timeouts.count];
    sas_server_timeout_heapify(server, 0);
    return min;
}
