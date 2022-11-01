#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>

#include "../../include/parser.h"
#include "../../include/HTTP_parser.h"
#include "../../include/HTTP_header_parser.h"
#include "../../include/HTTP_master_parser.h"
#include "../../include/HTTP_start_line_parser.h"
#include "../../include/HTTP_absolute_uri_parser.h"
#include "../../include/HTTP_request_uri_parser.h"
#include "../../include/bufferUtils.h"
#include "../../include/parser_utils.h"
#include "../../include/char_classes.h"

#define MESSAGE_BUFFER_SIZE 1024
#define HEADER_BUFFER_SIZE 1024
#define HOST_BUFFER_SIZE 1024
#define CONTENT_LENGTH_BUFFER_SIZE 32


#define CR 0xD

#define N(x) (sizeof(x)/sizeof((x)[0]))

static char * METHOD_NAMES[] = {  "CONNECT", "OPTIONS", "GET" , "HEAD" , "POST" , "PUT", "DELETE" , "TRACE" };
static char * VERSION_NAMES[] = {   "HTTP/1.0", "HTTP/1.1", "HTTP/2.0"  };



//aclaración: el contexto mantiene muchos parsers, pero la gran mayoría son dedicados a hacer comparaciones de string

/* mantiene el estado durante el parseo */
struct ctx {
    //el mensaje relacionado al contexto
    HTTP_message * message;

    //el tamaño de body leido
    size_t data_length;

/* los distintos subparsers relacionados */
    //parser en 3 partes principales del mensaje, start-line , header y data
    struct parser* HTTP_master;

    //Parser de la primera línea
    struct parser* HTTP_start_line;
    //Subparsers de la primera linea
		//Parsers para los métodos tradicionales
		struct parser** HTTP_methods;

		//Parsers para las versiones de HTTP
		struct parser** HTTP_versions;

		//Parser para el request target
        struct parser* HTTP_request_target;

        //Parser para el request target en forma absoluta
        struct parser* HTTP_request_abs;

    /* parseador de linea a linea de HTTP_header */
        struct parser* HTTP_header;

            //parsers dedicados a la deteccion de headers
            struct parser* HTTP_host;

            struct parser* HTTP_content_length;

            struct parser* HTTP_connection;

            struct parser* HTTP_auth;


    //Booleans relacionados a los métodos
		bool options_method_used; 

        bool connect_method_used;

		//Booleans relacionados al request target

		bool abs_form_used;

		bool asterisk_form_used;

		bool uri_is_valid;

		//Booleans relacionados a las versiones

		bool valid_version_used;

		bool uri_has_host;
		//Booleans relacionados a los métodos

		bool valid_method_used;

//estado de los parsers de comparacion en los headers
        enum string_cmp_event_types host_header_status;

        enum string_cmp_event_types content_length_header_status;
    
        enum string_cmp_event_types connection_header_status;

        enum string_cmp_event_types auth_header_status;

//zona de buffers, aca se guarda la información que es de interés
    //buffers de primera linea
        struct buffer* method;

		struct buffer* uri_host;

        struct buffer* uri_path_query_fragment;
    //buffers de headers
        struct buffer* auth;
        //buffers relacionados a los headers
        struct buffer* host;

        struct buffer* content_length;

    //booleans relacionados a los headers
        bool host_found;

        bool content_length_found;
    
        bool connection_found;

        bool connection_avoid;

        bool auth_found;

        bool error;

        bool has_finished;

        int host_count;

        int method_number;

        ssize_t content_length_num;

};


static bool TRUE = true;
static bool FALSE = false;




