#include "../../include/HTTP_start_line_parser.h"
#include <stdio.h>

enum {
    METHOD,
    REQUEST_TARGET,
    VERSION,
    MAY_FIN,
    CR,
    FIN,
    ERROR 
};

static void byte_method(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_START_LINE_METHOD;
    ret->data[0] = c;
    ret->n = 1;
}

static void wait_space1(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_START_LINE_WS1;
    ret->data[0] = c;
    ret->n = 1; 
}

static void wait_space2(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_START_LINE_WS2;
    ret->data[0] = c;
    ret->n = 1; 
}

static void byte_target(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_START_LINE_REQUEST_TARGET;
    ret->data[0] = c;
    ret->n = 1;
}

static void byte_version(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_START_LINE_VERSION;
    ret->data[0] = c;
    ret->n = 1;
}

static void wait_lf(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_START_LINE_MAY_FIN;
    ret->data[0] = c;
    ret->n = 1;
}

static void may_fin (struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_START_LINE_MAY_FIN;
    ret->n = 0;
}

static void fin(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_START_LINE_FIN;
    ret->data[0] = c;
    ret->n = 1;
}

static void error(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_START_LINE_ERROR;
    ret->n       = 0;
}



static const struct parser_state_transition ST_METHOD[] = {
    {.when = ' ',        .dest = REQUEST_TARGET,     .act1 = wait_space1, },
    {.when = ANY,        .dest = METHOD,             .act1 = byte_method, },
};

static const struct parser_state_transition ST_REQUEST_TARGET[] = {
    {.when = ' ',        .dest = VERSION,            .act1 = wait_space2, },
    {.when = ANY,        .dest = REQUEST_TARGET,     .act1 = byte_target, },
};


static const struct parser_state_transition ST_VERSION[] = {
    {.when = ' ',        .dest = MAY_FIN,        .act1 = wait_space2, },
    {.when = ANY,        .dest = VERSION,        .act1 = byte_version, },
};

static const struct parser_state_transition ST_MAY_FIN[] = {
    {.when = ' ',        .dest = MAY_FIN,        .act1 = may_fin,  },
    {.when = '\r',       .dest = CR,             .act1 = wait_lf, },
    {.when = ANY,        .dest = ERROR,          .act1 = error, },
};

static const struct parser_state_transition ST_CR[] = {
    {.when = '\n',       .dest = FIN,            .act1 = fin, },
    {.when = ANY,        .dest = ERROR,          .act1 = error, },
};


static const struct parser_state_transition ST_FIN[] = {
    {.when = ANY,        .dest = ERROR,        .act1 = error, },
};

static const struct parser_state_transition ST_ERROR[]={
    {.when = ANY,       .dest = ERROR,     .act1 = error,},
};



//////////////////////////////////////////////////

static const struct parser_state_transition *states[] = {
    ST_METHOD,
    ST_REQUEST_TARGET,
    ST_VERSION,
    ST_MAY_FIN,
    ST_FIN,
    ST_CR,
    ST_ERROR
};

#define N(x) (sizeof(x)/sizeof((x)[0]))

static const size_t states_n[] = {
    N(ST_METHOD),
    N(ST_REQUEST_TARGET),
    N(ST_VERSION),
    N(ST_MAY_FIN),
    N(ST_FIN),
    N(ST_CR),
    N(ST_ERROR)
};

static struct parser_definition definition = {
        .states_count = N(states),
        .states       = states,
        .states_n     = states_n,
        .start_state  = METHOD,
};

const struct parser_definition *
HTTP_start_line_parser(void) {
    return &definition;
}


