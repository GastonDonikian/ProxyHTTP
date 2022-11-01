#ifndef PC_2021A_03_ARGS_H
#define PC_2021A_03_ARGS_H

#include <stdbool.h>

struct doh {
    char           *host;
    char           *ip;
    char           * port;
    char           *path;
    char           *query;
};

struct httpargs {
    char           *http_addr;
    unsigned short  http_port;

    char *          mng_addr;  //management
    unsigned short  mng_port;

    bool            disectors_enabled;

    struct doh      doh;
};

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la seleccion humana. Puede cortar
 * la ejecución.
 */
void 
parse_args(const int argc, char **argv, struct httpargs *args);

#endif //PC_2021A_03_ARGS_H