//parsea y guarda los headers que son necesarios
//si me pasaron en la primera línea un host, este obtiene una mayor precedencia
//   When a proxy receives a request with an absolute-form of
//   request-target, the proxy MUST ignore the received Host header field
//   (if any) and instead replace it with the host information of the
//   request-target.  A proxy that forwards such a request MUST generate a
//   new Host field-value based on the received request-target rather than
//   forward the received Host field-value. RFC 7230 #5.4
static void
http_header(struct ctx *ctx, const uint8_t c) {
    const struct parser_event* e = parser_feed(ctx->HTTP_header, c);
    //aca se guaradar auxiliarmente los eventos de strc,
    const struct parser_event* he;
    const struct parser_event* cle;
    const struct parser_event* ce;
    const struct parser_event* au;
    do {
        switch(e->type) {
            case HTTP_HEADER_ERROR:
                    ctx->error=true;
                break;
            case HTTP_HEADER_NEW_LINE:

                if( ctx->host_header_status == STRING_CMP_EQ){
                    ctx->host_count += 1;
                    if(ctx->host_count > 1 ){
                        ctx->error = true;
                    }
                }
                //si leí host y no tengo
                if(ctx->host->from > 0 ){
                    if(ctx->uri_has_host && !ctx->host_found  ) {
                        //zona UNTESTED
                        writeNBuffer(ctx->message->full_message , ctx->uri_host->buffer , ctx->uri_host->from);
                        bufferReset(ctx->host);
                        writeNBuffer(ctx->host , ctx->uri_host->buffer , ctx->uri_host->from);
                    }
                        ctx-> host_found = TRUE;
                }

                if(ctx->content_length->from > 0 ){
                    ctx-> content_length_found = TRUE;
                }

                if(ctx->auth->from > 0 ){
                    ctx->auth_found = TRUE;
                }
                if( ctx->connection_found == FALSE) {
                    parser_reset(ctx->HTTP_connection);
                }
                ctx->connection_avoid = FALSE;

                writeBuffer(ctx->message->full_message , '\r');
                writeBuffer(ctx->message->full_message , '\n');

                parser_reset(ctx->HTTP_auth);
                parser_reset(ctx->HTTP_host);
                parser_reset(ctx->HTTP_content_length);

                break;
            case HTTP_HEADER_FIELD_NAME:
                for( int i = 0 ; i < e->n ; i++){
                    //hago la coomparacion de la proxima letra en los headers
                    he = parser_feed(ctx->HTTP_host , e->data[i]);
                    cle = parser_feed(ctx->HTTP_content_length , e->data[i]);
                    ce = parser_feed(ctx->HTTP_connection , e->data[i]);
                    au = parser_feed(ctx->HTTP_auth , e->data[i]);
                    //me guardo el estado en el que quedaron ( para ver si es EQUALS )
                    ctx->content_length_header_status = cle->type;
                    ctx->host_header_status= he->type;
                    ctx->connection_header_status= ce ->type;
                    ctx->auth_header_status = au -> type;

                    writeBuffer(ctx->message->full_message, e->data[i]);
                }
                break;
            case HTTP_HEADER_COLON:
                //Si lei un header de Connection, tengo que borrarlo
                    if ( ctx->connection_found == FALSE && ctx->connection_header_status == STRING_CMP_EQ ){
                        ctx->connection_found = TRUE;
                        ctx->connection_avoid = TRUE;
                        ctx->message->full_message->from -= 12;
                        if(ctx->message->full_message->from < 0  || ctx->message->full_message->from > ctx->message->full_message->len){
                                ctx->message->full_message->from= 0;
                        }
                        memset(ctx->message->full_message->buffer + ctx->message->full_message->from , 0 , 12);
                    }else{
                        ctx->connection_avoid = FALSE;
                    }
                    if ( ctx->connection_avoid != TRUE){
                            writeBuffer(ctx->message->full_message , ':');
                    }
                break;

            case HTTP_HEADER_FIELD_VALUE:
                //checkeo si A -> el parser dio == STRING_CMP_EQ
                //B-> si ya lo encontré el header antes ( está repetido)
                // en el de host se podria da

                if( ctx->host_header_status == STRING_CMP_EQ && !ctx->host_found ){
                     for( int i = 0 ; i < e->n ; i++){
                        writeBuffer(ctx->host , e->data[i]);
                     }
                }else if ( ctx->content_length_header_status == STRING_CMP_EQ && !ctx->content_length_found){
                      for( int i = 0 ; i < e->n ; i++){
                        writeBuffer(ctx->content_length , e->data[i]);
                     }
                }else if(ctx->auth_header_status == STRING_CMP_EQ && !ctx->auth_found  ){
                    for(int i = 0 ; i<e->n ; i++) {
                        writeBuffer(ctx->auth, e->data[i]);
                    }
                }
                if ( ctx->connection_avoid != TRUE ){
                    //SI ME ESCRIBIERON HOST EN LA URI Y EN UN HEADER
                    //DEBE TOMAR PRECEDENCIA EL DE LA URI
                    //      hay uri                             estoy en un header de uri         y es la primera vez que paso por aca
                    if (!( ctx->uri_has_host && ctx->host_header_status == STRING_CMP_EQ && !ctx->host_found )){
                        for( int i = 0 ; i < e->n ; i++){
                            writeBuffer(ctx->message->full_message , e->data[i]);
                        }
                    }
                }
                break;
            case HTTP_HEADER_FIN:

                writeBuffer(ctx->message->full_message , '\r');
                writeBuffer(ctx->message->full_message , '\n');
            break;
        }
        e = e->next;
    } while (e != NULL);
}

