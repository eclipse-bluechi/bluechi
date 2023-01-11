#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "../common/opt.h"
#include "../common/parse-util.h"
#include "opt.h"

const struct option options[] = { { ARG_HOST, required_argument, 0, ARG_HOST_SHORT },
                                  { ARG_PORT, required_argument, 0, ARG_PORT_SHORT },
                                  { NULL, 0, 0, '\0' } };

const char *get_optstring() {
        return "H:P:";
}

void get_opts(int argc, char *argv[], struct sockaddr_in *orch_addr) {
        int r = 0;
        int opt = 0;
        uint16_t port = 0;
        int hflag = 0;
        int pflag = 0;

        while ((opt = getopt_long(argc, argv, get_optstring(), options, NULL)) != -1) {
                switch (opt) {
                case ARG_HOST_SHORT:
                        r = inet_pton(AF_INET, optarg, &(orch_addr->sin_addr));
                        if (r < 1) {
                                fprintf(stderr, "Invalid host option '%s'\n", optarg);
                                exit(EXIT_FAILURE);
                        }

                        hflag = 1;
                        break;
                case ARG_PORT_SHORT:
                        if (!parse_port(optarg, &port)) {
                                fprintf(stderr, "Invalid port option '%s'\n", optarg);
                                exit(EXIT_FAILURE);
                        }
                        orch_addr->sin_port = htons(port);

                        pflag = 1;
                        break;
                default:
                        exit(EXIT_FAILURE);
                }
        }

        if (hflag != 1) {
                fprintf(stderr, "Missing host option. Usage: %s [-h host] [-p port]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
        if (pflag != 1) {
                fprintf(stderr, "Missing port option. Usage: %s [-h host] [-p port]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
}