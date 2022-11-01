#ifndef _HTTP_connection_
#define _HTTP_connection_
#include "doh.h"
#include "HTTP_parser.h"
#include <stdbool.h>

enum HTTP_connection_type{
    HTTP_REQUEST,
    HTTP_RESPONSE
};

typedef struct HTTP_connection {

    int client_socket;
    //despues del dns
    int server_socket;

    //enum HTTP_connection_type type;

    HTTP_message request;

    HTTP_message response;

    struct ctx *client_context_parser;

    //faltaria una DNS context

    int doh_socket;

    struct dohContext *dohContext;

    struct ctx *doh_context_parser;

    HTTP_message doh_request;

    HTTP_message doh_response;

    bool connected;

    bool connection_informed;

    bool stay_connected;

    bool request_ready;

    size_t connection_sent;

    //esto es para la para de stay connected
    bool stay_connected_setted_up;

    size_t client_sent;

    size_t server_sent;

    bool client_read_closed;

    bool server_read_closed;

    bool client_write_closed;

    bool server_write_closed;

    struct buffer_circular *tunel_client;

    struct buffer_circular *tunel_server;

    bool send_final_message;

    char *(*final_message )(void);

    size_t final_message_sent;

    bool header_finished;

    bool tried_direct;
}HTTP_connection;
/*
fd_set currentfds; // lleva registro antes de que se destruya el readfds
fd_set readfds;
FD_ZERO(&currentfds);

fd_set current_write_fds;
fd_set writefds;
*/
#endif