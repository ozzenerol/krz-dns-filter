#include "../include/udp.h"
#include "../include/log.h"
#include "../include/dns.h"
#include "../include/blacklist.h"
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/time.h>

#define UPSTREAM_DNS_DEFAULT_PORT    53
#define UPSTREAM_TIMEOUT_SEC         2

// Just Cloudflare and Google DNS ips. 
// If these two go down, the whole world is fucked so I don't care about any other DNS ip
static const char *upstream_dns_ips[] = { "1.1.1.1", "8.8.8.8" }; 
static const size_t upstream_dns_ips_size = 2;

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        LOG_ERROR("Error while getting flags for fd=%d", fd);
        return -1;
    }

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int setup_socket(UDP_SRV *srv) {
    srv->server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (srv->server_fd == -1) {
        LOG_ERROR("Error while setting up socket: %d", errno);
        return -1;
    }

    int reuse = 1;
    if (setsockopt(srv->server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        LOG_ERROR("Error while setting socket options: %d", errno);
        return -1;
    }

    memset(&srv->server_addr, 0, sizeof(srv->server_addr));
    srv->server_addr.sin_family = AF_INET;
    srv->server_addr.sin_addr.s_addr = INADDR_ANY;
    srv->server_addr.sin_port = htons(srv->port);

    if (bind(srv->server_fd, (struct sockaddr*) &srv->server_addr, sizeof(srv->server_addr)) == -1) {
        LOG_ERROR("Error while binding socket (fd=%d) to port %u: %d", srv->server_fd, srv->port, errno);
        return -1;
    }

    if (set_nonblocking(srv->server_fd) == -1) {
        LOG_ERROR("Error while setting the server (fd=%d) to non blocking", srv->server_fd);
        return -1;
    }

    return 0;
}

static int setup_epoll(UDP_SRV *srv) {
    srv->epoll_fd = epoll_create1(0);
    if (srv->epoll_fd == -1) {
        LOG_ERROR("Error while setting up epoll: %d", errno);
        return -1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = srv->server_fd;
    
    if (epoll_ctl(srv->epoll_fd, EPOLL_CTL_ADD, srv->server_fd, &ev) == -1) {
        LOG_ERROR("Error while adding the socket to epoll: %d", errno);
        return -1;
    }

    return 0;
}

static void handle_client_data(UDP_SRV *srv, struct sockaddr_in *client_addr, socklen_t client_len,
        char *buffer, ssize_t bytes_received) {
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));

    char domain[BUFFER_SIZE];
    if (dns_get_domain((unsigned char *) buffer, (int) bytes_received, domain) == -1) {
        LOG_WARN("Malformed DNS packet from %s:%d, dropping", client_ip, ntohs(client_addr->sin_port));
        return;
    }

    LOG_INFO("Query from %s:%d -> %s", client_ip, ntohs(client_addr->sin_port), domain);

    if (blacklist_contains(domain) != NULL) {
        LOG_WARN("Blocked domain: %s", domain);

        dns_build_block_response((unsigned char *) buffer, (int) bytes_received);

        if (sendto(srv->server_fd, buffer, bytes_received, 0,
                (struct sockaddr *) client_addr, client_len) < 0)
            LOG_WARN("Failed to send blocked response to %s:%d", client_ip, ntohs(client_addr->sin_port));

        return;
    }

    int upstream_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (upstream_fd == -1) {
        LOG_ERROR("Failed to create upstream socket: %d", errno);
        return;
    }

    struct timeval tv = { .tv_sec = UPSTREAM_TIMEOUT_SEC, .tv_usec = 0 };
    setsockopt(upstream_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char response[BUFFER_SIZE];
    
    int response_len = 0;
    for (size_t i = 0; i < upstream_dns_ips_size; i++) {
        response_len = dns_forward(upstream_fd, buffer, (int) bytes_received, upstream_dns_ips[i], UPSTREAM_DNS_DEFAULT_PORT, response, sizeof(response));
        if (response_len < 0) {
            LOG_WARN("Upstream %s failed to respond. Attempting the next upstream server", upstream_dns_ips[i]);
            continue;
        } else {
            close(upstream_fd);
            break;
        }
    }

    if (response_len < 0) {
        LOG_WARN("No response from any registered upstreams DNS { %s or %s } for query %s", upstream_dns_ips[0], upstream_dns_ips[1], domain);
        return;
    }

    if (sendto(srv->server_fd, response, response_len, 0,
            (struct sockaddr *) client_addr, client_len) < 0)
        LOG_WARN("Failed to relay upstream response to %s:%d", client_ip, ntohs(client_addr->sin_port));
}

int udp_srv_init(UDP_SRV *srv, uint16_t port) {
    if (srv == NULL) {
        LOG_ERROR("UDP_SRV *srv cannot be NULL");
        return -1;
    }

    memset(srv, 0, sizeof(UDP_SRV));
    srv->port = port > 0 ? port : DEFAULT_PORT;
    srv->running = false;

    if (setup_socket(srv) == -1) 
        return -1;

    if (setup_epoll(srv) == -1) {
        close(srv->server_fd);
        return -1;
    }

    return 0; 
}

int udp_srv_start(UDP_SRV *srv) {
    if (srv == NULL || srv->server_fd == -1 || srv->epoll_fd == -1) {
        LOG_ERROR("Server is not properly initialized");
        return -1;
    }

    struct epoll_event events[MAX_EVENTS];
    struct sockaddr_in client_addr;
    char buffer[BUFFER_SIZE];

    LOG_INFO("UDP server listening on port %u", srv->port);
    LOG_INFO("Using epoll for event handling");
    LOG_INFO("Press Ctrl + C to stop");

    srv->running = true;

    while (srv->running) {
        int num_events = epoll_wait(srv->epoll_fd, events, MAX_EVENTS, -1);
        if (num_events < 0) {
            if (errno == EINTR)
                continue;
            break;
        }

        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == srv->server_fd) {
                memset(buffer, 0, BUFFER_SIZE);
                socklen_t client_len = sizeof(client_addr);
                ssize_t bytes_received = recvfrom(
                            srv->server_fd, buffer,
                            BUFFER_SIZE - 1, 0,
                            (struct sockaddr*) &client_addr,
                            &client_len
                        );

                if (bytes_received < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) 
                        continue;
                    continue;
                }

                handle_client_data(srv, &client_addr, client_len, buffer, bytes_received);
            }
        }
    }

    return 0;
}

int udp_srv_stop(UDP_SRV *srv) {
    if (srv == NULL) {
        LOG_ERROR("UDP_SRV *srv cannot be NULL");
        return -1;
    }

    srv->running = false;
    return 0;
}

int udp_srv_cleanup(UDP_SRV *srv) {
    if (srv == NULL) {
        LOG_ERROR("UDP_SRV *srv cannot be NULL");
        return -1;
    }

    if (srv->epoll_fd != -1) {
        close(srv->epoll_fd);
        srv->epoll_fd = -1;
    }

    if (srv->server_fd != -1) {
        close(srv->server_fd);
        srv->server_fd = -1;
    }

    return 0;
}
