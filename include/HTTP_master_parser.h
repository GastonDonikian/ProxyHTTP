#ifndef __HTTP_MASTER_PARSER_H__
#define __HTTP_MASTER_PARSER_H__
#include "parser.h"

/**
 * Parser de partes de un HTTP
 *
 * Se encarga de dividir las aprtes 
 */
/** eventos del parser HTTP */
enum HTTP_master_type {
    HTTP_MASTER_START_LINE,
    HTTP_MASTER_HEADER,
    HTTP_MASTER_DATA,
    HTTP_MASTER_FIN,
    HTTP_MASTER_ERROR
};

/** la definición del parser */
const struct parser_definition * HTTP_master_parser(void);

const char * HTTP_master_event(enum HTTP_master_type type);
#endif
