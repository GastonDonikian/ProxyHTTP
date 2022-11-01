#include "include/proxyHandlerHelper.h"
#include "include/proxyUtils.h"
#include <time.h>
#include <string.h>
#include "include/metrics.h"
#include "include/proxyUtils.h"

void prepareFinalMessage(HTTP_connection *current, char *(*final_message )(void), fd_set *currentfds,
                         fd_set *current_write_fds) {
    //handle connection close

    FD_CLR(current->doh_socket, currentfds);
    FD_CLR(current->server_socket, currentfds);
    FD_CLR(current->client_socket, currentfds);

    FD_CLR(current->doh_socket, current_write_fds);
    FD_CLR(current->server_socket, current_write_fds);
    FD_SET(current->client_socket, current_write_fds);

    current->send_final_message = true;

    current->final_message = final_message;
}

void handleFinalMessage(HTTP_connection *current, fd_set *currentfds, fd_set *current_write_fds, fd_set *writefds) {
    if (FD_ISSET(current->client_socket, writefds)) {
        if (sendFinalMessage(current) <= 0) {
            closeAll(current, currentfds, current_write_fds);
        }
    }
    //Le doy un error final al cliente por algo que falle y limpio.

}

int checkIfMustClose(HTTP_connection * current,fd_set * currentfds,fd_set * current_write_fds){
    if ((current->client_write_closed && current->server_write_closed) ||
        (current->client_read_closed && current->server_read_closed) ||
        (current->client_write_closed && current->client_read_closed) ||
        (current->server_write_closed && current->server_read_closed) ||
        (current->server_read_closed && current->response.full_message->from == 0) ||
        current->client_read_closed){
        closeAll(current, currentfds, current_write_fds);
        return 1;
    }
    return 0;
}


void handleReadHttpFromUser(HTTP_connection *current, fd_set *currentfds, fd_set *current_write_fds, fd_set *readfds,
                            char *buffer, size_t buffer_size) {
    ssize_t read_size;
    if (FD_ISSET(current->client_socket, readfds)) {
        read_size = readAndParseRequest(&(current->client_socket), current->client_context_parser, currentfds, buffer,
                                        buffer_size);
        if (read_size == -2 ){

            prepareFinalMessage(current, badRequest, currentfds, currentfds);
        } else if (read_size == -1) {
            FD_CLR(current->client_socket, currentfds); //Ya no tengo que darle bola, lo saco del current
            current->client_read_closed = true;
            current->request_ready = true;
            saveAuthorization(current->client_context_parser);
        }else if( read_size == 0 ){
            current->header_finished = true;
        } else if( read_size == -3) {
            prepareFinalMessage(current,badRequest,currentfds,current_write_fds);
        }
    }
}

void handleWriteToClient(HTTP_connection *current, fd_set *currentfds, fd_set *current_write_fds, fd_set *writefds) {

    if (FD_ISSET(current->client_socket, writefds)) {
        ssize_t sent;
        if ((sent = handleWrite(current->client_socket, &(current->response))) == -1) {
        } else if (sent == 0) {
            //resetToNext(curren
            //FD_SET(current->client_socket, &currentfds);
            closeAll(current, currentfds, current_write_fds);
        }
    }

}

void handleReadFromServer(HTTP_connection *current, fd_set *currentfds, fd_set *current_write_fds, fd_set *readfds,
                          char *buffer, size_t buffer_size) {
    if (FD_ISSET(current->server_socket, readfds)) { //ESTO NO ESTA MAL???
        ssize_t serverRead;
        if ((serverRead = read(current->server_socket, buffer, buffer_size)) < 0) {
            prepareFinalMessage(current, badGateway, currentfds, current_write_fds);
        } else if (serverRead == 0) {
            FD_SET(current->client_socket, current_write_fds);
            FD_CLR(current->server_socket, currentfds); //Ya no tengo que darle bola, lo saco del current
        } else {
            if (current->response.message == 0) {
                current->response.message = bufferInit(1250);
            }
            writeNBuffer(current->response.full_message, buffer, serverRead);
        }
    }
}

void handleWriteToServer(HTTP_connection *current, fd_set *currentfds, fd_set *current_write_fds, fd_set *writefds) {
    size_t bytes_sent;
    if (FD_ISSET(current->server_socket, writefds)) {
        if ((bytes_sent = handleWrite(current->server_socket, &current->request)) == 0) {
            FD_CLR(current->server_socket, current_write_fds);
            FD_SET(current->server_socket, currentfds);
        } else if (bytes_sent == -1) {
            closeAll(current, currentfds, current_write_fds);
        }
    }
}

