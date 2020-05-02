#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>

#include <sas/log.h>
#include <sas/client.h>
#include <sas/transport.h>

#include "client.h"
#include "source.h"

struct sas_client * sas_client_alloc(void)
{
    struct sas_client *client = malloc(sizeof(struct sas_client));
    client->fd = -1;
    return client;
}

void sas_client_dealloc(struct sas_client *client)
{
    if (client->fd >= 0) {
        close(client->fd);
    }

    if (client->session.pipeline) {
        rxc_pipeline_dealloc(client->session.pipeline);
    }

    free(client);
}

int sas_client_init(struct sas_client *client, struct rxc_sink *sink)
{
    client->fd = -1;
    client->sink = sink;

    client->session.seq = 0;
    client->session.ack = 0;
    client->session.state = SAS_TRANSPORT_STATE_INITIAL;
    client->session.send_window = 0;
    client->session.receive_window = SAS_CLIENT_SESSION_WINDOW_SIZE;
    client->session.pipeline = NULL;
    client->session.sink_logic = NULL;

    return 0;
}

int sas_client_connect_hostname(struct sas_client *client, const char *hostname,
                                int port)
{
    if (client->fd >= 0) {
        close(client->fd);
        client->fd = -1;
    }

    char port_str[6];
    sprintf(port_str, "%d", port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_ADDRCONFIG;

    /* Retrieve address information for hostname */
    struct addrinfo *target;
    int err = getaddrinfo(hostname, port_str, &hints, &target);
    if (err) {
        return err;
    }

    /* Check each address until we have successfully connected to one */
    struct addrinfo *ai;
    int fd = -1;
    for (ai = target; ai != NULL; ai = ai->ai_next) {
        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

        if (fd < 0) {
            continue;
        }

        if (connect(fd, ai->ai_addr, ai->ai_addrlen) != -1) {
            break;
        }

        close(fd);
    }

    if (!ai) {
        freeaddrinfo(target);
        return EAI_NONAME;
    }

    client->fd = fd;

    freeaddrinfo(target);

    return 0;
}

int sas_client_session_send(struct sas_client *client, struct sas_transport_packet_header *packet)
{
    size_t size = sizeof(struct sas_transport_packet_header) + packet->length;

    packet->seq = client->session.seq;
    packet->ack = client->session.ack;
    packet->window = client->session.receive_window;
    packet->reserved[0] = 0;

    client->session.seq += packet->length > 0 ? packet->length : 1;

    sas_transport_packet_header_encode(packet);

    if (send(client->fd, packet, size, 0) == -1) {
        return errno;
    }

    return 0;
}

int sas_client_session_syn(struct sas_client *client, const char *name)
{
    int16_t name_len = strlen(name);
    int size = sizeof(struct sas_transport_packet_header) +
               sizeof(struct sas_transport_packet_cfg) + name_len;
    char packet_buffer[size];
    struct sas_transport_packet_header *header = (void *) packet_buffer;

    if (!header) {
        return ENOMEM;
    }

    header->flags = SAS_TRANSPORT_PACKET_FLAG_SYN;
    header->length = sizeof(struct sas_transport_packet_cfg) + name_len;

    client->session.state = SAS_TRANSPORT_STATE_SYN_SENT;

    struct sas_transport_packet_cfg *cfg = (void *) &header[1];
    cfg->codec = 0;

    sas_transport_packet_cfg_encode(cfg);

    char *buffer = (char *) &cfg[1];
    memcpy(buffer, name, name_len);

    return sas_client_session_send(client, header);
}

int sas_client_session_ack(struct sas_client *client)
{
    struct sas_transport_packet_header packet;

    packet.flags = SAS_TRANSPORT_PACKET_FLAG_ACK;
    packet.length = 0;

    return sas_client_session_send(client, &packet);
}

static void chunk_dealloc(struct sas_chunk *chunk)
{
    /* The chunk memory is re-used. No de-allocation needed */
    (void) chunk;
}

int sas_client_session_state_machine(struct sas_client *client,
                                     struct sas_transport_packet_header *header,
                                     size_t count)
{

    /* Validate packet */
    if (header->seq < client->session.ack) {
        /* At this point we are receiving a packet we have already ack'ed.
         * Thus, we drop the packet */
        sas_log(LOG_DEBUG "Dropping packet\n");
        return 0;
    } else if (header->seq > client->session.ack) {
        /* The packet is out-of-order. In the future, we could reorder them,
         * for now, we just process it */
        sas_log(LOG_DEBUG "Out-of-order packet\n");
    }

    /* Update send window */
    client->session.send_window = header->window;

    /* Ack this packet */
    client->session.ack = header->seq + (header->length > 0 ? header->length : 1);

    /* Forget the session on reset */
    if (sas_transport_packet_flag_isset(header->flags, SAS_TRANSPORT_PACKET_FLAG_RST)) {
        int err = ECONNRESET;

        if (sas_transport_packet_flag_isset(header->flags, SAS_TRANSPORT_PACKET_FLAG_ERR)) {
            /* An error occurred */
            struct sas_transport_packet_err *packet = (void *) &header[1];
            sas_transport_packet_err_decode(packet);
            err = packet->err;
        } else if (sas_transport_packet_flag_isset(header->flags, SAS_TRANSPORT_PACKET_FLAG_FIN)) {
            /* Stream finished successfully */
            err = 0;
            client->session.state = SAS_TRANSPORT_STATE_FINISHED;
        }

        return err;
    }

    switch (client->session.state) {
        case SAS_TRANSPORT_STATE_SYN_SENT: {
            struct sas_transport_packet_cfg_ack *cfg = (void *) &header[1];
            sas_transport_packet_cfg_ack_decode(cfg);

            client->session.chunk.dealloc = chunk_dealloc;
            client->session.chunk.sample_rate = cfg->sample_rate;
            client->session.chunk.sample_size = cfg->sample_size;
            client->session.chunk.channels = cfg->channels;
            client->session.chunk.codec = cfg->codec;
            client->session.chunk.size = header->length - sizeof(struct sas_transport_packet_cfg_ack);
            client->session.chunk.buffer = (void *) &cfg[1];

            sas_log(LOG_INFO "%d %d %d\n", cfg->sample_rate, cfg->sample_size, cfg->channels);

            /* Setup the streaming pipeline */
            int err;
            struct rxc_source *source = sas_client_session_source(client);
            client->session.pipeline = rxc_source_to(source, client->sink);

            if ((err = rxc_pipeline_start(client->session.pipeline, rxc_scheduler_trampoline())) != 0) {
                return -1;
            }

            sas_client_session_ack(client);
            client->session.state = SAS_TRANSPORT_STATE_ESTABLSH;

            /* Push configuration to sink */
            client->session.sink_logic->on_push(client->session.sink_logic, &client->session.chunk);
            break;
        }
        case SAS_TRANSPORT_STATE_ESTABLSH: {
            /* Push chunk to sink */
            client->session.chunk.size = header->length - sizeof(struct sas_transport_packet_cfg_ack);
            client->session.chunk.buffer = (void *) &header[1];
            client->session.sink_logic->on_push(client->session.sink_logic, &client->session.chunk);
            client->session.receive_window = 0;

            sas_client_session_ack(client);
            break;
        }
        default:
            sas_log(LOG_WARN "Unexpected state %d in state machine\n", client->session.state);
    }

    return 0;
}

int sas_client_receive(struct sas_client *client, const char *name)
{
    int fd = client->fd, nb, err;
    fd_set fds;
    struct timeval timeout;

    /* Allocate buffer on heap to prevent stack overflow on small stacks */
    size_t buffer_size = 4096;
    char *buffer = malloc(buffer_size);

    if ((err = sas_client_session_syn(client, name)) != 0) {
        free(buffer);
        return err;
    }

    while (client->session.state != SAS_TRANSPORT_STATE_FINISHED) {
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        nb = select(fd + 1, &fds, NULL, NULL, &timeout);

        if (nb < 0) {
            free(buffer);
            return errno;
        } else if (nb == 0) {
            free(buffer);
            return ETIMEDOUT;
        } else if (FD_ISSET(fd, &fds)) {
            /* Receive packet header */
            ssize_t count = recv(fd, buffer, buffer_size, 0);

            /* Check for failure */
            if (count == -1) {
                return errno;
            }

            sas_transport_packet_header_decode((struct sas_transport_packet_header *) buffer);
            err = sas_client_session_state_machine(client, (struct sas_transport_packet_header *) buffer, count);

            if (err != 0) {
                free(buffer);
                return err;
            }
        }
    }

    return 0;
}
