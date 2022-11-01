#ifndef _MCP_UTILS_
#define _MCP_UTILS_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include "args.h"
#include "shaUtils.h"
#include "logger.h"

#define MCP_CLIENTS 10
#define MCP_BUFFER_SIZE sizeof(mcp_package)

enum mcp_methods{
    GET_HISTORICAL_CONNECTIONS,
    GET_CONCURRENT_CONNECTIONS,
    GET_TOTAL_BYTES_SENT,
    SET_READ_BUFFER_SIZE,
};

enum mcp_codes{
    OK,
    AUTH_ERROR,
    INVALID_REQUEST_ERROR,
    SERVER_ERROR,
};

typedef struct mcp_package{
    uint8_t passphrase[32]; //Contraseña encodeada en SHA256
    uint8_t is_response;
    uint8_t type;
    uint64_t body;
}mcp_package;

int setupMCPSocket(struct httpargs * httpargs);

int handleMCPRequest(int mcpSocket); 

bool checkAuth(uint8_t * pass);


#endif