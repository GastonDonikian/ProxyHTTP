#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <errno.h>
#include <getopt.h>

#include "include/args.h"

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
        "   -h              Imprime la ayuda y termina.\n"
        "   -N              Deshabilita los password disectors.\n"
        "   -l <HTTP addr>  Dirección donde servirá el proxy HTTP.\n"
        "   -L <conf  addr> Dirección donde servirá el servicio de management.\n"
        "   -p <HTTP port>  Puerto entrante conexiones HTTP.\n"
        "   -o <conf port>  Puerto entrante conexiones configuracion.\n"
        "   -v              Imprime información sobre la versión versión y termina.\n"
        "\n"
        "   --doh-ip    <ip>    \n"
        "   --doh-port  <port>  XXX\n"
        "   --doh-host  <host>  XXX\n"
        "   --doh-path  <host>  XXX\n"
        "   --doh-query <host>  XXX\n"

        "\n",
        progname);
    exit(1);
}

void 
parse_args(const int argc, char **argv, struct httpargs *args) {
    memset(args, 0, sizeof(*args));

    args->http_addr = "0.0.0.0";
    args->http_port = 8080;

    args->mng_addr   = "127.0.0.1";
    args->mng_port   = 9090;

    args->disectors_enabled = true;

    args->doh.host = "localhost";
    args->doh.ip   = "127.0.0.1";
    args->doh.port = "8000";
    //Lo ponemos pero no lo usamos porque va con POST
    args->doh.path = "/getnsrecord";
    args->doh.query = "?dns=";

    int c;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            { "doh-ip",    required_argument, 0, 0xD001 },
            { "doh-port",  required_argument, 0, 0xD002 },
            { "doh-host",  required_argument, 0, 0xD003 },
            { "doh-path",  required_argument, 0, 0xD004 },
            { "doh-query", required_argument, 0, 0xD005 },
            { 0,           0,                 0, 0 }
        };

        c = getopt_long(argc, argv, "hl:L:Np:P:o:v", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'N':
                args->disectors_enabled = false;
                break;
            case 'l':
                args->http_addr = optarg;
                break;
            case 'L':
                args->mng_addr = optarg;
                break;
            case 'o':
                args->mng_port = atoi(optarg);
                break;            
            case 'p':
                args->http_port = port(optarg);
                break;
            
            case 'v':
                version();
                exit(0);
                break;
                
            case 0xD001:
                args->doh.ip = optarg;
                break;
            case 0xD002:
                args->doh.port = optarg;
//                sprintf( args->doh.port , "%s" ,optarg);
                break;
            case 0xD003:
                args->doh.host = optarg;
                break;
            case 0xD004:
                args->doh.path = optarg;
                break;
            case 0xD005:
                args->doh.query = optarg;
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