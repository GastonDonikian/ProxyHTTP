#include "../../include/HTTP_header_parser.h"
#include <stdio.h>
/* const char *
HTTP_header_event(enum HTTP_header_type type) {
    const char *ret;
    switch(type) {
        case  HTTP_HEADER_FIELD_NAME:
            ret = "byte_fn()";
            break;
        case  HTTP_HEADER_COLON:
            ret = "wait_colon()";
            break;
        case  HTTP_HEADER_FIELD_VALUE:
            ret = "byte_fv()";
            break;
        case  HTTP_HEADER_FIN:
            ret = "fin()";
            break;
        case  HTTP_HEADER_ERROR:
            ret = "error()";
            break;

    }
    return ret;
} */

enum {
    NEW_LINE,
    FIELD_NAME,
    COLON,
    FIELD_VALUE,
    FIN,
    ERROR,
    CR_FIELD_NAME,
    CR_FIELD_VALUE,
    CR_FIN,
    OWS,
    OWS_END
};


static void
byte_nl(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_HEADER_NEW_LINE;
    ret->data[0] = c;
    ret->n       = 1;
}

static void
byte_fv(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_HEADER_FIELD_VALUE;
    ret->data[0] = c;
    ret->n       = 1;
}

static void
wait_colon(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_HEADER_COLON;
    ret->n       = 0;
}

static void
byte_fn(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_HEADER_FIELD_NAME;
    ret->data[0] = c;
    ret->n       = 1;
}

static void
byte_cr_fn(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_HEADER_FIELD_NAME;
    ret->data[0] = '\r'; 
    ret->n       = 1;
}

static void
byte_cr_fv(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_HEADER_FIELD_VALUE;
    ret->data[0] = '\r'; 
    ret->n       = 1;
}

static void
wait_fv(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_HEADER_FIELD_VALUE;
    ret->n       = 0;
}

static void
wait_cr(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_HEADER_CR;
    ret->n       = 0;
}

static void
fin(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_HEADER_FIN;
    ret->n       = 0;
}
static void
error(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_HEADER_ERROR;
    ret->n       = 0;
}

static const struct parser_state_transition ST_NEW_LINE[] =  {
    {.when = ':',        .dest = ERROR,              .act1 = error,  },
    {.when = '\r',       .dest = CR_FIN,             .act1 = wait_cr,},
    {.when = ANY,        .dest = FIELD_NAME,         .act1 = byte_fn,},
};

static const struct parser_state_transition ST_FIELD_NAME[] =  {
    {.when = ':',       .dest = COLON,        .act1 = wait_colon,},
    {.when = '\r',      .dest = CR_FIELD_NAME,.act1 = wait_cr,   },
    {.when = ANY,       .dest = FIELD_NAME,   .act1 = byte_fn,   },
};

static const struct parser_state_transition ST_COLON[] =  {
   // {.when = ':',       .dest = ERROR,       .act1 = error,  },
    {.when = '\r',      .dest = CR_FIELD_VALUE,.act1 = wait_fv,  },
    {.when = ' ',        .dest = OWS,    .act1 = wait_fv,  },
    {.when = ANY,       .dest = FIELD_VALUE,   .act1 = byte_fv,  },
};

static const struct parser_state_transition ST_FIELD_VALUE[] =  {  
    //{.when = ':',        .dest = ERROR,          .act1 = error,  },
    {.when = '\r',       .dest = CR_FIELD_VALUE, .act1 = wait_fv,    },
    {.when = ANY,        .dest = FIELD_VALUE,    .act1 = byte_fv,  },
};

static const struct parser_state_transition ST_FIN[]={
    {.when = ANY,       .dest = ERROR,     .act1 = error,},
};

static const struct parser_state_transition ST_ERROR[]={
    {.when = ANY,       .dest = ERROR,     .act1 = error,},
};

static const struct parser_state_transition ST_CR_FIELD_NAME[]={
    {.when = '\r',      .dest = CR_FIELD_NAME,  .act1 = byte_cr_fn,                   },
    {.when = '\n',      .dest = ERROR,          .act1 = error,                        },
    {.when = ANY,       .dest = FIELD_NAME,     .act1 = byte_cr_fn, .act2 = byte_fv   },
};

static const struct parser_state_transition ST_CR_FIELD_VALUE[]={
    {.when = '\r',      .dest = CR_FIELD_VALUE,     .act1 = byte_cr_fv,                 },
    {.when = '\n',      .dest = NEW_LINE,           .act1 = byte_nl,                      },
    {.when = ANY,       .dest = FIELD_VALUE,        .act1 = byte_cr_fv, .act2 = byte_fv},
};


static const struct parser_state_transition ST_CR_FIN[]={
    {.when = '\r',      .dest = CR_FIELD_NAME ,     .act1 = byte_cr_fv,               },
    {.when = '\n',      .dest = FIN,                .act1 = fin,                      },
    {.when = ANY,       .dest = FIELD_VALUE,        .act1 = byte_cr_fv, .act2 = byte_fv },
};

static const struct parser_state_transition ST_OWS[]={
    {.when = '\r',      .dest = CR_FIELD_VALUE,.act1 = wait_fv,  },
    {.when = ' ',        .dest = OWS,    .act1 = wait_fv,  },
    {.when = ANY,       .dest = FIELD_VALUE,   .act1 = byte_fv,  },
};

///////////////////////////////////////////////////////////////////////////////
// Declaración formal

/*
enum {
    NEW_LINE,
    FIELD_NAME,
    COLON,
    FIELD_VALUE,
    FIN,
    ERROR,
    CR_FIELD_NAME,
    CR_FIELD_VALUE,
    CR_FIN
};
*/
static const struct parser_state_transition *states [] = {
    ST_NEW_LINE,
    ST_FIELD_NAME,
    ST_COLON,
    ST_FIELD_VALUE,
    ST_FIN,
    ST_ERROR,
    ST_CR_FIELD_NAME,
    ST_CR_FIELD_VALUE,
    ST_CR_FIN,
    ST_OWS 
};

#define N(x) (sizeof(x)/sizeof((x)[0]))

static const size_t states_n [] = {
    N(ST_NEW_LINE),
    N(ST_FIELD_NAME),
    N(ST_COLON),
    N(ST_FIELD_VALUE),
    N(ST_FIN),
    N(ST_ERROR),
    N(ST_CR_FIELD_NAME),
    N(ST_CR_FIELD_VALUE),
    N(ST_CR_FIN),
    N(ST_OWS)
     
};

static struct parser_definition definition = {
    .states_count = N(states),
    .states       = states,
    .states_n     = states_n,
    .start_state  = NEW_LINE,
};

const struct parser_definition *
HTTP_header_parser(void) {
    return &definition;
}