static void http_abs_parser(struct ctx * ctx , uint8_t c ){

    const struct parser_event* e = parser_feed(ctx->HTTP_request_abs, c);
    do{
        switch (e->type)
        {
            case HTTP_ABSOLUTE_URI_ERROR:
                ctx->abs_form_used = FALSE;
				ctx->uri_is_valid = FALSE;
				ctx->error = TRUE;
            break;

            case HTTP_ABSOLUTE_URI_HOST: //estoy parseando el host

				//Voy guardando mi host en el buffer
				for(int i = 0 ; i < e->n ; i++) {
                    writeBuffer(ctx->uri_host, e->data[i]);
				}
			break;

			//estoy leyendo la parte CON PATH y QUERY
			case HTTP_ABSOLUTE_URI_REST:
				//Guardo el resto
				for(int i = 0 ; i < e->n ; i++) {
                    writeBuffer(ctx->uri_path_query_fragment, e->data[i]);
				}
			break;
            case HTTP_ABSOLUTE_URI_FIN:
               if ( ctx->uri_path_query_fragment-> from == 0 ){
                    writeBuffer(ctx->uri_path_query_fragment, '/');
                }
                if ( ctx->uri_host-> from > 0 ){
                    ctx->uri_has_host = TRUE;
                }
				ctx->abs_form_used = TRUE;
                ctx->uri_is_valid = TRUE;
			break;
        
            }
            e = e->next;
    }
    while(e != NULL);
}
static void http_request_target(struct ctx *ctx , uint8_t c){

    const struct parser_event* e = parser_feed(ctx->HTTP_request_target, c);
	//const struct parser_event* abs_parser_event;

    do{
        switch (e->type)
        {
            case HTTP_REQUEST_URI_ERROR:
                ctx->error = true;
				ctx->uri_is_valid = FALSE;
            break;
            case HTTP_REQUEST_URI_MAY_ABSOLUTE:
				for(int i = 0 ; i < e->n ; i++){
                    writeBuffer(ctx->message->full_message , e->data[i]);
					http_abs_parser( ctx , e->data[i]);
				}
            break;
            
            case HTTP_REQUEST_URI_IS_ASTERISK:
				if(ctx->options_method_used == TRUE)
					ctx->uri_is_valid = TRUE;
                writeBuffer(ctx->message->full_message , '*');
                ctx->asterisk_form_used = TRUE;
                writeBuffer(ctx->uri_path_query_fragment, '*');
            break;
			
            case HTTP_REQUEST_URI_FIN:
                // el espacio desde may abs igual me lleva a fin
                    http_abs_parser( ctx , ' ');
            break;
        }
        e = e->next;
    }
    while(e != NULL);

}

