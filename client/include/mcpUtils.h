#ifndef _MCP_UTILS_
#define _MCP_UTILS_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include "mcp.h"
#include "shaUtils.h"

#define NAME_BUFFER_SIZE 32
#define DESC_BUFFER_SIZE 1024
#define COMMAND_BUFFER_SIZE 1024


    struct mcpargs{

        char *          addr; // addr del cliente
        char *          port; // puerto del cliente
    };

    typedef struct get_command{
        char name[NAME_BUFFER_SIZE];
        char desc[DESC_BUFFER_SIZE];
        uint64_t (*command)();
    }get_command;

    typedef struct set_command{
        char name[NAME_BUFFER_SIZE];
        char desc[DESC_BUFFER_SIZE];
        uint8_t (*command)(char* arg);
    }set_command;

        void parse_args(const int argc, char **argv, struct mcpargs *args);

    //Arma un socket IPv6 para comunicarse con el proxy
    int setupRemoteSocket(struct mcpargs *mcp_args, struct sockaddr_in *servaddr);

    //Informa sobre las funciones del cliente MCP
    void help();

    //De existir, ejecuta el comando
    int executeCommand(char *command_name, char *arg);
#endif