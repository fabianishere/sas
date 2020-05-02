#ifndef SAS_INTERNAL_SESSION_H
#define SAS_INTERNAL_SESSION_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <rxc/rxc.h>
#include <rxc/logic.h>

#include <sas/chunk.h>
#include <sas/transport.h>
#include <sas/server.h>

/**
 * A client that is connected to the server and has established a session.
 */
struct sas_server_session {
    /**
     * The address of the client.
     */
    struct sockaddr_in6 addr;

    /**
     * The state of the session.
     */
    uint8_t state;

    /**
     * The accumulated sequence number of the current session.
     */
    uint32_t seq;

    /**
     * The acknowledgment number.
     */
    uint32_t ack;

    /**
     * The window size of the client.
     */
    uint16_t send_window;

    /**
     * The window size of the server.
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
     * The point in time when the session will timeout.
     */
    time_t timeout;

    /**
     * The sample rate that is used.
     */
    int32_t sample_rate;

    /**
     * The sample size that is used.
     */
    int8_t sample_size;

    /**
     * The channels used.
     */
    int8_t channels;

    /**
     * The codec that is used.
     */
    uint8_t codec;

    /**
     * The file descriptor of the file to stream.
     */
    int fd;

    /**
     * The next session at this hash table index.
     */
    struct sas_server_session *next;
};

/**
 * The table of active sessions in the server.
 */
struct sas_server_session_table {
    /**
     * The amount of active sessions in the hash table.
     */
    int count;

    /**
     * The capacity of the session table.
     */
    int capacity;

    /**
     * A hash table of active sessions.
     */
    struct sas_server_session **table;
};

/**
 * Find an active connection in the server.
 *
 * @param[in] server The server to find the session for.
 * @param[in] addr The address of the session to look for.
 * @return The connection details of the client or <code>NULL</code>
 */
struct sas_server_session * sas_server_session_find(const struct sas_server *server,
                                                    const struct sockaddr_in6 *addr);

/**
 * Create a session for the specified address in the session hash table. When
 * a session already exists, it is returned instead.
 *
 * @param[in] server The server to create the session for.
 * @param[in] addr The address to create a session for.
 * @return The newly created server session or <code>NULL</code> on allocation
 * failure.
 */
struct sas_server_session * sas_server_session_get(struct sas_server *server,
                                                   const struct sockaddr_in6 *addr);

/**
 * Forget the specified session.
 *
 * @param[in] server The server to forget the session for.
 * @param[in] session The session to forget.
 */
void sas_server_session_delete(struct sas_server *server,
                               struct sas_server_session *session);

/**
 * Send the given packet to the specified session.
 *
 * @param[in] server The server to use.
 * @param[in] session The session to send the message to.
 * @param[in] packet The packet to send.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_server_session_send(struct sas_server *server,
                            struct sas_server_session *session,
                            struct sas_transport_packet_header *packet);

/**
 * Send the given chink of the given size to the specified session.
 *
 * @param[in] server The server to use.
 * @param[in] session The session to send the message to.
 * @param[in] chunk The chunk to send.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_server_session_send_chunk(struct sas_server *server,
                                  struct sas_server_session *session,
                                  struct sas_chunk *chunk);
/**
 * Reset the specified session.
 *
 * @param[in] server The server to use.
 * @param[in] session The session to reset.
 * @param[in] fin Flag to mark the session as finished.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_server_session_reset(struct sas_server *server,
                             struct sas_server_session *session,
                             int fin);
/**
 * Indicate an error to the session.
 *
 * @param[in] server The server that is running.
 * @param[in] session The session to indicate the error to.
 * @param[in] code The error code to send.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_server_session_error(struct sas_server *server,
                             struct sas_server_session *session,
                             int32_t code);

/**
 * Send a SYN-ACK message to the client.
 *
 * @param[in] server The server that is running.
 * @param[in] session The session to send the message to.
 * @param[
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_server_session_syn_ack(struct sas_server *server,
                               struct sas_server_session *session);

/**
 * This function represents the state machine of a session and is invoked
 * for every packet.
 *
 * @param[in] server The server that is handling the request.
 * @param[in] session The session that received a packet.
 * @param[in] header The packet header that was received.
 * @param[in] count The number of bytes read.
 */
void sas_server_session_state_machine(struct sas_server *server,
                                      struct sas_server_session *session,
                                      struct sas_transport_packet_header *header,
                                      size_t count);

#endif /* SAS_INTERNAL_SESSION_H */
