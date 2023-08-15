/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "libbluechi/common/common.h"
#include "libbluechi/log/log.h"

bool is_ipv4(const char *domain);
bool is_ipv6(const char *domain);

char *get_hostname();

char *typesafe_inet_ntop4(const struct sockaddr_in *addr);
char *typesafe_inet_ntop6(const struct sockaddr_in6 *addr);

char *assemble_tcp_address(const struct sockaddr_in *addr);
char *assemble_tcp_address_v6(const struct sockaddr_in6 *addr);

typedef int (*getaddrinfo_func)(
                const char * restrict node,
                const char * restrict service,
                const struct addrinfo * restrict hints,
                struct addrinfo ** restrict res);
int get_address(const char *domain, char **ip_address, getaddrinfo_func resolve_addr);
