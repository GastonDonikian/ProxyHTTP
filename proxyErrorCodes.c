#include "include/proxyErrorCodes.h"

char * badRequest(void){
//Esta la pense si falla el DoH y me da un badrequest o algo medio bizarro que me tira el cliente
    return "HTTP/1.1 400 Bad Request\r\n\r\n";
}

char * internalServerError(void){
    return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
}

char * badGateway(void){
    return "HTTP/1.1 502 Bad Gateway\r\n\r\n";
}

/*
 * No se me ocurre ningun otro pero agregarlo deberia ser bastante facilovich
 */