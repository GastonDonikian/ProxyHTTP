#include "include/shaUtils.h"



void sha256_digest(void *src, void *dest, size_t size){
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, src, size);
    SHA256_Final(dest, &ctx);
}

void printSHA(char * dest){
    for(int x = 0; x < SHA256_DIGEST_LENGTH; x++)
        printf("%02x",  dest[x]);
    putchar( '\n' );
}

int compareHash(void * h1, void  * h2){
    return memcmp(h1, h2, SHA256_DIGEST_LENGTH);
}

void getPassphrase(char * dest){
    int size = strlen(PASSPHRASE);
    sha256_digest(&PASSPHRASE, dest, size);
}

void getOtherPassphrase(char * dest){
    int size = strlen(PROXYPASSPHRASE);
    sha256_digest(&PROXYPASSPHRASE, dest, size);
}


bool checkAuth(uint8_t * pass){
    uint8_t hashedPass[SHA256_DIGEST_LENGTH];
    getOtherPassphrase(hashedPass);
    return memcmp(hashedPass, pass, SHA256_DIGEST_LENGTH) == 0;
}
