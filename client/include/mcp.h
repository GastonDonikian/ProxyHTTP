#ifndef _MCP_
#define _MCP_

#include <stdint.h>
#include <stddef.h>

uint64_t getHistoricalConnections();

uint64_t getConcurrentConnections();

uint64_t getTotalBytesSent();

uint8_t setReadBufferSize(char* buffSize);



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

#endif