static void http_start_line(struct ctx *ctx, uint8_t c){

    const struct parser_event* e = parser_feed(ctx->HTTP_start_line, c);

    //Similar al uso en los headers, son parser de comparación de strings
	const struct  parser_event* method_events[METHOD_COUNT];
	const struct parser_event* version_events[VERSION_COUNT];

    do{
        switch(e->type){
            case HTTP_START_LINE_METHOD: //when Any -> me quedo aca, when ' ' voy a start_line_request_target.
                if( ctx->method->from > 10 ){
                    ctx->error = true;
                }
                //Examino el método a ver si usé OPTIONS o CONNECT
                for(int i=0 ; i < METHOD_COUNT ; i++){
                        for(int j = 0 ; j < e-> n ; j++) {
                            //Meto la data en los parsers de métodos
                            method_events[i] = parser_feed(ctx->HTTP_methods[i], e->data[j]);
                        }//Chequeo si matcheé con alguno de los métodos "especiales"
                    if ( e->n > 0 ) {
                        if ( method_events[i]->type == STRING_CMP_EQ ) {
                            ctx->method_number = i;
                            if (i == OPTIONS ) {
                                ctx->options_method_used = TRUE;
                            } else if (i == CONNECT ) {
                                ctx->connect_method_used = TRUE;

                            }
                        }
                    }
                }
                for( int i = 0 ; i < e->n ; i++) {
                    writeBuffer(ctx->message->full_message, e->data[i]);
                    //Lo guardo en el buffer para el método
                    writeBuffer(ctx->method, e->data[i]);
                }
                break;
            case HTTP_START_LINE_REQUEST_TARGET: 
                if( ctx->uri_host->from > 8000){
                    ctx->error = true;
                }
				if(ctx->connect_method_used == TRUE){//Si estoy usando connect, el RFC me dice que va a mandar la authority
					for(int i = 0 ; i < e->n ; i++) {
                        writeBuffer(ctx->uri_host, e->data[i]);

                        writeBuffer(ctx->message->full_message , e->data[i]);

                    }
				}	

				else{
					//Quiero ver qué tipo de request target form es, y guardarme el host en un buffer auxiliar
					for(int i = 0; i < e->n ; i++){
						http_request_target(ctx, e->data[i]);
					}
				}			
		

            break;
            case HTTP_START_LINE_VERSION:
                for(int i = 0 ; i < VERSION_COUNT ; i++) {
                    for( int j = 0 ; j < e->n ; j++) {
                        version_events[i] = parser_feed(ctx->HTTP_versions[i], e->data[j]);
                    }
					//Veo si usó una versión válida
					if ( e->n  > 0 ) {
                        if (version_events[i]->type == STRING_CMP_EQ) {
                            ctx->valid_version_used = TRUE;

                        }
                    }
				}
                for(int i = 0 ; i < e->n ;i++) {
                    writeBuffer(ctx->message->full_message, e->data[i]);
                }
            break;
            case HTTP_START_LINE_WS1:
                writeBuffer(ctx->message->full_message , ' ');
                break;

            case  HTTP_START_LINE_WS2:
                if ( ctx->connect_method_used == FALSE){
                    http_request_target(ctx , ' ' );
                }
                writeBuffer(ctx->message->full_message , ' ');
                break;
			case HTTP_START_LINE_FIN:

            if(ctx->uri_is_valid == FALSE || ctx->valid_version_used == FALSE){
                ctx->error = true;
            }else if(ctx->abs_form_used == TRUE && ctx->options_method_used == TRUE){
                ctx->abs_form_used = FALSE;
                ctx->asterisk_form_used = TRUE;
                writeBuffer(ctx->uri_path_query_fragment, '*');
			}else if(ctx->connect_method_used == FALSE && ctx->options_method_used == FALSE && ctx->abs_form_used == FALSE){
				ctx->error = true;
			}
			break;
        }
        e = e->next;
    }
    while(e != NULL);
}

//PARSER DE REQUEST, MENOS PERMISIVO
void http_master(struct ctx *ctx, const uint8_t c) {
    const struct parser_event* e = parser_feed(ctx->HTTP_master, c);

    if ( ctx->error ){
        return;
    }
    do {
        //Rfc 7230: It is RECOMMENDED that all HTTP senders and recipients
        //   support, at a minimum, request-line lengths of 8000 octets. #section-3.1.1

        // A server that receives a request header field, or set of fields,
        //   larger than it wishes to process MUST respond with an appropriate 4xx
        //   (Client Error) status code.  Ignoring such header fields would
        //   increase the server's vulnerability to request smuggling attacks
        //   (Section 9.5).
        switch (e->type) {
            case HTTP_MASTER_START_LINE:
                for(int i = 0 ; i < e->n ; i++){
                    http_start_line(ctx, e->data[i]);
                }
                //parseo de la primera linea
                break;
            case HTTP_MASTER_HEADER:
                for(int i = 0 ; i < e->n ; i++){
                    http_header(ctx , e->data[i]);
                }
                break;
            case HTTP_MASTER_DATA:
                ctx->has_finished = true;
                for(int i = 0 ; i < e->n ; i++) {
                    ctx->data_length += 1;
                    writeBuffer(ctx->message->full_message, e->data[i]); //le escribe directo al buffer
                    writeBuffer(ctx->message->message, e->data[i]);
                }
                break;
            case HTTP_MASTER_FIN:
                ctx->has_finished = true;
                break;
        }
        e = e->next;
    } while (e != NULL);
}


