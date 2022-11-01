#include "../include/bufferUtils.h"


#define CHUNK 50


struct buffer * bufferInit(int initialSize){
    struct buffer * buf = malloc(sizeof(struct buffer));
    struct buffer * aux = buf;
    if(buf != NULL){
        buf->buffer = malloc(initialSize);
        if(buf->buffer == NULL){
            free(aux);
            return NULL; //chequeo que se haya inicializado correctamente
        }

        memset(buf->buffer, 0, initialSize);
        buf->len = initialSize;
        buf->from = 0; //empiezo a escribir desde el principio del buffer.
    }

    return buf;
}


int resizeBuffer(struct buffer * buf){
    int oldLen = buf->len;
    buf->buffer = realloc(buf->buffer, oldLen + CHUNK);
    buf->len = oldLen + CHUNK;
    return buf->buffer == NULL? 0: -1; 
}

void writeBuffer(struct buffer * buf, char c){

    if(buf->len == buf->from){
        resizeBuffer(buf);
    }

    buf->buffer[buf->from++] = c;
}

void resizeBufferN( struct buffer * buf , size_t n){
    int oldLen = buf->len;
    buf->buffer = realloc(buf->buffer, oldLen + CHUNK * n);
    buf->len = oldLen + CHUNK * n;
}

void writeNBuffer(struct buffer * buf , char * input , size_t size ){
    if ( buf->from + size >= buf->len){
        size_t chunks = (buf->from + size - buf->len) / CHUNK;
        resizeBufferN( buf, chunks + 1);
    }
    memcpy(buf->buffer + buf->from , input , size );
    buf->from += size;
}



void freeBuffer(struct buffer * buf) {
    if (buf != 0) {
        free(buf->buffer);
        free(buf);
    }
}

char getChar(struct buffer * buf) {
    return buf->buffer[buf->from-1];
}


char getAndRemoveChar(struct buffer * buf){
    return buf->buffer[--buf->from];
}


void dumpBuffer(struct buffer * buf, int start){
    if( buf == 0 ) {
        printf("Buffer not initialized\n");
        return;
    }

    int from = buf->from;
    for(int i = start; i < buf->len; i++){
        if(i == from){
            printf("\n Hasta acá está escrito(el from : %ld). Ahora viene lo que hay en memoria sin usar \n" , buf->from);
        }
        printf("%c", buf->buffer[i]);
    }

}

void bufferReset( struct  buffer * buf) {
    if (buf != 0) {
        if (buf->from > 0) {
            memset(buf->buffer, 0, buf->len);
            buf->from = 0;
        }
    }

}
void bufferDeleteN(struct buffer * buf , size_t size){
        if ( buf -> from > size) {
            buf->from -= size;
            memset(buf->buffer + buf->from , 0 , size);

        }else{
            size_t old_from = buf->from;
            buf->from = 0 ;
            memset(buf->buffer , 0 , old_from);
        }

}


void deleteStartBuffer(struct buffer * buf , size_t size ){
    if( buf == 0 || size <= 0){
        return;
    }
    if ( size >= buf->from){
        bufferReset(buf);
        return;
    }
    size_t new_from = buf->from - size;
    for(int i = 0 ; i < new_from  ; i++){
        buf->buffer[i] = buf->buffer[i + size];
    }

    memset(buf->buffer + new_from , 0 , buf->len - new_from);
    buf->from = new_from;
}