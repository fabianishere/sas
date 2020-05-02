#ifndef SAS_INTERNAL_CLIENT_H
#define SAS_INTERNAL_CLIENT_H

#include <rxc/rxc.h>

#include <sas/client.h>
#include <sas/chunk.h>

#define SAS_CLIENT_SESSION_WINDOW_SIZE 1024

struct sas_client {
    /**
     * The file descriptor of the socket created for the client.
     */
    int fd;

    /**
     * The sink to pipe the audio to.
     */
    struct rxc_sink *sink;

    /**
     * The session of the client.
     */
    struct sas_client_session {
        /**
         * The state of the session.
         */
        uint8_t state;

        /**
         * The accumulated sequence number of the current session.
         */
        uint32_t seq;

        /**
         * The acknowledgment number indicating the sequence number that has been
         * received by the sender of this packet.
         */
        uint32_t ack;

        /**
         * The window size of the server.
         */
        uint16_t send_window;

        /**
         * The window size of the client.
         */
        uint16_t receive_window;

        /**
         * The pipeline that is used for this session.
         */
        struct rxc_pipeline *pipeline;

        /**
         * The sink logic to control the sink.
         */
        struct rxc_sink_logic *sink_logic;

        /**
         * The chunk containing the audio information.
         */
        struct sas_chunk chunk;
    } session;
};

/**
 * Send a packet to the server.
 *
 * @param[in] client The client to use.
 * @param[in] packet The packet to send.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_client_session_send(struct sas_client *client, struct sas_transport_packet_header *packet);

/**
 * Sent a SYN packet to the server.
 *
 * @param[in] client The client to use.
 * @param[in] name The name of file to play.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_client_session_syn(struct sas_client *client, const char *name);

/**
 * Sent an ACK packet to the server.
 *
 * @param[in] client The client to use.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_client_session_ack(struct sas_client *client);

/**
 * This function represents the state machine of a session and is invoked
 * for every packet.
 *
 * @param[in] client The client that has received the packet.
 * @param[in] header The packet header that was received.
 * @param[in] count The number of bytes read.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_client_session_state_machine(struct sas_client *client,
                                     struct sas_transport_packet_header *header,
                                     size_t count);

#endif /* SAS_INTERNAL_CLIENT_H */