void handleInformConnection(HTTP_connection *current, fd_set *currentfds, fd_set *current_write_fds, fd_set *writefds) {
    size_t bytes_sent;
    if (FD_ISSET(current->client_socket, writefds)) {
        if ((bytes_sent = informConnection(current)) == 0) {
            //activo el modo tunnel, ya le avise por completo al user
            current->connection_informed = true;
            current->stay_connected = true;
            //todavia no tengo nada para escribirle al usuario, hago clear aca
            FD_CLR(current->client_socket, current_write_fds);
            FD_SET(current->client_socket, currentfds);

            //quiero esperar a tener cosas del server, me va a volver a hablar el user
        } else if (bytes_sent == -1) {
            //me cerraron la conexion, que mal :(
            closeAll(current, currentfds, current_write_fds);
        }
    }
}

void resetDoH(HTTP_connection *current) {

    freeCtx(current->doh_context_parser);
    if (current->dohContext->doh_query != NULL)
        free(current->dohContext->doh_query);
    current->dohContext->doh_query = 0;
    current->dohContext->sentBytes = 0;
    clearHTTPmessage(&current->doh_response);
    clearHTTPmessage(&current->doh_request);
    current->doh_context_parser = initCtxResponse(&current->doh_request);
}

void DoHHandlingZone(HTTP_connection *current, fd_set *current_write_fds, fd_set *writefds, fd_set *current_read_fds,
                     fd_set *readfds, char *buffer, int *maxSocket, size_t buffer_size) {
    if (current->tried_direct == false) {

        current->dohContext = initDohContext();

        if (checkIfIp(getHost(current->client_context_parser), current->dohContext,getHostLength(current->client_context_parser)) != 0) {
            current->tried_direct = true;
            freeDoH(current->dohContext);
            current->dohContext = 0;
        } else {


            if (setupRemoteSocket6(current, maxSocket) != -1) {
                if (isConnect(current->client_context_parser)) {
                    //setup, informar conexion

                    FD_SET(current->client_socket, current_write_fds);
                    current->connection_informed = false;
                } else { ;
                    //setup, hablar con el server
                    current->connection_informed = true;
                    FD_SET(current->server_socket, current_write_fds);
                }
                //dejo de leerle al doh
                FD_CLR(current->doh_socket, current_read_fds);
                freeDoH(current->dohContext);
            }
            else{

                prepareFinalMessage(current,internalServerError,current_read_fds,current_write_fds);
            }
            return;
        }
    }
    if (current->doh_socket == 0) {
        current->doh_socket = connectToDoH();
        if (current->doh_socket <= 0) {
            prepareFinalMessage(current,internalServerError,current_read_fds,current_write_fds);
            return;
        }
        if (current->dohContext == 0) {
            current->dohContext = initDohContext();
            current->doh_context_parser = initCtxResponse(&(current->doh_request));
        }

        FD_SET(current->doh_socket, current_write_fds);
        if (current->doh_socket > *maxSocket) {
            *maxSocket = current->doh_socket;
        }
        return;
    } else {
        if (FD_ISSET(current->doh_socket, writefds)) {
            if (getHost(current->client_context_parser) != NULL) {
                size_t pending;
                if (current->dohContext->familyFlag == 0) {
//                    dumpBuffer(current->doh_request.full_message, 0);
//                    dumpBuffer(current->doh_response.message, 0);
//                    dumpBuffer(current->doh_response.message, 0);
//                    dumpBuffer(current->doh_request.full_message, 0);
                    if ((pending = makeRequest(current->doh_socket, getHost(current->client_context_parser),
                                               current->dohContext)) == 0) {

                        resetDoH(current);

                        FD_CLR(current->doh_socket, current_write_fds);
                        FD_SET(current->doh_socket, current_read_fds);
                    }

                } else {
                    if ((pending = makeRequest(current->doh_socket, getHost(current->client_context_parser),
                                               current->dohContext)) == 0) {
                        FD_CLR(current->doh_socket, current_write_fds);
                        FD_SET(current->doh_socket,
                               current_read_fds); ///<--- a los que tengo que prestarle atencion en el select
                    }
                }
            }
        }
    }
    //Otro modulo
    //LEER AL DOH SERVER
    if (current->doh_socket != 0 &&
        FD_ISSET(current->doh_socket, readfds)) { //readfds -> los que me devolvio el select como activos
        ssize_t aux_read;
        //misma rutina de send?
        if ((aux_read = readAndParseResponse(&current->doh_socket, current->doh_context_parser, current_read_fds,
                                             buffer, buffer_size)) <= 0) {
            HTTP_message *message = getMessage(current->doh_context_parser);
            if (message == 0) {
                //ERROR
                //TODO:  prepareFinalMessage(current,internalServerError,current_read_fds,current_write_fds); return;
            } else {
                if (makeResponseStructure((u_int8_t *) message->message->buffer, current->dohContext) < 0) {
                    logError("DoH me devolvio un error");
                    if ((current->dohContext->familyFlag) == 1) {
                        prepareFinalMessage(current, internalServerError, current_read_fds, current_write_fds);
                        return;
                    }
                }
                //tengo la estructura lista
                if (current->dohContext->familyFlag == 0) {
                    current->dohContext->familyFlag = 1;
                    handleConnectionClose(&current->doh_socket);
                    resetDoH(current);
                    current->doh_socket = connectToDoH();
                    FD_CLR(current->doh_socket, current_read_fds);
                    FD_SET(current->doh_socket, current_write_fds);
                    if (current->doh_socket > *maxSocket) {
                        *maxSocket = current->doh_socket;
                    }
                    return;
                }

                if (setupRemoteSocket6(current, maxSocket) != -1) {
                    if (isConnect(current->client_context_parser)) {
                        //setup, informar conexion

                        FD_SET(current->client_socket, current_write_fds);
                        current->connection_informed = false;
                    } else {
                        //setup, hablar con el server
                        current->connection_informed = true;
                        FD_SET(current->server_socket, current_write_fds);
                    }
                    //dejo de leerle al doh
                    FD_CLR(current->doh_socket, current_read_fds);
                    handleConnectionClose(&(current->doh_socket));
                    freeDoH(current->dohContext);
                }
                else{
                    prepareFinalMessage(current,internalServerError,current_read_fds,current_write_fds);
                }
            }
        }
    }
}