void http_master_response(struct ctx *ctx, const uint8_t c) {
    const struct parser_event* e = parser_feed(ctx->HTTP_master, c);
    if( ctx->error ) {
        return;
    }
    do {
        //debug("0. multi", pop3_multi_event,  e);

        switch (e->type) {
            case HTTP_MASTER_START_LINE:

                for(int i = 0 ; i < e->n ; i++){
                    writeBuffer(ctx->message->full_message , e->data[i]);
                }
                //parseo de la primera linea

                break;
            case HTTP_MASTER_HEADER:
                for(int i = 0 ; i < e->n ; i++){
                    http_header(ctx , e->data[i]);
                }
                break;
            case HTTP_MASTER_DATA:
                ctx->has_finished = true;
                for(int i = 0 ; i < e->n ; i++) {
                    ctx->data_length += 1;
                    writeBuffer(ctx->message->full_message, e->data[i]);
                    writeBuffer(ctx->message->message, e->data[i]);
                }
                break;
            case HTTP_MASTER_FIN:
                parser_reset(ctx->HTTP_master);
                break;
        }
        e = e->next;
    } while (e != NULL);
}

struct parser_definition host_head_parser_def;
struct parser_definition content_length_head_parser_def;
struct parser_definition connection_parser_def;
struct parser_definition authorization_parser_def;

struct parser_definition method_parser_defs[METHOD_COUNT];
struct parser_definition version_parser_defs[VERSION_COUNT];

const unsigned int* no_class;
const unsigned int* char_class;

void initHTTPparser(){
    const struct parser_definition aux_host_head_parser_def = parser_utils_strcmpi("host");
    memcpy( &host_head_parser_def , &aux_host_head_parser_def , sizeof (struct  parser_definition));
    const struct parser_definition aux_content_length_head_parser_def = parser_utils_strcmpi("content-length");
    memcpy( &content_length_head_parser_def , &aux_content_length_head_parser_def , sizeof (struct parser_definition));
    const struct parser_definition aux_connection_parser_def = parser_utils_strcmpi("connection");
    memcpy( &connection_parser_def , &aux_connection_parser_def , sizeof (struct parser_definition));
    const struct parser_definition aux_authorization_parser_def = parser_utils_strcmpi("authorization");
    memcpy( &authorization_parser_def , &aux_authorization_parser_def , sizeof(struct  parser_definition));


    no_class = parser_no_classes();
    char_class = init_char_class();

    for(int i = 0; i < METHOD_COUNT ; i++){
        const struct parser_definition aux_def = parser_utils_strcmps(METHOD_NAMES[i]);
        memcpy(&(method_parser_defs[i]), &aux_def, sizeof(aux_def));
    }
    //Defino los parsers de versiones
    for(int i = 0; i < VERSION_COUNT ; i++){
        const struct parser_definition aux_def = parser_utils_strcmps(VERSION_NAMES[i]);
        memcpy(&(version_parser_defs[i]), &aux_def, sizeof(aux_def));
    }
}


void closeHTTPparser(){
    parser_utils_strcmpi_destroy(&host_head_parser_def);
    parser_utils_strcmpi_destroy(&content_length_head_parser_def);
    parser_utils_strcmpi_destroy(&connection_parser_def);
    parser_utils_strcmpi_destroy(&authorization_parser_def);

    for(int i = 0; i < METHOD_COUNT ; i++){
        parser_utils_strcmps_destroy(&(method_parser_defs[i]));
    }
    //Defino los parsers de versiones
    for(int i = 0; i < VERSION_COUNT ; i++){
        parser_utils_strcmps_destroy(&(version_parser_defs[i]));
    }
}

struct ctx * initCtxResponse(HTTP_message *m){
//Defino los parsers de métodos

    if ( m-> message == 0 ) {
        m->message = bufferInit(MESSAGE_BUFFER_SIZE);
    }
    if( m->full_message == 0) {
        m->full_message = bufferInit(MESSAGE_BUFFER_SIZE);
    }

