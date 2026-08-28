#include "../include/dns.h"

#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int dns_get_domain(const unsigned char *buf, int len, char *out) {
    int pos = 12;
    int name_len = 0;

    while (pos < len) {
        uint8_t label_len = buf[pos];
        if (label_len == 0) { pos++; break; }
        if (pos + 1 + label_len >= len) return -1;

        if (name_len > 0) out[name_len++] = '.';
        memcpy(out + name_len, buf + pos + 1, label_len);
        name_len += label_len;
        pos += 1 + label_len;
    }
    out[name_len] = '\0';
    return name_len > 0 ? 0 : -1;
}

int dns_forward(int sock_fd, const char *buf, int len, const char *upstream_ip, int upstream_port, char *out, int out_size) {
    struct sockaddr_in upstream;
    memset(&upstream, 0, sizeof(upstream));
    upstream.sin_family = AF_INET;
    upstream.sin_port = htons(upstream_port);
    if (inet_pton(AF_INET, upstream_ip, &upstream.sin_addr) != 1)
        return -1;

    if (sendto(sock_fd, buf, len, 0, (struct sockaddr *)&upstream, sizeof(upstream)) < 0)
        return -1;

    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int n = recvfrom(sock_fd, out, out_size, 0, (struct sockaddr *)&from, &fromlen);
    if (n < 0)
        return -1;

    return n;
}

void dns_build_block_response(unsigned char *buf, int len) {
    if (len < 4) return;

    buf[2] |= 0x80;                  /* QR = 1 (response) */
    buf[3] = (buf[3] & 0xF0) | 0x03; /* RCODE = NXDOMAIN, keep other flag bits */
}
