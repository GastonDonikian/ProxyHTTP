#include "../../include/HTTP_master_parser.h"

const char *
HTTP_master_event(enum HTTP_master_type type) {
    /*const char *ret;
    switch(type) {
        case  HTTP_HEADER_FIELD_NAME:
            ret = "wait_fn()";
            break;
        case  HTTP_HEADER_COLON:
            ret = "wait_colon()";
            break;
        case  HTTP_HEADER_FIELD_VALUE:
            ret = "wait_fv()";
            break;
        case  HTTP_HEADER_MAY_HOST:
            ret = "wait_may_host()";
            break;
        case  HTTP_HEADER_COLON_HOST:
            ret = "wait_host_colon()";
            break;
        case  HTTP_HEADER_HOST_PATH:
            ret = "wait_host_path()";
            break;
        case  HTTP_HEADER_FIN:
            ret = "fin()";
            break;
        case  HTTP_HEADER_PATH:
            ret = "error()"
            break;              
    }*/
    return "not implemented";
}

enum {
    START_LINE,
    HEADER,
    HEADER_NEW_LINE,
    DATA,
    DATA_NEW_LINE,
    FIN,
    ERROR,
    START_LINE_CR,
    HEADER_CR,
    HEADER_CR_FIN,
    DATA_CR,
    DATA_CR_FIN
};



static void
start_line_byte(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_MASTER_START_LINE;
    ret->data[0] = c;
    ret->n       = 1;
}

static void
header_byte(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_MASTER_HEADER;
    ret->data[0] = c;
    ret->n       = 1;
}

static void
data_byte(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_MASTER_DATA;
    ret->data[0] = c;
    ret->n       = 1;
}

static void
fin(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_MASTER_FIN;
    ret->n       = 0;
}
static void
error(struct parser_event *ret, const uint8_t c) {
    ret->type    = HTTP_MASTER_ERROR;
    ret->n       = 0;
}

static const struct parser_state_transition ST_START_LINE[] =  {
    {.when = '\r',       .dest = START_LINE_CR,    .act1 = start_line_byte,},
    {.when = ANY,        .dest = START_LINE,         .act1 = start_line_byte,},
};

static const struct parser_state_transition ST_HEADER[] =  {
    {.when = '\r',       .dest = HEADER_CR,    .act1 =   header_byte,},
    {.when = ANY,       .dest = HEADER,                .act1 = header_byte,},
};
static const struct parser_state_transition ST_HEADER_NEW_LINE[] =  {
    {.when = '\r',       .dest = HEADER_CR_FIN,    .act1 = header_byte,},
    {.when = ANY,       .dest = HEADER,               .act1 = header_byte,},
};


static const struct parser_state_transition ST_DATA_NEW_LINE[] =  {
    {.when = '\r',      .dest = DATA_CR_FIN,       .act1 = data_byte,},
    {.when = ANY,       .dest = DATA,               .act1 = data_byte,},
};

static const struct parser_state_transition ST_DATA[] =  {
    {.when = '\r',      .dest = DATA_CR,       .act1 = data_byte,},
    {.when = ANY,       .dest = DATA,                .act1 = data_byte,},
};


static const struct parser_state_transition ST_FIN[]={
    {.when = ANY,       .dest = ERROR,     .act1 = error,},
};
static const struct parser_state_transition ST_ERROR[]={
    {.when = ANY,       .dest = ERROR,     .act1 = error,},
};


static const struct parser_state_transition ST_START_LINE_CR[]={
    {.when = '\r',      .dest = START_LINE_CR,    .act1 = start_line_byte,},
    {.when = '\n',      .dest = HEADER_NEW_LINE,    .act1 = start_line_byte,},
    {.when = ANY,       .dest = START_LINE,     .act1 = start_line_byte,},
};
static const struct parser_state_transition ST_HEADER_CR[]={
    {.when = '\r',      .dest = HEADER_CR,    .act1 = header_byte,},
    {.when = '\n',      .dest = HEADER_NEW_LINE,    .act1 = header_byte,},
    {.when = ANY,       .dest = HEADER,     .act1 = header_byte,},
};

static const struct parser_state_transition ST_HEADER_CR_FIN[]={
    {.when = '\r',      .dest = HEADER_CR,    .act1 = header_byte,},
    {.when = '\n',      .dest = DATA_NEW_LINE,    .act1 = header_byte,},
    {.when = ANY,       .dest = HEADER,     .act1 = header_byte,},
};

static const struct parser_state_transition ST_DATA_CR[]={
    {.when = '\r',      .dest = DATA_CR,    .act1 = data_byte,},
    {.when = '\n',      .dest = DATA_NEW_LINE,    .act1 = data_byte,},
    {.when = ANY,       .dest = DATA,     .act1 = data_byte,},
};

static const struct parser_state_transition ST_DATA_CR_FIN[]={
    {.when = '\r',      .dest = DATA_CR,    .act1 = data_byte,},
    {.when = '\n',      .dest = FIN,      .act1 = fin,},
    {.when = ANY,       .dest = DATA,     .act1 = data_byte,},
};

///////////////////////////////////////////////////////////////////////////////
// Declaración formal

static const struct parser_state_transition *states [] = {
    ST_START_LINE,
    ST_HEADER,
    ST_HEADER_NEW_LINE,
    ST_DATA,
    ST_DATA_NEW_LINE,
    ST_FIN,
    ST_ERROR,
    ST_START_LINE_CR,
    ST_HEADER_CR,
    ST_HEADER_CR_FIN,
    ST_DATA_CR,
    ST_DATA_CR_FIN
    
};

#define N(x) (sizeof(x)/sizeof((x)[0]))

static const size_t states_n [] = {
    N(ST_START_LINE),
    N(ST_HEADER),
    N(ST_HEADER_NEW_LINE),
    N(ST_DATA),
    N(ST_DATA_NEW_LINE),
    N(ST_FIN),
    N(ST_ERROR),
    N(ST_START_LINE_CR),
    N(ST_HEADER_CR),
    N(ST_HEADER_CR_FIN),
    N(ST_DATA_CR),
    N(ST_DATA_CR_FIN)
    
};

static struct parser_definition definition = {
    .states_count = N(states),
    .states       = states,
    .states_n     = states_n,
    .start_state  = START_LINE
            ,
};

const struct parser_definition *
HTTP_master_parser(void) {
    return &definition;
}
