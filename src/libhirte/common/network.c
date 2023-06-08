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

int get_address(const char *domain, char **ip_address) {
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
                hirte_log_errorf("getaddrinfo failed: %s\n", gai_strerror(status));
                return 1;
        }

        if (res->ai_family == AF_INET) { // IPv4 address
                struct sockaddr_in *ipv4 = (struct sockaddr_in *) res->ai_addr;
                *ip_address = typesafe_inet_ntop4(ipv4);
                if (ip_address == NULL) {
                        hirte_log_errorf("AF_INET: Failed to convert the IP address. errno: %d\n", errno);
                        return 1;
                }
        } else if (res->ai_family == AF_INET6) { // IPv6 address
                struct sockaddr_in *ipv6 = (struct sockaddr_in *) res->ai_addr;
                *ip_address = typesafe_inet_ntop6(ipv6);
                if (ip_address == NULL) {
                        hirte_log_errorf("AF_INET6: Failed to convert the IP address. errno: %d\n", errno);
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

char *typesafe_inet_ntop6(const struct sockaddr_in *addr) {
        char *dst = malloc0_array(0, sizeof(char), INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &addr->sin_addr, dst, INET6_ADDRSTRLEN);
        return dst;
}

char *assemble_tcp_address(const struct sockaddr_in *addr) {
        if (addr == NULL) {
                hirte_log_error("Can not assemble an empty address");
                return NULL;
        }

        _cleanup_free_ char *host = typesafe_inet_ntop4(addr);
        char *dbus_addr = NULL;
        int r = asprintf(&dbus_addr, "tcp:host=%s,port=%d", host, ntohs(addr->sin_port));
        if (r < 0) {
                hirte_log_error("Out of memory");
                return NULL;
        }
        return dbus_addr;
}
