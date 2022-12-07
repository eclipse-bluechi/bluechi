#include "opt.h"
#include "../common/parse-util.h"

#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

void get_opts(int argc, char *argv[], struct sockaddr_in *orchAddr) {
        int r = 0;
        int opt = 0;
        int port = 0;
        int hflag = 0;
        int pflag = 0;

        while ((opt = getopt(argc, argv, "h:p:")) != -1) {
                switch (opt) {
                case 'h':
                        r = inet_pton(AF_INET, optarg, &(orchAddr->sin_addr));
                        if (r < 1) {
                                fprintf(stderr, "Invalid host option '%s'\n", optarg);
                                exit(EXIT_FAILURE);
                        }

                        hflag = 1;
                        break;
                case 'p':
                        if (parse_long(optarg, (long *) &port) != 0) {
                                fprintf(stderr, "Invalid port option '%s'\n", optarg);
                                exit(EXIT_FAILURE);
                        }
                        orchAddr->sin_port = htons(port);

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