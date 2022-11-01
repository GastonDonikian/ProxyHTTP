#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdbool.h>


#include "include/HTTP_connection.h"
#include "include/proxyUtils.h"
#include "include/HTTP_parser.h"
#include "include/logger.h"
#include "include/proxyHandlerHelper.h"
#include "include/mcpUtils.h"
#include "include/metrics.h"
#define CLIENT_NUMBER 512
#define IP_PROTOCOLS 2
#define BUFFER_INITIAL_SIZE 1024
#define FALSE 0
#define TRUE 1


//defines del utils en realidad
#define MAX_PENDING 16 //tire un nro cualquiera

size_t CLIENTS = CLIENT_NUMBER;
char **BUFFER_LOCATION;
size_t BUFFER_SIZE = BUFFER_INITIAL_SIZE;

int setReadBufferSize( uint64_t size ){
    if ( size == 0 ){
        return 0;
    }
    BUFFER_SIZE = size;
    char * newbuf =realloc(*BUFFER_LOCATION , size);

    if ( newbuf != 0 ){
        *BUFFER_LOCATION = newbuf;
        return 1;
    }
    return -1;
}

void setupServer(struct httpargs *httpargs) {

        int mcpSocket;
        int passiveSocket[2]; //Sockets del proxy: En el 0 me guardo el socket ipv4 en el otro el ipv6


        if ((passiveSocket[0] = setupServerSocket(AF_INET, AI_PASSIVE, SOCK_STREAM, IPPROTO_TCP, httpargs)) < 0) {
            printf("Error creando el socket IPv4");
            exit(1);
        }

        if ((passiveSocket[1] = setupServerSocket(AF_INET6, AI_PASSIVE, SOCK_STREAM, IPPROTO_TCP, httpargs)) < 0) {
            printf("Error creando el socket IPv6");
            exit(1);
        }

        if((mcpSocket = setupMCPSocket(httpargs)) < 0){
            printf("Error creando el socket del cliente UDP");
            exit(1);
        }

        HTTP_connection HTTP_connections[CLIENTS];
        memset(HTTP_connections, 0, sizeof(HTTP_connection) * CLIENTS);


        fd_set currentfds; // lleva registro antes de que se destruya el readfds
        fd_set readfds;
        FD_ZERO(&currentfds);

        fd_set current_write_fds;
        fd_set writefds;
        FD_ZERO(&current_write_fds);


        int maxSocket = 0; //"mejora" eficiencia
        int socketActivo;
        BUFFER_LOCATION = malloc(sizeof(char**));

        *BUFFER_LOCATION = malloc(BUFFER_SIZE + 1); //BUFFER de lectura, se podria aumentar o achicar con un REALLOC

        for (int i = 0; i < IP_PROTOCOLS; i++) {
            FD_SET(passiveSocket[i], &currentfds);
            if (passiveSocket[i] > maxSocket)
                maxSocket = passiveSocket[i];
        }
        FD_SET(mcpSocket , &currentfds);

        if(mcpSocket > maxSocket)
            maxSocket = mcpSocket;

        //ACA TERMINÓ EL SETUP, COMIENZA LA RUTINA PARA ESCUCHAR
        while (TRUE) {
            readfds = currentfds;
            writefds = current_write_fds;
            socketActivo = select(maxSocket + 1, &readfds, &writefds, NULL,
                                  NULL); //https://www.youtube.com/watch?v=QFsFiDRwPqc
            //Como ya abarque todas las posibilidades este select no es bloqueante, o sea, no hay nada para hacer aparte de el select

            if ((socketActivo < 0) && (errno != EINTR)) {
                continue;
            }

            //Me fijo si el select esta en alguno de los socket pasivos
            for (int i = 0; i < IP_PROTOCOLS; i++) {

                int currentSocket = passiveSocket[i];

                if (FD_ISSET(currentSocket, &readfds)) { //Tengo una nueva conexion entrante
                    int newClientSocket = handleConnection(currentSocket, HTTP_connections);
                    if (newClientSocket > 0) {
                        addMetric(HISTORICAL_CONNECTIONS , 1 );
                        addMetric(CONCURRENT_CONNECTIONS , 1 );
                        FD_SET(newClientSocket, &currentfds);
                        // FD_SET(newClientSocket,  &current_write_fds); // agrego a los current el socket nuevo ow}
                    }
                    if (newClientSocket > maxSocket)
                        maxSocket = newClientSocket;
                }
            }
       //escucho el paquete
            if(FD_ISSET(mcpSocket, &readfds)){
                handleMCPRequest(mcpSocket);

            }
            
            //me fijo en cada conexion
            for (ssize_t client_n = 0; client_n < CLIENTS; client_n++) {
                
                struct HTTP_connection *current = &HTTP_connections[client_n];
                if (current->client_socket > 0) {
                    if (current->stay_connected == false && current->send_final_message == false) {
                        //si le pongo un flag para hacer una vez?
                        handleReadHttpFromUser(current , &currentfds , &current_write_fds , &readfds , *BUFFER_LOCATION , BUFFER_SIZE);
                        if (!(current->connected) && getHost(current->client_context_parser) != NULL) {
                             DoHHandlingZone(current, &current_write_fds, &writefds, &currentfds, &readfds, *BUFFER_LOCATION, &maxSocket , BUFFER_SIZE);
                       }
                        else if (current->connected && current->connection_informed == false) {
                            handleInformConnection(current, &currentfds, &current_write_fds, &writefds);

                        }
                        else if (current->connection_informed && current->stay_connected == false) {
                            current->stay_connected = true;

                        }
                    }else if(current->stay_connected == true && current->send_final_message == false ) {
                        handleStayConnected(current, &currentfds, &current_write_fds, &readfds, &writefds);
                    }else{
                            handleFinalMessage(current, &currentfds, &current_write_fds, &writefds);
                        }
                    }
                }



        }
    }



struct httpargs *httpargs;

int main(int argc, char *argv[]){


    //Setup de parametros
    startLog();
    //Parseo los argumentos
    httpargs = malloc(sizeof(struct httpargs));
    parse_args(argc, argv, httpargs);
    initHTTPparser();
    printf("Estamos esperando tus requests...\n");
    setupServer(httpargs);
    //Me creo un socket para cada cliente, ademas de un buffer de escritura para cada socket para no bloquearme por escritura
    closeHTTPparser();
    free(httpargs);
    closeLog();
    return 0;
}
