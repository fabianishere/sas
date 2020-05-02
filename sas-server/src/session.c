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
#include "session.h"
#include "sink.h"

#define SAS_SERVER_SESSION_WINDOW_SIZE 1024
#define SAS_SERVER_SESSION_CHUNK_SIZE  1024

/**
 * Hash the specified IPv6 socket address into a 64 bit integer.
 *
 * This function is based on the djb2 algorithm.
 * See http://www.cse.yorku.ca/~oz/hash.html
 *
 * @param[in] addr The socket address to hash.
 */
static unsigned long addr_hash(const struct sockaddr_in6 *addr)
{
    char bytes[18];

    memcpy(bytes, addr->sin6_addr.s6_addr, 16);
    memcpy(bytes + 16, &addr->sin6_port, 2);

    unsigned long hash = 5381;

    for (int i = 0; i < sizeof(bytes); i++) {
        hash = ((hash << 5) + hash) + bytes[i]; /* hash * 33 + c */
    }

    return hash;
}

/**
 * Compare the two given IPv6 socket addresses based on the IP address and port.
 *
 * @param a The first address to compare.
 * @param b The second address to compare.
 * @return <code>0</code> if the addresses are equal, some other value
 * otherwise.
 */
static int addr_eq(const struct sockaddr_in6 *a, const struct sockaddr_in6 *b)
{
    /* Fast-path: pointer-equality */
    if (a == b) {
        return 1;
    }

    int res = memcmp(a->sin6_addr.s6_addr, b->sin6_addr.s6_addr, sizeof(a->sin6_addr.s6_addr));
    if (res != 0) {
        return res;
    }

    return ntohs(a->sin6_port) == ntohs(b->sin6_port);
}

struct sas_server_session * sas_server_session_find(const struct sas_server *server,
                                                    const struct sockaddr_in6 *addr)
{
    unsigned int index = addr_hash(addr) % server->sessions.capacity;

    struct sas_server_session *session = server->sessions.table[index];

    while (session) {
        if (addr_eq(&session->addr, addr)) {
            break;
        }

        session = session->next;
    }

    return session;
}

struct sas_server_session * sas_server_session_get(struct sas_server *server,
                                                   const struct sockaddr_in6 *addr)
{
    unsigned int index = addr_hash(addr) % server->sessions.capacity;
    struct sas_server_session *prev = server->sessions.table[index];

    while (prev) {
        if (addr_eq(&prev->addr, addr)) {
            return prev;
        } else if (!prev->next) {
            break;
        }

        prev = prev->next;
    }

    struct sas_server_session *session = malloc(sizeof(struct sas_server_session));

    if (!session) {
        return NULL;
    }

    memcpy(&session->addr, addr, sizeof(struct sockaddr_in6));
    session->state = SAS_TRANSPORT_STATE_INITIAL;
    session->seq = 0;
    session->ack = 0;
    session->send_window = 0;
    session->receive_window = SAS_SERVER_SESSION_WINDOW_SIZE;
    session->pipeline = NULL;
    session->sink_logic = NULL;
    session->fd = -1;
    session->next = NULL;

    if (prev) {
        prev->next = session;
    } else {
        server->sessions.table[index] = session;
    }

    server->sessions.count++;
    sas_log(LOG_DEBUG "Active sessions: %d\n", server->sessions.count);
    return session;
}

void sas_server_session_delete(struct sas_server *server,
                               struct sas_server_session *session)
{
    unsigned int index = addr_hash(&session->addr) % server->sessions.capacity;

    struct sas_server_session *prev = NULL;
    struct sas_server_session *curr = server->sessions.table[index];

