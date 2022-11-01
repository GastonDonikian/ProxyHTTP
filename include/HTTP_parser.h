#ifndef __HTTP_PARSER_H__
#define __HTTP_PARSER_H__

#include "parser.h"
#include "bufferUtils.h"
#include <stdbool.h>

#include "HTTP_message.h"

struct ctx * initCtx(HTTP_message * m);

void initHTTPparser();
void printBuffers(struct ctx * ctx);
void http_master(struct ctx *ctx, const uint8_t c);

void http_master_response(struct ctx *ctx, const uint8_t c);

struct ctx * initCtxResponse(HTTP_message *m);

bool hasFinished(struct ctx * ctx);

bool hasError(struct ctx * ctx);

char * getHost(struct ctx * ctx);

void freeCtx(struct ctx * ctx);

void closeHTTPparser();

size_t getHostLength(struct ctx * ctx);

HTTP_message * getMessage( struct ctx * ctx );

int parse_response( int socketFd , char * buffer , int buffer_size , struct ctx * message );

struct buffer * getAuth(struct ctx * ctx);

bool isConnect( struct ctx * ctx);


bool hasFinished(struct ctx * ctx);
#endif