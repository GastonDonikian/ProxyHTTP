//
// Created by gaston on 16/6/21.
//

#ifndef PC_2021A_03_BASE64DECODER_H
#define PC_2021A_03_BASE64DECODER_H
#include <stddef.h>
#endif //PC_2021A_03_BASE64DECODER_H

unsigned char * base64_decode(const unsigned char *src, size_t len,
                              size_t *out_len);