    struct buffer * host_buffer = bufferInit(sizeof(char) * HOST_BUFFER_SIZE);
    struct buffer * content_length_buffer = bufferInit(sizeof(char) * CONTENT_LENGTH_BUFFER_SIZE);
    struct buffer * auth_buffer = bufferInit(sizeof(char) * HOST_BUFFER_SIZE);

    struct ctx * ret = malloc ( sizeof(struct ctx) );
    struct ctx ctx = {
            .HTTP_master  = parser_init(no_class, HTTP_master_parser()),
            .HTTP_header  = parser_init(no_class, HTTP_header_parser()),
            .HTTP_auth    = parser_init(no_class, &authorization_parser_def),
            //parsers para headers
            .HTTP_host    = parser_init(no_class, &host_head_parser_def),
            .HTTP_content_length = parser_init(no_class , &content_length_head_parser_def),
            .HTTP_connection = parser_init(no_class, &connection_parser_def),
            .connection_found = FALSE,
            .connection_avoid = FALSE,
            .host         = host_buffer,
            .auth         = auth_buffer,
            .host_found   = FALSE,
            .has_finished = TRUE,
            .error = FALSE,
            .host_count = 0,
            .content_length_found = FALSE,
            .content_length = content_length_buffer,
            .host_header_status   = STRING_CMP_NEQ,
            .content_length_header_status = STRING_CMP_NEQ,
            .data_length = 0,
            .message = m,
            .uri_has_host = FALSE,
            .content_length_num = -1,
    };
    (*ret) = ctx;
    return ret;
}
void printBuffers(struct ctx * ctx) {
    printf("BUFFERS DE CONTEXTO: \n");
    printf("message: body\n");
    dumpBuffer(ctx->message->message , 0);
    printf("message: headers\n");
    dumpBuffer(ctx->message->full_message , 0);


    printf("En el buffer de uri_path_query_fragment:\n");
        dumpBuffer(ctx->uri_path_query_fragment, 0 );
    printf("\n fin\n");
    printf("En el buffer de uri_host:\n");
    dumpBuffer(ctx->uri_host , 0);
    printf("\n fin\n");
    printf("En el buffer de method:\n");
    dumpBuffer(ctx->method , 0);
    printf("\n fin\n");
    printf("En el buffer de host:\n");
    dumpBuffer(ctx->host , 0);
    printf("\n fin\n");
    printf("En el buffer de content-length:\n");
    dumpBuffer(ctx->content_length , 0);
    printf("\n fin\n");


}
struct ctx * initCtx(HTTP_message * m){

    //Inicio los parsers de métodos
    struct parser ** method_parsers = (struct parser **)malloc(sizeof (struct parser *) * METHOD_COUNT);
    for(int i = 0; i < METHOD_COUNT ; i++){
        method_parsers[i] = parser_init(no_class, &(method_parser_defs[i]));
    }

    //Inicio los parsers de versiones
    struct parser ** version_parsers = (struct parser **) malloc(sizeof (struct parser *) * VERSION_COUNT);
    for(int i = 0; i < VERSION_COUNT ; i++){
        version_parsers[i] = parser_init(no_class, &(version_parser_defs[i]));
    }

    if ( m-> message == 0 ) {
        m->message = bufferInit(MESSAGE_BUFFER_SIZE);
    }
    if( m->full_message == 0) {
        m->full_message = bufferInit(MESSAGE_BUFFER_SIZE);
    }

    struct buffer * host_buffer = bufferInit(sizeof(char) * HOST_BUFFER_SIZE);
    struct buffer * content_length_buffer = bufferInit(sizeof(char) * CONTENT_LENGTH_BUFFER_SIZE);
    struct buffer * method_buffer = bufferInit(sizeof(char) * CONTENT_LENGTH_BUFFER_SIZE);
    struct buffer * uri_host_buffer = bufferInit(sizeof(char) * HOST_BUFFER_SIZE);
    struct buffer * uri_path_query_fragment_buffer = bufferInit(sizeof(char) * HOST_BUFFER_SIZE);
    struct buffer * auth_buffer = bufferInit(sizeof(char) * HOST_BUFFER_SIZE);