void handleStayConnected(HTTP_connection *current, fd_set *currentfds, fd_set *current_write_fds, fd_set *readfds,
                         fd_set *writefds) {
    //agarro mis buffers de HTTP request y response , y los uso, request para user y response para server
   if(checkIfMustClose(current,currentfds,current_write_fds)){
        return;
    }
    if (current->stay_connected_setted_up == FALSE) {
        if (isConnect(current->client_context_parser)) {
            bufferReset(current->request.full_message);
            current->request.sent = 0;
        }
        if ( current->request.full_message->from > 0 ){
            FD_SET(current->server_socket , current_write_fds);
        }
        bufferReset(current->response.full_message);
        current->stay_connected_setted_up = TRUE;
        current->response.sent = 0;

        //para que entre aca estaba escuchando solo al client socket, asi que va a ser el unico seteado
        FD_SET(current->server_socket, currentfds);
        return;
    }
    ssize_t aux_bytes;
    //leer del usuario
    if (FD_ISSET(current->client_socket, readfds)) {
        if ((aux_bytes = simpleRead(current->client_socket, &current->request)) > 0) {
            FD_SET(current->server_socket, current_write_fds);
        } else if (aux_bytes == 0) {
            current->client_read_closed = true;
            FD_CLR(current->client_socket, currentfds);
            checkIfMustClose(current,currentfds,current_write_fds);
        } else {
            //error
            closeAll(current, currentfds, current_write_fds);
            return;
        }
    }
    /* no me crean condicion de server los reads, asi que dejar de leer despues de un punto seria factible*/
    //leer del server
    if (FD_ISSET(current->server_socket, readfds) && !current->client_write_closed) {
        if ((aux_bytes = simpleRead(current->server_socket, &current->response)) > 0) {
            FD_SET(current->client_socket, current_write_fds);
        } else if (aux_bytes == 0) {
            current->server_read_closed = true;
            //deberia ver un flag de dejar de leerle al usuario, solo enviarle lo que me quedó en el buffer
            FD_SET(current->client_socket, current_write_fds);
            FD_CLR(current->server_socket, currentfds);
            if(checkIfMustClose(current,currentfds,current_write_fds))
                return;
        } else {

            //error
            closeAll(current, currentfds, current_write_fds);
            return;
        }
    }

    if (FD_ISSET(current->server_socket, writefds) && !current->server_write_closed) {
        if ((aux_bytes = handleWrite(current->server_socket, &current->request)) == 0) {
            FD_CLR(current->server_socket, current_write_fds);
        } else if (aux_bytes == -1) {
            current->server_write_closed = true;
            //prender un flag de write server cerrado
            FD_CLR(current->client_socket, currentfds);
            FD_CLR(current->server_socket, current_write_fds);
            if(checkIfMustClose(current,currentfds,current_write_fds))
                return;
        } else if ( aux_bytes == -2) {
            //error
            closeAll(current, currentfds, current_write_fds);
            return;
        }

    }
    if (FD_ISSET(current->client_socket, writefds) && !current->client_read_closed) {
        if ((aux_bytes = handleWrite(current->client_socket, &current->response)) == 0) {
            FD_CLR(current->client_socket, current_write_fds);
            if(checkIfMustClose(current,currentfds,current_write_fds))
                return;
        } else if (aux_bytes == -1) {
            current->client_write_closed = true;
            //prender un flag de write server cerrado
            FD_CLR(current->server_socket, currentfds);
            FD_CLR(current->client_socket, current_write_fds);
            if(checkIfMustClose(current,currentfds,current_write_fds))
                return;
        } else if(aux_bytes == -2){
            //error
            closeAll(current, currentfds, current_write_fds);
            return;
        }
    }

    if(checkIfMustClose(current,currentfds,current_write_fds))
        return;


}



