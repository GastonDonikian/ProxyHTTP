//
// Created by gaston on 15/6/21.
//

#ifndef PC_2021A_03_PROXYHANDLERHELPER_H
#define PC_2021A_03_PROXYHANDLERHELPER_H

#include "proxyUtils.h"
#include "proxyErrorCodes.h"
#include "logger.h"
#define CLIENT_NUMBER 512
#define IP_PROTOCOLS 2
#define BUFFER_INITIAL_SIZE 1024
#define FALSE 0
#define TRUE 1


void handleReadHttpFromUser(HTTP_connection * current , fd_set * currentfds , fd_set * current_write_fds , fd_set * readfds ,char * buffer , size_t buffer_size);

void handleWriteToClient(HTTP_connection *current , fd_set * currentfds , fd_set * current_write_fds , fd_set * writefds);

void handleReadFromServer(HTTP_connection * current , fd_set * currentfds, fd_set * current_write_fds , fd_set * readfds , char *  , size_t buffer_size);

void handleWriteToServer(HTTP_connection * current , fd_set * currentfds , fd_set * current_write_fds , fd_set * writefds);

void handleInformConnection(HTTP_connection * current , fd_set * currentfds , fd_set* current_write_fds , fd_set * writefds);

void DoHHandlingZone(HTTP_connection *current, fd_set *current_write_fds, fd_set *writefds, fd_set *current_read_fds,fd_set *readfds, char *buffer, int *maxSocket,size_t buffer_size);

void handleStayConnected(HTTP_connection *current ,fd_set * currentfds , fd_set * current_write_fds , fd_set * readfds , fd_set * writefds);

ssize_t sendFinalMessage(HTTP_connection * current);

void handleFinalMessage(HTTP_connection * current, fd_set * currentfds , fd_set * current_write_fds , fd_set * writefds);

ssize_t readAndParseResponse(int *socket, struct ctx *parser_context, fd_set *currentfds, char *buffer , size_t buffer_size);
ssize_t readAndParseRequest(int *socket, struct ctx *parser_context, fd_set *currentfds, char *buffer , size_t buffer_size);
void resetToNext( HTTP_connection * current);
void closeAll( HTTP_connection * current , fd_set * currentfds , fd_set *current_write_fds);
#endif //PC_2021A_03_PROXYHANDLERHELPER_H
