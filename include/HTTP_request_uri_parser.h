#ifndef _HTTP_REQUEST_URI_PARSER_H_
#define _HTTP_REQUEST_URI_PARSER_H_

#include "parser.h"

enum HTTP_request_uri_type {
    HTTP_REQUEST_URI_START,
    HTTP_REQUEST_URI_MAY_ASTERISK,
    HTTP_REQUEST_URI_IS_ASTERISK,
    HTTP_REQUEST_URI_MAY_ABSOLUTE,
    HTTP_REQUEST_URI_ERROR,
    HTTP_REQUEST_URI_FIN,
};
const struct parser_definition *
HTTP_request_uri_parser(void);

#endif