#include "include/logger.h"
#include "include/doh.h"
#include "include/proxyUtils.h"
#include "include/parser_utils.h"
#include "include/HTTP_connection.h"
#include "include/bufferUtils.h"
#include "include/metrics.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include "include/base64Decoder.h"

#define CLIENTS 512
#define TRUE 1

char connected_response[] = "HTTP/1.1 200 Connection Established\r\n\r\n";
size_t connectedResponseSize = sizeof(connected_response) - 1;


ssize_t sendFinalMessage(HTTP_connection *connection) {
    char *message = (*connection->final_message)();
    size_t msgsize = strlen(message) - 1;
    size_t bytesToSend = msgsize - connection->final_message_sent;
    if (bytesToSend > 0) {
        //TODO:Aca tendria que hacer algo para mandarlo al socket de salida, que puede ser 1 o N. Yo lo mando a entrada estandar para testear
        ssize_t bytesSent = send(connection->client_socket, message + connection->final_message_sent, bytesToSend,
                                 MSG_DONTWAIT);
        if (bytesSent < 0) {
            return -2;
        } else if (bytesSent == 0) {
            return -1;
        } else {
            addMetric(SENT_BYTES, bytesSent);
            ssize_t bytesLeft = bytesToSend - bytesSent;
            // Si se pudieron mandar todos los bytes limpiamos el buffer y sacamos el fd para el select
            if (bytesLeft == 0) {
                connection->final_message_sent = msgsize;
                return 0;
            } else {
                connection->final_message_sent += bytesSent;
            }
            return 1;
        }
    }
    return 0;


}

ssize_t informConnection(HTTP_connection *connection) {
    ssize_t bytesToSend = connectedResponseSize - connection->connection_sent;
    if (bytesToSend > 0) {
        //TODO:Aca tendria que hacer algo para mandarlo al socket de salida, que puede ser 1 o N. Yo lo mando a entrada estandar para testear
        ssize_t bytesSent = send(connection->client_socket, connected_response + connection->connection_sent,
                                 bytesToSend, MSG_DONTWAIT);
        if (bytesSent < 0) {
            return -2;
        } else if (bytesSent == 0) {
            return -1;
        } else {
            ssize_t bytesLeft = bytesToSend - bytesSent;
            addMetric(SENT_BYTES, bytesSent);
            // Si se pudieron mandar todos los bytes limpiamos el buffer y sacamos el fd para el select
            if (bytesLeft == 0) {
                connection->connection_sent = connectedResponseSize;
                return 0;
            } else {

                connection->connection_sent += bytesSent;
            }
            return 1;
        }
    }
    return 0;
}

void clearHTTPmessage(HTTP_message *httpMessage) {
    freeBuffer(httpMessage->message);
    freeBuffer(httpMessage->full_message);

}

void clearHTTPconnection(HTTP_connection *connection) {
    if (connection == 0) {
        return;
    }

    freeCtx(connection->client_context_parser);
    freeCtx(connection->doh_context_parser);

    clearHTTPmessage(&connection->request);
    clearHTTPmessage(&connection->response);
    clearHTTPmessage(&connection->doh_request);
    clearHTTPmessage(&connection->doh_response);

    memset(connection, 0, sizeof(HTTP_connection));

}

char buffer[2048];

ssize_t simpleRead(int fromSocket, HTTP_message *message) {
    ssize_t read_bytes = read(fromSocket, buffer, 2048);
    if (read_bytes == 0) {
        return 0; //me cerro el fd
    } else if (read_bytes < 0) {
        return -1;
    } else {
        //como no es circular este buffer puede quedar enorme
        writeNBuffer(message->full_message, buffer, read_bytes);
        return read_bytes;
    }
}

