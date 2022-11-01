#ifndef __BUFFER_UTILS_H__
#define __BUFFER_UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct buffer {
    char *buffer;
    size_t len;     // longitud del buffer
    size_t from;    // desde donde falta escribir (apunta a la pos vacia)
};

#define MESSAGE_BUFFER_SIZE 1024
#define HEADER_BUFFER_SIZE 1024
#define HOST_BUFFER_SIZE 1024
#define CONTENT_LENGTH_BUFFER_SIZE 32

struct buffer * bufferInit(int initialSize);

int resizeBuffer(struct buffer * buf);

void writeBuffer(struct buffer * buf, char c);

void freeBuffer(struct buffer* buf);

char getChar(struct buffer * buf);

char getAndRemoveChar(struct buffer * buf);

void dumpBuffer(struct buffer * buf, int start);

void writeNBuffer(struct buffer * buf , char * input , size_t size );

void deleteStartBuffer(struct buffer * buf , size_t size );

void bufferReset( struct  buffer * buf);


#endif