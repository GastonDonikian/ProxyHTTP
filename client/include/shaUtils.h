#ifndef _SHA_UTILS_
#define _SHA_UTILS_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <stdbool.h>

#define PASSPHRASE "protos2c2021g3"

#define PROXYPASSPHRASE "soyelproxywololo"

void sha256_digest(void *src, void *dest, size_t size);

void shaPrint(void * dest);

int compareHash(void * h1, void  * h2);

void getPassphrase(char * dest);

bool checkAuth(uint8_t * pass);

void getOtherPassphrase(char * dest);

#endif

