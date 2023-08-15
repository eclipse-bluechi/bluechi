/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "network.h"

bool is_ipv4(const char *domain) {
        if (domain == NULL) {
                return false;
        }
        struct sockaddr_in sa;
        return (inet_pton(AF_INET, domain, &(sa.sin_addr)) == 1);
}

bool is_ipv6(const char *domain) {
        if (domain == NULL) {
                return false;
        }
        struct sockaddr_in6 sa;
        return (inet_pton(AF_INET6, domain, &(sa.sin6_addr)) == 1);
}

int get_address(const char *domain, char **ip_address, getaddrinfo_func resolve_addr) {
        if (domain == NULL) {
                return -EINVAL;
        }

        if (is_ipv4(domain) || is_ipv6(domain)) {
                return 0;
        }

        struct addrinfo hints, *res = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        int status = resolve_addr(domain, NULL, &hints, &res);
        if (status != 0) {
                bc_log_errorf("getaddrinfo failed: %s\n", gai_strerror(status));
                return status;
        }

        if (res->ai_family == AF_INET) {
                struct sockaddr_in *ipv4 = (struct sockaddr_in *) res->ai_addr;
                _cleanup_free_ char *reverse = typesafe_inet_ntop4(ipv4);
                if (reverse == NULL) {
                        freeaddrinfo(res);
                        return -ENOMEM;
                }
                *ip_address = steal_pointer(&reverse);
        } else if (res->ai_family == AF_INET6) {
                struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) res->ai_addr;
                _cleanup_free_ char *reverse = typesafe_inet_ntop6(ipv6);
                if (reverse == NULL) {
                        freeaddrinfo(res);
                        return -ENOMEM;
                }
                *ip_address = steal_pointer(&reverse);
        } else {
                bc_log_errorf("Unsupported address family '%d'", res->ai_family);
                freeaddrinfo(res);
                return -EFAULT;
        }

        freeaddrinfo(res);

        return 0;
}

char *typesafe_inet_ntop4(const struct sockaddr_in *addr) {
        char *dst = malloc0_array(0, sizeof(char), INET_ADDRSTRLEN);
        if (dst == NULL) {
                bc_log_error("AF_INET OOM: Failed allocate memory");
                return NULL;
        }
        inet_ntop(AF_INET, &addr->sin_addr, dst, INET_ADDRSTRLEN);
        return dst;
}

char *typesafe_inet_ntop6(const struct sockaddr_in6 *addr) {
        char *dst = malloc0_array(0, sizeof(char), INET6_ADDRSTRLEN);
        if (dst == NULL) {
                bc_log_error("AF_INET6 OOM: Failed allocate memory");
                return NULL;
        }
        inet_ntop(AF_INET6, &addr->sin6_addr, dst, INET6_ADDRSTRLEN);
        return dst;
}

char *assemble_tcp_address(const struct sockaddr_in *addr) {
        if (addr == NULL) {
                bc_log_error("INET4: Can not assemble an empty address");
                return NULL;
        }

        _cleanup_free_ char *host = typesafe_inet_ntop4(addr);

        char *dbus_addr = NULL;
        int r = asprintf(&dbus_addr, "tcp:host=%s,port=%d", host, ntohs(addr->sin_port));
        if (r < 0) {
                bc_log_error("INET4: Out of memory");
                return NULL;
        }
        return dbus_addr;
}

char *assemble_tcp_address_v6(const struct sockaddr_in6 *addr) {
        if (addr == NULL) {
                bc_log_error("INET6: Can not assemble an empty address");
                return NULL;
        }

        _cleanup_free_ char *host = typesafe_inet_ntop6(addr);
        char *dbus_addr = NULL;
        int r = asprintf(&dbus_addr, "tcp:host=%s,port=%d", host, ntohs(addr->sin6_port));
        if (r < 0) {
                bc_log_error("INET6: Out of memory");
                return NULL;
        }
        return dbus_addr;
}

char *get_hostname() {
        char hostname[BUFSIZ];
        memset((char *) hostname, 0, sizeof(hostname));
        if (gethostname(hostname, sizeof(hostname)) < 0) {
                fprintf(stderr, "Warning failed to gethostname, error code '%s'.\n", strerror(-errno));
                return "";
        }
        return strdup(hostname);
}
