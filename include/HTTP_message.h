#ifndef __HTTP_MESSAGE_H_
#define __HTTP_MESSAGE_H_
#include "bufferUtils.h"
#include <stdbool.h>
typedef struct HTTP_message //TODO PASARLO A HTTP_CONNECTION
{

    struct buffer* message; // para ir guardando la request original

    struct buffer* full_message;

    size_t sent;

    size_t content_length;

    bool send_connect;

}HTTP_message;

#endif