#ifndef DNS_H
#define DNS_H

int dns_get_domain(const unsigned char *buf, int len, char *out);
int dns_forward(int sock_fd, const char *buf, int len, const char *upstream_ip, int upstream_port, char *out, int out_size);

/* Turns a DNS query already sitting in buf into a minimal NXDOMAIN response
 * in place (sets QR and RCODE only, no answer records). Used to reply to
 * blacklisted queries without contacting the upstream resolver. */
void dns_build_block_response(unsigned char *buf, int len);

#endif // DNS_H
