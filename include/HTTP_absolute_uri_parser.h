#ifndef _HTTP_ABSOLUTE_URI_PARSER_H_
#define _HTTP_ABSOLUTE_URI_PARSER_H_

#include "parser.h"

enum HTTP_absolute_uri_type {
    HTTP_ABSOLUTE_URI_SCHEME,
    HTTP_ABOSLUTE_URI_2SLASH,
    HTTP_ABSOLUTE_URI_MAY_HOST,
    HTTP_ABSOLUTE_URI_HOST,
    HTTP_ABSOLUTE_URI_REST,
    HTTP_ABSOLUTE_URI_ERROR,
    HTTP_ABSOLUTE_URI_FIN
};

const struct parser_definition *
HTTP_absolute_uri_parser(void);

#endif