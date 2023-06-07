/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "network.h"

bool isIPv4Addr(const char *domain) {
        if (domain == NULL) {
                return false;
        }
        struct sockaddr_in sa;
        return (inet_pton(AF_INET, domain, &(sa.sin_addr)) == 1);
}

bool isIPv6Addr(const char *domain) {
        if (domain == NULL) {
                return false;
        }
        struct sockaddr_in6 sa;
        return (inet_pton(AF_INET6, domain, &(sa.sin6_addr)) == 1);
}

int get_address(const char *domain, char *ip_address, size_t ip_address_size) {
        if (domain == NULL) {
                return 1;
        }

        // Already an IP, no need to resolve
        if (isIPv4Addr(domain) || isIPv6Addr(domain)) {
                return 1;
        }

        struct addrinfo hints, *res = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        int status = getaddrinfo(domain, NULL, &hints, &res);
        if (status != 0) {
                fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(status));
                return 1;
        }

        void *addr = NULL;
        if (res->ai_family == AF_INET) { // IPv4 address
                struct sockaddr_in *ipv4 = (struct sockaddr_in *) res->ai_addr;
                addr = &(ipv4->sin_addr);
                if (inet_ntop(AF_INET, addr, ip_address, ip_address_size) == NULL) {
                        fprintf(stderr, "AF_INET: Failed to convert the IP address. errno: %d\n", errno);
                        return 1;
                }
        } else if (res->ai_family == AF_INET6) { // IPv6 address
                struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) res->ai_addr;
                addr = &(ipv6->sin6_addr);
                if (inet_ntop(AF_INET6, addr, ip_address, ip_address_size) == NULL) {
                        fprintf(stderr, "AF_INET6: Failed to convert the IP address. errno: %d\n", errno);
                        return 1;
                }
        }

        freeaddrinfo(res);

        return 0;
}

char *typesafe_inet_ntop4(const struct sockaddr_in *addr) {
        char *dst = malloc0_array(0, sizeof(char), INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &addr->sin_addr, dst, INET_ADDRSTRLEN);
        return dst;
}
