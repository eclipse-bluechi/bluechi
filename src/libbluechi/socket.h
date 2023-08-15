/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <inttypes.h>

int create_tcp_socket(uint16_t port);
int accept_tcp_connection_request(int fd);

int fd_check_peercred(int fd);
