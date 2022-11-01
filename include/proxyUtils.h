#ifndef __PROXY_UTILS_H__
#define __PROXY_UTILS_H__

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "HTTP_parser.h"
#include "HTTP_connection.h"
#include "args.h"

extern struct httpargs *httpargs;    

int setupServerSocket(int, int, int, int, struct httpargs *httpargs);

int handleConnection(int, HTTP_connection []);

int setupRemoteSocket(struct serverAddress *, fd_set *, HTTP_connection *,int *);

int setupRemoteSocket6( HTTP_connection * connection , int * maxSocket);

void clearHTTPmessage(HTTP_message * httpMessage);

void handleConnectionClose(int *);

int acceptTCPConnection(int);

int checkIfIp( char * buff , struct dohContext * context,size_t length);

ssize_t handleWrite(int destinationSocket , HTTP_message * message);

void clear(struct buffer *);

ssize_t simpleRead(int fromSocket , HTTP_message * message );

void clearHTTPconnection(HTTP_connection *);

ssize_t informConnection( HTTP_connection *);

void saveAuthorization( struct ctx * ctx);

#endif