ssize_t handleWrite(int destinationSocket,
                    HTTP_message *message) { //TODO: Creo que el currentSocket no es necesario, va tal vez si pero solo para cerrar la conexion
    ssize_t bytesToSend = message->full_message->from;
    if (bytesToSend > 0) {
        ssize_t bytesSent = send(destinationSocket, message->full_message->buffer, bytesToSend, MSG_DONTWAIT);
        if (bytesSent < 0) {
            return -2;
        }else if(bytesSent == 0){
            return -1;
        } else {
            ssize_t bytesLeft = bytesToSend - bytesSent;
            addMetric(SENT_BYTES, bytesSent);
            // Si se pudieron mandar todos los bytes limpiamos el buffer y sacamos el fd para el select
            if (bytesLeft == 0) {
                //clear(buffer);
                bufferReset(message->full_message);
                return 0;
            } else {
                message->sent += bytesSent;
                deleteStartBuffer(message->full_message, bytesSent);
            }
            return 1;
        }
    }
    return 0;
}


int handleConnection(int connectingSocket, HTTP_connection connections[]) {
    int newSocket;
    if ((newSocket = acceptTCPConnection(connectingSocket)) < 0) {
        return -1;
    }
    FILE *fp = fopen("./proxyAccess.log", "a");
    if (fp != NULL) {

        struct sockaddr_storage addrA;
        struct sockaddr_storage addrB;
        socklen_t serv_len = sizeof(struct sockaddr_in);
        getpeername(newSocket, (struct sockaddr *) &addrA, &serv_len);
        getsockname(newSocket, (struct sockaddr *) &addrB, &serv_len);
        struct sockaddr_in *addr1 = (struct sockaddr_in *) &addrA;
        struct sockaddr_in *addr2 = (struct sockaddr_in *) &addrB;
        if (addr2->sin_family == AF_INET && addr1->sin_family == AF_INET) {

            //por cuestion de tiepmo no implementamos el otro
            time_t now;
            time(&now);
            fprintf(fp, "OPEN: Client %s:%d - Server %s:%d %s", inet_ntoa(addr2->sin_addr), ntohs(addr2->sin_port),
                    inet_ntoa(addr1->sin_addr), ntohs(addr1->sin_port), ctime(&now));
        }
        fclose(fp);
    }

    //asigno la nueva conexión a un socket disponible
    for (int j = 0; j < CLIENTS; j++) {
        if (connections[j].client_socket == 0) {
            connections[j].client_context_parser = initCtx(&(connections[j].request));
            connections[j].client_socket = newSocket;
            connections[j].response.message = bufferInit(1250);
            connections[j].response.full_message = bufferInit(1250);
            j = CLIENTS + 1;
        }
    }

    return newSocket;
}

int acceptTCPConnection(int masterSocket) {
    int clientSocket = accept(masterSocket, NULL, NULL);

    if (clientSocket < 0) {
        logError("No me pude aceptar la conexion TCP");
        return -1;
    }

    //Me pude conectar
    return clientSocket;
}

void handleConnectionClose(int *currentSocket) {
    close(*currentSocket);
    *currentSocket = 0;
}

void clear(struct buffer *buffer) {
    free(buffer->buffer);
    buffer->buffer = NULL;
    buffer->from = buffer->len = 0;
}

