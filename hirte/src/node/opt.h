#pragma once

#include <netinet/in.h>

void get_opts(int argc, char *argv[], struct sockaddr_in *orch_addr);
