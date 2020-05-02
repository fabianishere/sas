#include <arpa/inet.h>

#include <sas/transport.h>

void sas_transport_packet_header_encode(struct sas_transport_packet_header *header)
{
    header->seq = htonl(header->seq);
    header->ack = htonl(header->ack);
    header->window = htons(header->window);
    header->length = htons(header->length);
}

void sas_transport_packet_header_decode(struct sas_transport_packet_header *header)
{
    header->seq = ntohl(header->seq);
    header->ack = ntohl(header->ack);
    header->window = ntohs(header->window);
    header->length = ntohs(header->length);
}

void sas_transport_packet_cfg_encode(struct sas_transport_packet_cfg *packet)
{
    /* Future-proofing; no fields actually need encoding */
}

void sas_transport_packet_cfg_decode(struct sas_transport_packet_cfg *packet)
{
    /* Future-proofing; no fields actually need decoding */
}

void sas_transport_packet_cfg_ack_encode(struct sas_transport_packet_cfg_ack *packet)
{
    packet->sample_rate = htonl(packet->sample_rate);
}

void sas_transport_packet_cfg_ack_decode(struct sas_transport_packet_cfg_ack *packet)
{
    packet->sample_rate = ntohl(packet->sample_rate);
}

void sas_transport_packet_err_encode(struct sas_transport_packet_err *packet)
{
    packet->err = htonl(packet->err);
}

void sas_transport_packet_err_decode(struct sas_transport_packet_err *packet)
{
    packet->err = ntohl(packet->err);
}