ssize_t
readAndParseRequest(int *socket, struct ctx *parser_context, fd_set *currentfds, char *buffer, size_t buffer_size) {
    ssize_t clientRead;
    if ((clientRead = read(*socket, buffer, buffer_size)) < 0) { //levanto lo que me escribio el cliente
        return -2;
    }else if(clientRead == 0){
        return -1;
    } else {
        //parseo
        ssize_t j;
        for (j = 0; j < clientRead; j++) {
            http_master(parser_context, buffer[j]);
        }
        if (hasError(parser_context)){
            return -3;
            //if -3 => bad request;
        }
        if(hasFinished(parser_context)){
            return 0;
        }
        return clientRead;
    }
}

ssize_t
readAndParseResponse(int *socket, struct ctx *parser_context, fd_set *currentfds, char *buffer, size_t buffer_size) {
    ssize_t clientRead;
    if ((clientRead = read(*socket, buffer, buffer_size)) <= 0) { //levanto lo que me escribio el cliente
        return -1;
    } else {
        ssize_t j;
        for (j = 0; j < clientRead; j++) {
            http_master_response(parser_context, buffer[j]);
        }
    }
    return clientRead;
}


void resetToNext(HTTP_connection *current) {
    freeCtx(current->client_context_parser);
    current->client_context_parser = initCtx(&current->request);
}

void closeAll(HTTP_connection *current, fd_set *currentfds, fd_set *current_write_fds) {
    FD_CLR(current->doh_socket, current_write_fds);
    FD_CLR(current->doh_socket, currentfds);

    FD_CLR(current->server_socket, current_write_fds);
    FD_CLR(current->server_socket, currentfds);

    FD_CLR(current->client_socket, current_write_fds);
    FD_CLR(current->client_socket, currentfds);
    if (current->doh_socket != 0) {
        handleConnectionClose(&current->doh_socket);
    }
    if (current->client_socket != 0) {
        FILE *fp = fopen("./proxyAccess.log", "a");
        if (fp != NULL) {

            struct sockaddr_storage addrA;
            struct sockaddr_storage addrB;
            int clnt_sock_err, clnt_peer_err;
            socklen_t serv_len = sizeof(struct sockaddr_in);
            clnt_peer_err = getpeername(current->client_socket, (struct sockaddr *) &addrA, &serv_len);
            clnt_sock_err = getsockname(current->client_socket, (struct sockaddr *) &addrB, &serv_len);
            if(clnt_peer_err == -1 || clnt_sock_err == -1);
            else{
            struct sockaddr_in *addr1 = (struct sockaddr_in *) &addrA;
            struct sockaddr_in *addr2 = (struct sockaddr_in *) &addrB;
            if (addr2->sin_family == AF_INET && addr1->sin_family == AF_INET) {

                //por cuestion de tiepmo no implementamos el otro
                time_t now;
                time(&now);
                fprintf(fp, "CLOSE: Client %s:%d - Server %s:%d %s", inet_ntoa(addr2->sin_addr), ntohs(addr2->sin_port),
                        inet_ntoa(addr1->sin_addr), ntohs(addr1->sin_port), ctime(&now));
            }}
            fclose(fp);
        }
        handleConnectionClose(&current->client_socket);
    }
    if (current->server_socket != 0) {
        handleConnectionClose(&current->server_socket);
    }
    clearHTTPconnection(current);
    substractMetric(CONCURRENT_CONNECTIONS, 1);
}