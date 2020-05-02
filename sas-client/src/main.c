#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <getopt.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <sas/log.h>
#include <sas/client.h>

#include <sas/drivers/ignore.h>

/**
 * Print the usage message of this program.
 *
 * @param[in] name The name of this program (most probably given by argv[0]).
 */
static void print_usage(const char *name) {
    fprintf(stderr, "usage: %s host file\n", name);
    fprintf(stderr, "\t-h --help\t\tshow a help message\n");
}

/* Command line options */
static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
};

/**
 * Main entry point of the client program.
 *
 * @param[in] argc The amount of arguments passed to the program.
 * @param[in] argv The command line arguments passed to the program.
 * @return <code>0</code> on success, otherwise failure.
 */
int main(int argc, char **argv)
{
    /* Parse command line options */
    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "h", long_options, &option_index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 0:
                if (long_options[option_index].flag != 0)
                    break;
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            case '?':
                print_usage(argv[0]);
                return EXIT_FAILURE;
            default:
                abort();
        }
    }

    if (argc <= 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    int err;
    struct sas_client *client = sas_client_alloc();

    if (!client) {
        sas_log(LOG_ERR "Failed to allocate server\n");
        return EXIT_FAILURE;
    }

    if ((err = sas_client_init(client, sas_drivers_ignore())) != 0) {
        sas_log(LOG_ERR "Failed to initialize client: %s\n", strerror(err));
        sas_client_dealloc(client);
        return EXIT_FAILURE;
    }

    int port = 3456;
    char *addr = argv[1];

    if ((err = sas_client_connect_hostname(client, addr, port)) != 0) {
        sas_log(LOG_ERR "Failed to connect to address %s: %s\n", argv[1], gai_strerror(err));
        sas_client_dealloc(client);
        return EXIT_FAILURE;
    }

    sas_log(LOG_INFO "Connected to address %s:%d\n", addr, port);
    sas_log(LOG_INFO "Requesting %s\n", argv[2]);
    if ((err = sas_client_receive(client, argv[2])) != 0) {
        sas_log(LOG_ERR "Failed to receive: %s\n", strerror(err));
        sas_client_dealloc(client);
        return EXIT_FAILURE;
    }
    sas_client_dealloc(client);

    return EXIT_SUCCESS;
}


