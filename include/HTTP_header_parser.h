#ifndef _HTTP_header_parser_
#define _HTTP_header_parser_
#include "parser.h"

/**
 * parser de un header de HTTP.
 *
 * Se encarga de encontrar el Host 
 */
/** eventos del parser HTTP */
enum HTTP_header_type {
    // message-header = field-name ":" [ field-value ]
    HTTP_HEADER_NEW_LINE,
    HTTP_HEADER_FIELD_NAME,
    HTTP_HEADER_COLON,
    HTTP_HEADER_FIELD_VALUE,
    // message-header = Host ":" [ hostPath ]
    HTTP_HEADER_FIN,
    HTTP_HEADER_CR,

    HTTP_HEADER_ERROR
};

/** la definición del parser */
const struct parser_definition * HTTP_header_parser(void);

//const char * HTTP_header_event(enum HTTP_header_type type);
#endif