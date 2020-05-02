#ifndef SAS_CLIENT_H
#define SAS_CLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>

#include <rxc/rxc.h>

/**
 * A client for the sas-server.
 */
struct sas_client;

/**
 * Allocate a client object.
 *
 * @return The allocated object or <code>NULL</code> on allocation failure.
 */
struct sas_client * sas_client_alloc(void);

/**
 * Deallocate the specified client instance.
 *
 * @param[in] client The client to deallocate.
 */
void sas_client_dealloc(struct sas_client *client);

/**
 * Initialize the specified client instance.
 *
 * @param[in] client The client to initialize.
 * @param[in] sink The sink to pipe the audio to.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_client_init(struct sas_client *client, struct rxc_sink *sink);

/**
 * Connect to the specified hostname with the client.
 *
 * @param[in] client The client to connect.
 * @param[in] hostname The hostname to connect to.
 * @param[in] port The port to connect to.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_client_connect_hostname(struct sas_client *client, const char *hostname,
                                int port);

/**
 * Request the specified song from the server and start streaming.
 *
 * @param[in] client The client to use.
 * @param[in] name The name of the song to receive.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_client_receive(struct sas_client *client, const char *name);

#endif /* SAS_CLIENT_H */
