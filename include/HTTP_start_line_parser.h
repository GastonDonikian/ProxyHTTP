#ifndef _HTTP_FIRST_LINE_PARSER_H_
#define _HTTP_FIRST_LINE_PARSER_H_

#include"parser.h"

/**
 * Parser de la primera línea
 * Se encarga de encontrar el método, request target y versión 
**/

/** eventos del parser*/
enum HTTP_start_line_type {
    //First line: [METHOD] [REQUEST TARGET] [VERSION]
    HTTP_START_LINE_METHOD,
    HTTP_START_LINE_REQUEST_TARGET,
    HTTP_START_LINE_VERSION,
    HTTP_START_LINE_WS1,
    HTTP_START_LINE_WS2,
    


    //Termino con /r/n
    HTTP_START_LINE_FIN,
    HTTP_START_LINE_MAY_FIN,

    HTTP_START_LINE_ERROR,
};

//Métodos tradicionales de HTTP
enum HTTP_METHODS{
    CONNECT,
    OPTIONS,
	GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    TRACE,
    METHOD_COUNT
};

//Versiones de HTTP soportadas
enum HTTP_VERSIONS{
    ONE_POINT_CERO,
    ONE_POINT_ONE,
    TWO_POINT_CERO,
    VERSION_COUNT
};

const struct parser_definition *
HTTP_start_line_parser(void);


#endif