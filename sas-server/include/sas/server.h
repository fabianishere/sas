#ifndef SAS_SERVER_H
#define SAS_SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>

/**
 * A streaming audio server.
 */
struct sas_server;

/**
 * Allocate a server object.
 *
 * @return The allocated object or <code>NULL</code> on allocation failure.
 */
struct sas_server * sas_server_alloc(void);

/**
 * Deallocate the specified server instance.
 *
 * @param[in] server The server to deallocate.
 */
void sas_server_dealloc(struct sas_server *server);

/**
 * Initialize the specified server instance.
 *
 * @param[in] server The server to initialize.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_server_init(struct sas_server *server);

/**
 * Bind the specified server to the given address.
 *
 * @param[in] server The server to use.
 * @param[in] addr The address to bind the server to.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_server_bind(struct sas_server *server, struct sockaddr_in6 *addr);

/**
 * Run the specified server and handle the incoming requests.
 *
 * @param[in] server The server to run.
 * @return <code>0</code> on success, otherwise an error code.
 */
int sas_server_run(struct sas_server *server);

#endif /* SAS_SERVER_H */