    struct ctx * ret = malloc ( sizeof(struct ctx) );
    struct ctx ctx = {
            .HTTP_master  = parser_init(no_class, HTTP_master_parser()),
            .HTTP_header  = parser_init(no_class, HTTP_header_parser()),
            //parsers para headers
            .HTTP_host    = parser_init(no_class, &host_head_parser_def),
            .HTTP_content_length = parser_init(no_class , &content_length_head_parser_def),
            .HTTP_connection = parser_init(no_class, &connection_parser_def),
            .HTTP_auth    = parser_init(no_class, &authorization_parser_def),
            //parsers para checkeos en la primera linea
            .HTTP_methods = method_parsers,
            .HTTP_versions = version_parsers,
            .HTTP_request_target = parser_init(char_class, HTTP_request_uri_parser()),
            .HTTP_request_abs = parser_init(char_class, HTTP_absolute_uri_parser()),
            .HTTP_start_line = parser_init(no_class, HTTP_start_line_parser()),
            .connection_found = FALSE,
            .connection_avoid = FALSE,
            .has_finished = FALSE,
            .uri_host = uri_host_buffer,
            .uri_path_query_fragment = uri_path_query_fragment_buffer,
            .host         = host_buffer,
            .content_length= content_length_buffer,
            .method = method_buffer,
            .auth = auth_buffer,
            .host_found   = FALSE,
            .content_length_found = FALSE,
            .error = FALSE,
            .host_count = 0,
            //Métodos "especiales" arrancan sin ser usados
            .connect_method_used = FALSE,
            .options_method_used = FALSE,

            //Propiedades del request target
            .asterisk_form_used = FALSE,
            .abs_form_used = FALSE,
            .uri_is_valid = FALSE,

            //Verión válida a confirmar
            .valid_version_used = FALSE,

            .host_header_status   = STRING_CMP_NEQ,
            .content_length_header_status = STRING_CMP_NEQ,
            .data_length = 0,
            .message = m,
            .uri_has_host = FALSE
    };
    (*ret) = ctx;
    return ret;
}


/*
 * Funciones dedicadas a proveer información en cuanto al estado del contexto en gral
 *
 *
 */
bool isConnect( struct ctx * ctx){
    return ctx->connect_method_used;
}

struct buffer * getAuth(struct ctx * ctx){
    return ctx->auth;
}

HTTP_message * getMessage( struct ctx * ctx ){
    if ( ctx->message == 0 ){
        return 0;
    }
    ctx->message->content_length = ctx->data_length;
    return ctx->message;
}


char * getHost(struct ctx * ctx){
    if (ctx == 0 ){
        return 0;
    }
    if ( ctx->host_found == TRUE) {
        return ctx->host->buffer;
    }
    return 0;
}

size_t getHostLength(struct ctx * ctx){
    if(ctx == 0)
        return 0;
    if(ctx->host_found == TRUE)
        return ctx->host->from;
    return 0;
}


bool hasError( struct ctx * ctx ){
    return ctx->error;
}

bool hasFinished(struct ctx * ctx ){
    return ctx->has_finished;
}
void freeCtx( struct ctx * ctx){
    if( ctx == 0 ){
        return;
    }

//    printBuffers(ctx);
    freeBuffer(ctx->message->message);
    freeBuffer(ctx->message->full_message);
    ctx->message->message = ctx->message->full_message = 0;
    freeBuffer(ctx->host);
    freeBuffer(ctx->content_length);
    freeBuffer(ctx->uri_path_query_fragment);
    freeBuffer(ctx->uri_host);
    freeBuffer(ctx->method);
    parser_destroy(ctx->HTTP_connection);
    parser_destroy(ctx->HTTP_master);
    parser_destroy(ctx->HTTP_host);
    parser_destroy(ctx->HTTP_header);
    parser_destroy(ctx->HTTP_content_length);
    parser_destroy(ctx->HTTP_start_line);
    parser_destroy(ctx->HTTP_request_target);
    parser_destroy(ctx->HTTP_request_abs);
    if( ctx->HTTP_methods != 0 ){

        for( int i = 0 ; i < METHOD_COUNT ; i++){
            parser_destroy(ctx->HTTP_methods[i]);
        }
        free(ctx->HTTP_methods);
    }

    if( ctx->HTTP_versions != 0 ){
        for( int i = 0 ; i < VERSION_COUNT ; i++){
            parser_destroy(ctx->HTTP_versions[i]);
        }
        free(ctx->HTTP_versions);
    }

    free(ctx);
}
