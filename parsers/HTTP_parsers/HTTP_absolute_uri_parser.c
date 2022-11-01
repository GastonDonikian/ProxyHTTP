#include "../../include/HTTP_absolute_uri_parser.h"
#include <stdio.h>
#include "../../include/char_classes.h"
enum {
    SCHEME_FIRST,
    SCHEME,
    MAY_HOST1,
    MAY_HOST2,
    HOST,
    GUION,
    REST,
    FIN,
    ERROR
};

static void wait_host(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_ABSOLUTE_URI_MAY_HOST;
    ret->n = 0;
}

static void scheme(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_ABSOLUTE_URI_SCHEME;
    ret->n = 1;
    ret->data[0] = c;
}



static void byte_host(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_ABSOLUTE_URI_HOST;
    ret->data[0] = c;
    ret->n = 1;
}


/* static void wait_rest(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_ABSOLUTE_URI_REST;
    ret->n = 0;
} */



static void byte_rest(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_ABSOLUTE_URI_REST;
    ret->data[0] = c;
    ret->n = 1;
}

static void fin(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_ABSOLUTE_URI_FIN;
    ret->n = 0;
}


static void error(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_ABSOLUTE_URI_ERROR;
    ret->n = 0;
}


static const struct parser_state_transition ST_SCHEME_FIRST[] ={
    {.when = TOKEN_ALPHA,       .dest = SCHEME,      .act1 = scheme, },
    {.when = ANY,               .dest = ERROR,         .act1 = error, }
};

static const struct parser_state_transition ST_SCHEME[] = {
    {.when = SCHEME_TOKEN,       .dest = SCHEME,      .act1 = scheme, },
    {.when = ':',       .dest = MAY_HOST1,      .act1 = scheme, },
    {.when = ANY,       .dest = ERROR,         .act1 = error, }
};

static const struct parser_state_transition ST_MAY_HOST1[] = {
    {.when = '/',       .dest = MAY_HOST2,      .act1 = wait_host, },
    {.when = ANY,       .dest = ERROR,         .act1 = error, }
};


static const struct parser_state_transition ST_MAY_HOST2[] = {
    {.when = '/',       .dest = GUION,      .act1 = wait_host, },
    {.when = ANY,       .dest = ERROR,      .act1 = error, }
};


static const struct parser_state_transition ST_HOST[] = {
    {.when = '/',       .dest = REST,      .act1 = byte_rest, },
    {.when = '-',       .dest = GUION,      .act1 = byte_host, },
    {.when = '_',       .dest = ERROR,      .act1 = error, },
    {.when = ANY,       .dest = HOST,       .act1 = byte_host, }
};

static const struct parser_state_transition ST_GUION[] = {
    {.when = '-',       .dest = ERROR,      .act1 = error, },
    {.when = '_',       .dest = ERROR,      .act1 = error, },
    {.when = ANY,       .dest = HOST,        .act1 = byte_host, }
};


static const struct parser_state_transition ST_REST[] = {
    {.when = ' ',       .dest = FIN,        .act1 = fin, },
    {.when = ANY,       .dest = REST,      .act1 = byte_rest, },
};


static const struct parser_state_transition ST_FIN[] = {
    {.when = ANY,       .dest = ERROR,      .act1 = error, }
};

static const struct parser_state_transition ST_ERROR[] = {
    {.when = ANY,       .dest = ERROR,      .act1 = error, }
};





/////////////////////////////////////////////

static const struct parser_state_transition *states[] = {
    ST_SCHEME_FIRST,
    ST_SCHEME,
    ST_MAY_HOST1,
    ST_MAY_HOST2,
    ST_HOST,
    ST_GUION,
    ST_REST,
    ST_FIN,
    ST_ERROR
};


#define N(x) (sizeof(x)/sizeof((x)[0]))

static const size_t states_n[] = {
    N(ST_SCHEME_FIRST),
    N(ST_SCHEME),
    N(ST_MAY_HOST1),
    N(ST_MAY_HOST2),
    N(ST_HOST),
    N(ST_GUION),
    N(ST_REST),
    N(ST_FIN),
    N(ST_ERROR)
};




static struct parser_definition definition = {
    .states_count = N(states),
    .states       = states,
    .states_n     = states_n,
    .start_state  = SCHEME_FIRST,
};



const struct parser_definition *
HTTP_absolute_uri_parser(void) {
    return &definition;
}