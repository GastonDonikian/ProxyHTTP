#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <errno.h>
#include <getopt.h>

#include "include/mcpUtils.h"

static unsigned short
port(const char *s) {
     char *end     = 0;
     const long sl = strtol(s, &end, 10);

     if (end == s|| '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 0 || sl > USHRT_MAX) {
         fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
         exit(1);
         return 1;
     }
     return (unsigned short)sl;
}

static void
version(void) {
    fprintf(stderr, "HTTP version 1.1\n"
                    "ITBA Protocolos de Comunicación 2021a -- Grupo 3\n");
}

static void
usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s [OPTION]...\n"
        "\n"
        "   -h             Imprime la ayuda y termina.\n"
        "   -l <MCP addr>  Dirección utilizado para el protocolo MCP.\n"
        "   -p <MCP port>  Puerto utilizado para el protocolo MCP.\n"
        "\n",
        progname);
    exit(1);
}

void 
parse_args(const int argc, char **argv, struct mcpargs *args) {
    memset(args, 0, sizeof(*args)); // sobre todo para setear en null los punteros de users

    
    args->addr      = "127.0.0.1";
    args->port      = "9090";

    int c;

    while (true) {
        int option_index = 0;
        

        c = getopt(argc, argv, "hl:p:");
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'l':
                args->addr = optarg;
                break;
            case 'p':
                args->port = optarg;
                break;            

            
            default:
                fprintf(stderr, "unknown argument %d.\n", c);
                exit(1);
        }

    }

    if (optind < argc) {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
}