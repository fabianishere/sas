#ifndef SAS_TRANSPORT_H
#define SAS_TRANSPORT_H

#include <stdint.h>

#define SAS_TRANSPORT_PACKET_FLAG_FIN (1 << 0)
#define SAS_TRANSPORT_PACKET_FLAG_SYN (1 << 1)
#define SAS_TRANSPORT_PACKET_FLAG_RST (1 << 2)
#define SAS_TRANSPORT_PACKET_FLAG_ACK (1 << 3)
#define SAS_TRANSPORT_PACKET_FLAG_ERR (1 << 4)

#define SAS_TRANSPORT_STATE_INITIAL     0 /* Initial state of a session */
#define SAS_TRANSPORT_STATE_SYN_RCVD    1 /* A SYN has been received */
#define SAS_TRANSPORT_STATE_SYN_SENT    2 /* A SYN has been sent */
#define SAS_TRANSPORT_STATE_SACK_SENT   3 /* A SYN-ACK has been sent */
#define SAS_TRANSPORT_STATE_ESTABLSH    4 /* The connection has been established */
#define SAS_TRANSPORT_STATE_FINISHED    5 /* The connection is finished */

/**
 * A macro to determine whether a flag bit is set.
 */
#define sas_transport_packet_flag_isset(flags, flag) (((flags) & (flag)) == flag)

/**
 * The packet header format sas uses to communicate over UDP.
 */
struct sas_transport_packet_header {
    /**
     * The accumulated sequence number of the current session.
     */
    uint32_t seq;

    /**
     * The acknowledgment number indicating to the receiver the next sequence
     * number to be received.
     */
    uint32_t ack;

    /**
     * Reserved bits in the packet.
     */
    uint8_t reserved[1];

    /**
     * Control bits of the session.
     */
    uint8_t flags;

    /**
     * The window size of the sender.
     */
    uint16_t window;

    /**
     * The length of the remainder of the packet.
     */
    uint16_t length;
};

/**
 * Encode in-place the specified packet header for transmission over the
 * network.
 *
 * @param[in] header The header to encode.
 */
void sas_transport_packet_header_encode(struct sas_transport_packet_header *header);

/**
 * Decode in-place the specified packet header that was received from the
 * network.
 *
 * @param[in] header The header to decode.
 */
void sas_transport_packet_header_decode(struct sas_transport_packet_header *header);

/**
 * A packet sent to the synchronize with the server.
 */
struct sas_transport_packet_cfg {
    /**
     * The preferred codec to use.
     */
    uint8_t codec;
};

/**
 * Encode in-place the specified packet for transmission over the network.
 *
 * @param[in] packet The packet to encode.
 */
void sas_transport_packet_cfg_encode(struct sas_transport_packet_cfg *packet);

/**
 * Decode in-place the specified packet for transmission over the network.
 *
 * @param[in] packet The packet to decode.
 */
void sas_transport_packet_cfg_decode(struct sas_transport_packet_cfg *packet);

/**
 * A packet sent to acknowledge the configuration request of the client and
 * to establish the actual values used.
 */
struct sas_transport_packet_cfg_ack {
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
};

/**
 * Encode in-place the specified packet for transmission over the network.
 *
 * @param[in] packet The packet to encode.
 */
void sas_transport_packet_cfg_ack_encode(struct sas_transport_packet_cfg_ack *packet);

/**
 * Decode in-place the specified packet for transmission over the network.
 *
 * @param[in] packet The packet to decode.
 */
void sas_transport_packet_cfg_ack_decode(struct sas_transport_packet_cfg_ack *packet);

/**
 * A packet sent to the other side on failure.
 */
struct sas_transport_packet_err {
    /**
     * The code of the error that occurred.
     */
    int32_t err;
};

/**
 * Encode in-place the specified packet for transmission over the network.
 *
 * @param[in] packet The packet to encode.
 */
void sas_transport_packet_err_encode(struct sas_transport_packet_err *packet);

/**
 * Decode in-place the specified packet for transmission over the network.
 *
 * @param[in] packet The packet to decode.
 */
void sas_transport_packet_err_decode(struct sas_transport_packet_err *packet);

#endif /* SAS_TRANSPORT_H */
