#include "../include/log.h"
#include "../include/blacklist.h"
#include "../include/udp.h"

#include <signal.h>
#include <stdlib.h>

static UDP_SRV g_srv;

static void handle_signal(int sig) {
    (void) sig;
    udp_srv_stop(&g_srv);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        LOG_ERROR("Usage: %s </path/to/blacklist.txt> [port]", argv[0]);
        return EXIT_FAILURE;
    }

    if (blacklist_load(argv[1]) == -1) {
        LOG_ERROR("Failed to load the blacklist");
        return EXIT_FAILURE;
    }

    uint16_t port = argc > 2 ? (uint16_t) atoi(argv[2]) : DEFAULT_PORT;

    if (udp_srv_init(&g_srv, port) == -1) {
        LOG_ERROR("Failed to initialize the UDP server");
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    int rc = udp_srv_start(&g_srv);

    udp_srv_cleanup(&g_srv);

    return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