struct sockaddr_in proxyAddress;
struct sockaddr_in6 proxyAddress6;
struct sockaddr_in6 proxyAddress6_localhost;
//Aca me tengo que encargar de crear el socket pasivo (ipv4 o ipv6), para luego mandar a sockets activos.
int setupServerSocket(int family, int flags, int socktype, int protocol, struct httpargs *httpargs) {

    int opt = TRUE, ipSocket;

    if ((ipSocket = socket(family, socktype, IPPROTO_TCP)) < 0) {
        logError("Error al crear el socket\n");
        return -1;
    }

    if (setsockopt(ipSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
        logError("Error al ponerle opciones\n");
        return -1;
    }

    if (family == AF_INET) { //configuro y bindeo el socket IPv4
//        struct sockaddr_in ipAddress;

        //limpio mi dirección ip
        memset(&proxyAddress, 0, sizeof(proxyAddress));

        proxyAddress.sin_family = AF_INET;
        proxyAddress.sin_addr.s_addr = inet_addr(httpargs->http_addr);
        proxyAddress.sin_port = htons(httpargs->http_port);
        //conecto mi socket con la dirección
        if (bind(ipSocket, (struct sockaddr *) &proxyAddress, sizeof(proxyAddress)) < 0) {
            logError("Error en el bind de IPv4\n");
            close(ipSocket);
            return -1;
        }

    } else if (family == AF_INET6) { //idem arriba con IPv6


        if (setsockopt(ipSocket, SOL_IPV6, IPV6_V6ONLY, (char *) &opt, sizeof(opt)) < 0) {
            logError("Error al ponerle opciones\n");
            return -1;
        }

        //limpio mi dirección ip
        memset(&proxyAddress6, 0, sizeof(proxyAddress6));

        proxyAddress6.sin6_family = AF_INET6;
        proxyAddress6.sin6_port = htons(httpargs->http_port);
        inet_pton(AF_INET6, httpargs->http_addr, &(proxyAddress6.sin6_addr));
        inet_pton(AF_INET6,"127.0.0.1",&(proxyAddress6_localhost.sin6_addr));
        proxyAddress6 = proxyAddress6;
        if (bind(ipSocket, (struct sockaddr *) &proxyAddress6, sizeof(proxyAddress6)) < 0) {
            logError("Aca paso algo en el bind\n");
            close(ipSocket);
            return -1;
        }

    }

    //escucho a mis clientes
    if (listen(ipSocket, CLIENTS) < 0) {
        logError("Error en el listen de IPv4\n");
        close(ipSocket);
        return -1;
    }

    fcntl(ipSocket, F_SETFL, O_RDWR | O_NONBLOCK);

    return ipSocket;


}


int setupRemoteSocket6(HTTP_connection *connection, int *maxSocket) {
    int currentSocket = -1;
    char error_msg[1025];
    int addressSize = connection->dohContext->index6;
    struct in6_addr *addressArray6 = connection->dohContext->address6;
    for (int i = 0; i < addressSize && currentSocket == -1; i++) {
        currentSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        int synRetries = 1;
        setsockopt(currentSocket, IPPROTO_TCP, 7,&synRetries,sizeof(synRetries));

        struct sockaddr_in6 sock_addr_6 = {
                .sin6_addr = addressArray6[i],
                .sin6_family = AF_INET6,
                .sin6_port = htons(connection->dohContext->sin_port),
        };

        uint8_t * addr_1 = proxyAddress6.sin6_addr.s6_addr;
        uint8_t * addr_2 = sock_addr_6.sin6_addr.s6_addr;
        uint8_t * addr_3 = proxyAddress6_localhost.sin6_addr.s6_addr;

        int res = memcmp(addr_1,addr_2,16);
        int res2 = memcmp(addr_2,addr_3,16);

        if( (res == 0 || res2 == 0) && sock_addr_6.sin6_port == proxyAddress6.sin6_port){
            close(currentSocket);
            currentSocket = -1;
            return currentSocket;
        }
        if (currentSocket >= 0) {

            if (connect(currentSocket, (const struct sockaddr *) &sock_addr_6,
                                   sizeof(struct sockaddr_in6))) {
                close(currentSocket);
                currentSocket = -1;
            } else {
                if (currentSocket > *maxSocket) {
                    *maxSocket = currentSocket;
                }
                connection->server_socket = currentSocket;
                connection->connected = true;
                return currentSocket;
            }
        } else {
            perror(error_msg);
        }
    }
    addressSize = connection->dohContext->index4 ;
    struct in_addr *addressArray4 = connection->dohContext->address;
    for (int i = 0; i < addressSize && currentSocket == -1; i++) {
        currentSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        int synRetries = 2;
        setsockopt(currentSocket, IPPROTO_TCP, 7,&synRetries,sizeof(synRetries));
        struct sockaddr_in sock_addr = {
                .sin_addr = addressArray4[i],
                .sin_family = AF_INET,
                .sin_port = htons(connection->dohContext->sin_port),
        };

        uint32_t clientIpAddr = ntohl(sock_addr.sin_addr.s_addr);
        uint32_t proxyIpAddr = ntohl(proxyAddress.sin_addr.s_addr);
        //Si ves esta linea, perdon juan
        if((clientIpAddr == proxyIpAddr || clientIpAddr == 2130706433) && sock_addr.sin_port == proxyAddress.sin_port){
            logWarning("*DETECTED: Attempting recursivity*\n");
            close(currentSocket);
            currentSocket = -1;
            return currentSocket;
        }
        if (currentSocket >= 0) {
            if (connect(currentSocket, (const struct sockaddr *) &sock_addr, sizeof(struct sockaddr_in))) {
                close(currentSocket);
                currentSocket = -1;
            } else {

                if (currentSocket > *maxSocket) {
                    *maxSocket = currentSocket;
                }
                connection->server_socket = currentSocket;
                connection->connected = true;
                return currentSocket;
            }
        } else {
            perror(error_msg);
        }

    }
    return currentSocket;
}

char basic_string[] = "Basic";

void saveAuthorization(struct ctx *ctx) {
    struct buffer *authorization = getAuth(ctx);
    unsigned char *buffer2 = (unsigned char *)authorization->buffer;
    uint8_t j = 0;
    for (uint8_t i = 0; i < authorization->from; i++) {
        if (i < 5) {
            if (toupper(buffer2[i]) != toupper(basic_string[i])) {
                return;
            }
        } else {
            if (buffer2[i] != ' ') {
                //Decode
                //llegue a lo que es auth
                while (buffer2[i + j] != '\r' && buffer2[i + j] != ' ' && i + j < authorization->from) {
                    j++;
                }
                size_t out_len = 0;
                unsigned char *out = base64_decode(buffer2 + i, j, &out_len);
                out[out_len] = 0;
                logInfo("Usuario y contraseña detectada: ");
                logInfo((char *)out);
                free(out);
                return;
            }
        }
    }
}


int checkIfIp( char * buff , struct dohContext * context,size_t length){
    char * token1;
    char * token2;
    char * token3;

    char * token6_1;
    char * token6_2;

    size_t auxSize= length;
    char * aux = malloc(auxSize + 1);
    char * aux2 = malloc(auxSize + 1);
    aux[auxSize] = 0;
    aux2[auxSize] = 0;
    memcpy(aux,buff,auxSize);
    memcpy(aux2,buff,auxSize);



    if(aux2[0] == '['){
        token6_1 = strtok(aux2+1 , "]");
        struct in6_addr *inaddr6 = malloc(sizeof(struct in6_addr));
        if ( inet_pton(AF_INET6 , token6_1 , inaddr6 )  <= 0){
            free(aux);
            free(aux2);
            free(inaddr6);
            return -1;
        } else{
            context->index6 = 1;
            context->address6 = inaddr6;
        }
        token6_2 = strtok(NULL , "]");

        if(token6_2 == 0 ){
            context->sin_port = 80;
        }else if(token6_2[0] == 0  ){
            context->sin_port = 80;
        }else if( token6_2[0] == ':' && token6_2[1] != 0){
            context->sin_port = atoi(token6_2 + 1);
        }
        free(aux2);
        free(aux);
        return 0;

    }

    token1 = strtok(aux, ":");
    token2 = strtok(NULL, ":");
    if(token2 != NULL)
        token3 = strtok(NULL, ":");
    else
        token3 = NULL;

    struct in_addr *inaddr = malloc(sizeof(struct in_addr));

    if (token3 == NULL) {

        if ( inet_pton(AF_INET , token1 , inaddr )  <= 0){
            free(aux);
            free(aux2);
            free(inaddr);
            return -1;
        } else {
            context->index4 = 1;
            context->address = inaddr;
        }
        if (token2 == NULL) {
            context->sin_port = 80;
        } else {
            context->sin_port = atoi(token2);
        }
        free(aux2);
        free(aux);
        return 0;
        //ipv4
    }
    return -2; //No era IPv4 ni IPv6
}