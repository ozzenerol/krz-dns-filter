#ifndef UDP_H
#define UDP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>

/* Configuration constants */
#define MAX_EVENTS 1000
#define BUFFER_SIZE 512
#define DEFAULT_PORT 5353

/* Server structure to hold state */
typedef struct {
    int server_fd;
    int epoll_fd;
    uint16_t port;
    bool running;
    struct sockaddr_in server_addr;
} UDP_SRV;

/* Initialized UDP_SRV struct => Creates socket, binds to port, sets up epoll */
int udp_srv_init(UDP_SRV *srv, uint16_t port);

/* Starts the UDP server and enters the main event loop */
int udp_srv_start(UDP_SRV *srv);

/* Stops the UDP server */
int udp_srv_stop(UDP_SRV *srv);

/* Cleanup socket and resources */
int udp_srv_cleanup(UDP_SRV *srv);


#endif // UDP_H
