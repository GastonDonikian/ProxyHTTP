#include "../../include/HTTP_request_uri_parser.h"
#include <stdio.h>


enum {
    START,
    URI,
    IS_ASTERISK,
    ERROR,
    FIN
};

static void fin(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_REQUEST_URI_FIN;
    ret->n       = 0;
}
static void error(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_REQUEST_URI_ERROR;
    ret->n       = 0;
}

static void may_asterisk(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_REQUEST_URI_MAY_ASTERISK;
    ret->data[0] = c;
    ret->n = 1;
}

static void is_asterisk(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_REQUEST_URI_IS_ASTERISK;
    ret->data[0] = c;
    ret->n = 1;
}

static void may_abs(struct parser_event *ret, const uint8_t c){
    ret->type = HTTP_REQUEST_URI_MAY_ABSOLUTE;
    ret->data[0] = c;
    ret->n = 1;
}


static const struct parser_state_transition ST_START[] = {
    {.when = '/',       .dest=ERROR,            .act1 = error, },
    {.when = ' ',       .dest=ERROR,            .act1 = error, },
    {.when = '*',       .dest=IS_ASTERISK,      .act1 = may_asterisk, },
    {.when = ANY,       .dest=URI,              .act1 = may_abs, },
};

static const struct parser_state_transition ST_URI[] = {
    {.when = ' ',       .dest= FIN,            .act1 = fin, },
    {.when = ANY,       .dest= URI,              .act1 = may_abs},
};


static const struct parser_state_transition ST_IS_ASTERISK[] = {
    {.when = ' ',       .dest=FIN,              .act1 = is_asterisk, },
    {.when = ANY,       .dest = ERROR,          .act1 = error, },
};

static const struct parser_state_transition ST_FIN[] = {
    {.when = ANY,       .dest = ERROR,          .act1 = error, },
};

static const struct parser_state_transition ST_ERROR[] = {
    {.when = ANY,       .dest = ERROR,          .act1 = error, },
};


/////////////////////////////////////////////////////
static const struct parser_state_transition *states[] = {
    ST_START,
    ST_URI,
    ST_IS_ASTERISK,
    ST_ERROR,
    ST_FIN
};

#define N(x) (sizeof(x)/sizeof((x)[0]))

static const size_t states_n[] = {
    N(ST_START),
    N(ST_URI),
    N(ST_IS_ASTERISK),
    N(ST_ERROR),
    N(ST_FIN)

};


static struct parser_definition definition = {
    .states_count = N(states),
    .states       = states,
    .states_n     = states_n,
    .start_state  = START,
};

const struct parser_definition * HTTP_request_uri_parser(void) {
    return &definition;
}