    while (curr) {
        if (addr_eq(&curr->addr, &session->addr)) {
            if (prev) {
                prev->next = curr->next;
            } else {
                server->sessions.table[index] = curr->next;
            }

            if (session->pipeline) {
                rxc_pipeline_dealloc(session->pipeline);
            }
            if (session->fd >= 0) {
                close(session->fd);
            }
            free(session);
            server->sessions.count--;
            sas_log(LOG_DEBUG "Active sessions: %d\n", server->sessions.count);
            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

int sas_server_session_send(struct sas_server *server,
                            struct sas_server_session *session,
                            struct sas_transport_packet_header *packet)
{
    size_t size = sizeof(struct sas_transport_packet_header) + packet->length;

    packet->seq = session->seq;
    packet->ack = session->ack;
    packet->window = session->receive_window;
    packet->reserved[0] = 0;

    session->seq += packet->length > 0 ? packet->length : 1;

    sas_transport_packet_header_encode(packet);

    if (sendto(server->fd, packet, size, 0,
               (const struct sockaddr *) &session->addr,
               sizeof(struct sockaddr_in6)) == -1) {
        return errno;
    }

    return 0;
}

int sas_server_session_send_chunk(struct sas_server *server,
                                  struct sas_server_session *session,
                                  struct sas_chunk *chunk)
{
    size_t size = chunk->size;
    struct sas_transport_packet_header *packet = malloc(sizeof(struct sas_transport_packet_header) + size);

    packet->flags = 0;
    packet->length = size;

    char *dst = (void *) &packet[1];
    memcpy(dst, chunk->buffer, size);

    int err = sas_server_session_send(server, session, packet);
    free(packet);
    return err;
}

int sas_server_session_reset(struct sas_server *server,
                             struct sas_server_session *session,
                             int fin)
{
    struct sas_transport_packet_header packet;

    packet.flags = SAS_TRANSPORT_PACKET_FLAG_RST;
    if (fin) {
        packet.flags |= SAS_TRANSPORT_PACKET_FLAG_FIN;
    }
    packet.length = 0;

    return sas_server_session_send(server, session, &packet);
}

int sas_server_session_error(struct sas_server *server,
                             struct sas_server_session *session,
                             int32_t code)
{
    char buffer[sizeof(struct sas_transport_packet_header) + sizeof(struct sas_transport_packet_err)];

    struct sas_transport_packet_header *header = (void *) buffer;

    header->flags = SAS_TRANSPORT_PACKET_FLAG_RST | SAS_TRANSPORT_PACKET_FLAG_ERR;
    header->length = sizeof(struct sas_transport_packet_err);

    struct sas_transport_packet_err *packet = (void *) &header[1];
    packet->err = code;
    sas_transport_packet_err_encode(packet);

    return sas_server_session_send(server, session, header);
}

int sas_server_session_syn_ack(struct sas_server *server,
                               struct sas_server_session *session)
{
    char buffer[sizeof(struct sas_transport_packet_header) + sizeof(struct sas_transport_packet_cfg_ack)];

    struct sas_transport_packet_header *header = (void *) buffer;

    header->flags = SAS_TRANSPORT_PACKET_FLAG_SYN | SAS_TRANSPORT_PACKET_FLAG_ACK;
    header->length = sizeof(struct sas_transport_packet_cfg_ack);

    struct sas_transport_packet_cfg_ack *packet = (void *) &header[1];

    packet->sample_rate = session->sample_rate;
    packet->sample_size = session->sample_size;
    packet->channels = session->channels;
    packet->codec = session->codec;

    sas_transport_packet_cfg_ack_encode(packet);

    return sas_server_session_send(server, session, header);
}

void sas_server_session_state_machine(struct sas_server *server,
                                      struct sas_server_session *session,
                                      struct sas_transport_packet_header *header,
                                      size_t count)
{
    int err;

    /* Validate packet */
    if (header->seq < session->ack) {
        /* At this point we are receiving a packet we have already ack'ed.
         * Thus, we drop the packet */
        sas_log(LOG_DEBUG "Dropping packet\n");
        return;
    } else if (header->seq > session->ack) {
        /* The packet is out-of-order. In the future, we could reorder them,
         * for now, we just process it */
        sas_log(LOG_DEBUG "Out-of-order packet\n");
    }

    /* Update send window */
    session->send_window = header->window;

    /* Ack this packet */
    session->ack = header->seq + (header->length > 0 ? header->length : 1);

    /* Forget the session on reset */
    if (sas_transport_packet_flag_isset(header->flags, SAS_TRANSPORT_PACKET_FLAG_RST)) {
        sas_server_session_delete(server, session);
        return;
    }

    switch (session->state) {
        case SAS_TRANSPORT_STATE_INITIAL: {
            /* Client did not sent SYN; reset connection */
            if (!sas_transport_packet_flag_isset(header->flags, SAS_TRANSPORT_PACKET_FLAG_SYN)) {
                sas_server_session_reset(server, session, 0);
                return;
            }

            struct sas_transport_packet_cfg *cfg = (void *) &header[1];
            sas_transport_packet_cfg_decode(cfg);
            char *name = strndup((char *) &cfg[1], header->length - sizeof(struct sas_transport_packet_cfg));

            if (cfg->codec != 0) {
                sas_server_session_error(server, session, EINVAL);
                sas_server_session_delete(server, session);
                return;
            }

            session->fd = open(name, O_RDONLY);

            free(name);

            if (session->fd < 0) {
                sas_server_session_error(server, session, errno);
                sas_server_session_delete(server, session);
                return;
            }

            /* Setup the streaming pipeline */
            struct rxc_source *source = sas_formats_wav(session->fd);
            if (!source) {
                sas_server_session_error(server, session, errno);
                sas_server_session_delete(server, session);
                return;
            }
            struct rxc_sink *sink = sas_server_session_sink(server, session);
            session->pipeline = rxc_source_to(source, sink);

            session->sample_rate = ((struct sas_formats_wav_source *) source)->sample_rate;
            session->sample_size = ((struct sas_formats_wav_source *) source)->sample_size;
            session->channels = ((struct sas_formats_wav_source *) source)->channels;

            sas_server_session_syn_ack(server, session);
            session->state = SAS_TRANSPORT_STATE_SACK_SENT;

            /* Start streaming pipeline */
            if ((err = rxc_pipeline_start(session->pipeline, rxc_scheduler_trampoline())) != 0) {
                sas_server_session_error(server, session, err);
                sas_server_session_delete(server, session);
                return;
            }
            break;
        }
        case SAS_TRANSPORT_STATE_SACK_SENT:
            session->state = SAS_TRANSPORT_STATE_ESTABLSH;
            /* Fall-through */
        case SAS_TRANSPORT_STATE_ESTABLSH:
            /* Request chunks from the source */
            rxc_inlet_pull(session->sink_logic->in, 1);
            break;
        default:
            sas_log(LOG_WARN "Unexpected state %d in state machine\n", session->state);
    }
}
