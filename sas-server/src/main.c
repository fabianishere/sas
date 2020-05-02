#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sas/log.h>
#include <sas/server.h>

/**
 * Main entry point of the server program.
 *
 * @param[in] argc The amount of arguments passed to the program.
 * @param[in] argv The command line arguments passed to the program.
 * @return <code>0</code> on success, otherwise failure.
 */
int main(int argc, char **argv)
{
    /* SPEC: The audio server must not take any command-line parameter */
    if (argc != 1) {
        sas_log("usage: %s\n", argv[0]);
        return EXIT_FAILURE;
    }

    int err;
    struct sas_server *server = sas_server_alloc();

    if (!server) {
        sas_log(LOG_ERR "Failed to allocate server\n");
        return EXIT_FAILURE;
    }

    if ((err = sas_server_init(server)) != 0) {
        sas_log(LOG_ERR "Failed to initialize server: %s\n", strerror(err));
        sas_server_dealloc(server);
        return EXIT_FAILURE;
    }

    int port = 3456;
    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any;
    addr.sin6_port = htons(port);

    if ((err = sas_server_bind(server, &addr)) != 0) {
        sas_log(LOG_ERR "Failed to bind to address: %s\n", strerror(err));
        sas_server_dealloc(server);
        return EXIT_FAILURE;
    }

    sas_log(LOG_INFO "Server bound to port %d\n", port);

    if ((err = sas_server_run(server)) != 0) {
        sas_log(LOG_ERR "%s\n", strerror(err));
        sas_server_dealloc(server);
        return EXIT_FAILURE;
    }

    sas_server_dealloc(server);
    return EXIT_SUCCESS;
}

