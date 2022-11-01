
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "include/doh.h"

#include "include/logger.h"
#include "include/proxyUtils.h"


#define QUERY_SIZE 1024


//Establece la conexión don el servidor DoH dado el host y puerto del mismo. Devuelve el fd del socket conectado, o -1 en caso de error

//Procesa la respuesta del DoH, devuelvo los bytes de respuesta y guardo la lista de resultados. Devuelvo -1 si hubo un error

struct dohContext * initDohContext(){
    struct dohContext * aux = malloc(sizeof(struct dohContext));
        memset(aux,0,sizeof(struct dohContext));
    return aux;
}


int connectToDoH() {
    unsigned char buf[INET6_ADDRSTRLEN];
    int s = inet_pton(AF_INET,httpargs->doh.ip,buf);
    if(s <= 0){
        logInfo("Estaba en formato IPv6");
        s = inet_pton(AF_INET6,httpargs->doh.ip,buf);
        if(s <= 0){
            logError("No pude establecer la conexion con el DoH");
            return -1;
        }
        int clientSocket;
        if ((clientSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0) {
            logError("Error abriendo socket cliente");
            logError(strerror(errno));
            return -1;
        }
        logInfo("Abri el socket para el DoH");


        struct sockaddr_in6 clientAddr;

        //resuelvo el hostname
        memset(&clientAddr, 0, sizeof(clientAddr));
        clientAddr.sin6_family = AF_INET6;
        memcpy(&(clientAddr.sin6_addr),buf,sizeof(struct in6_addr));
        clientAddr.sin6_port = htons(atoi(httpargs->doh.port));

        if(connect(clientSocket,(struct sockaddr *) &clientAddr,sizeof(clientAddr)) < 0){
            logError("Error estableciendo la conexión con el DoH");
            logError(strerror(errno));
            return -1;
        }
        logInfo("Me conecte al socket del Doh");
        return clientSocket;
    }
    int clientSocket;
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        logError("Error abriendo socket cliente");
        logError(strerror(errno));
        return -1;
    }
    logInfo("Abri el socket para el DoH");


    struct sockaddr_in clientAddr;


    memset(&clientAddr, 0, sizeof(clientAddr));
    //La configuro para la conexión
    clientAddr.sin_family = AF_INET;
    memcpy(&(clientAddr.sin_addr), buf,sizeof(struct in_addr));
    clientAddr.sin_port = htons(atoi(httpargs->doh.port));

    if (connect(clientSocket, (struct sockaddr *) &clientAddr, sizeof(clientAddr)) < 0) {
        logError("Error estableciendo la conexión con el DoH");
        logError(strerror(errno));
        return -1;
    }
    else
        logInfo("Me conecte al socket del Doh");
    return clientSocket;
}

int makeRequest(int dohSocket, char *domain, struct dohContext *ctx) {

    //Armo el pedido
    if( ctx->doh_query == 0 ) {
        char query[QUERY_SIZE];
        int formatLength = prepareQuery(domain, query, ctx);
        ctx->doh_query = malloc(sizeof(char) * (formatLength + 2));
        memcpy(ctx->doh_query, query, formatLength);
        ctx->doh_query[formatLength + 1] = 0;
        ctx->pendingBytes = formatLength;
    }
    return sendRequest(dohSocket , ctx);
}

int sendRequest(int dohSocket, struct dohContext *ctx) {

    if (ctx->pendingBytes > 0) {
        int sentBytes = send(dohSocket, ctx->doh_query + ctx->sentBytes, ctx->pendingBytes, MSG_DONTWAIT);
        ctx->sentBytes += sentBytes;
        ctx->pendingBytes -= sentBytes;
    }

    return ctx->pendingBytes;
}

int makeResponseStructure(u_int8_t *dnsPackage, struct dohContext *ctx) {
    //Ignoramos el ID
    size_t i = 2;

    u_int8_t qr = (dnsPackage[i++] & (1 << 7));

    if (qr == 0) {
        logError("Me llego una query cuando necesitaba un response en DoH");
        return -1;
    }

    u_int8_t rcode = (dnsPackage[i++] & 0x0F);

    switch (rcode) {
        case 0:
            logInfo("Rcode no reporto errores");
            break;
        case 1:
            logError("La query tiene un Format Error (o el DoH server no sabe leer)");
            return -1;
        case 2:
            logError("DoH server reported a server failure");
            return -1;
        case 3:
            logError("DoH server ice que hubo un Name Error");
            return -1;
        case 4:
            logError("DoH server no soporta este tipo de query");
            return -1;
        case 5:
            logError("DoH server se niega a hacer esa conexion");
            return -1;
        default:
            logError("Response Code Invalido");
            return -1;
    }

    u_int16_t qdcount = ((dnsPackage[i] << 8) | (dnsPackage[i + 1]));
    i += 2;
    if (qdcount != 1) {
        logError(
                "QDcount distinto de 1 cuando yo mando de a 1 query"); //!important no se si deberia ser 1 por la pregunta que le mande yo, revisar.
        return -1;
    }
    u_int16_t ancount = ((dnsPackage[i] << 8) | (dnsPackage[i + 1]));
    i += 2;
    if (ancount == 0) {
        logError("Answer Count igual a 0");
    }

    i += 4;

    i += ctx->querySize;


    for (int j = 0; j < ancount; j++) {
        i += parseAnswerFormat(dnsPackage + i, ctx);
    }
    return 1;
}

int parseAnswerFormat(u_int8_t *answerSection, struct dohContext *ctx) {

    int i;
    if ((answerSection[0] & 0xC0) == 0xC0) {
        i = 2;
    } else //Este caso no deberia pasar nunca pero lo hacemos por si las dudadas
        i = getAnswerSectionName(answerSection);


    u_int16_t type = ((answerSection[i] << 8) | (answerSection[i + 1]));
    i += 2;
    if(type != 1 && type != 28) {
        logWarning("Answer different from A or AAAA");

    }
    i += 6;

    u_int16_t rdlength = ((answerSection[i] << 8) | (answerSection[i + 1]));
    i += 2;

    if(type == 1){
       handleIPv4(answerSection,ctx,i,rdlength);
    }
    else if(type == 28){
       handleIPv6(answerSection,ctx,i,rdlength);
    }
    else{
        logWarning("El address no esta en formato A ni AAAA, sigo con el proximo");
    }
    return i + rdlength;

}

int handleIPv6(u_int8_t *answerSection, struct dohContext *ctx,int i,int rdlength){
    int aux = rdlength/sizeof(struct in6_addr);
    ctx->index6 += aux;
    ctx->address6 = realloc(ctx->address6,sizeof(struct in6_addr)*ctx->index6);
    memcpy(ctx->address6 + ctx->index6 - aux,answerSection + i,sizeof(struct in6_addr)*aux);
    return i;
}


int handleIPv4(u_int8_t *answerSection, struct dohContext *ctx,int i,int rdlength) {
    int aux = rdlength/sizeof(struct in_addr);
    ctx->index4 += aux;
    ctx->address = realloc(ctx->address,sizeof(struct in_addr)*ctx->index4);
    memcpy(ctx->address + ctx->index4 - aux,answerSection + i,sizeof(struct in_addr)*aux);
    return i;
}

int getAnswerSectionName(const u_int8_t *answerSection) {
    if (answerSection[0] == 0) {
        return 0;
    }
    int i = 1;
    int j = 0;
    int sectionCount = answerSection[0];
    char domain[24] = {0};
    while (answerSection[i] != 0) {
        if (sectionCount == 0) {
            sectionCount = answerSection[i];
            domain[j] = '.';
        } else {
            domain[j] = (char) answerSection[i];
            sectionCount--;
        }
        i++;
        j++;
    }
    domain[j] = 0;

    logInfo("Estoy analizando algun Answer Section de:");
    logInfo(domain);
    return i + 1;// EN i ESTOY PARADO EN EL 0 FINAL.
}




//Prepara la query HTTP para hacerle el pedido al DoH

//Hago un pedido desde el socket cliente al socket servidor


int prepareQuery(char *domain, char *query, struct dohContext *ctx) {

    uint8_t *dnsStructure = malloc(sizeof(u_int8_t) * QUERY_SIZE);


    int dnsLength = makeDNSStructure(domain, dnsStructure, ctx) - 2;
    char dnsStringLength[8];
    sprintf(dnsStringLength, "%d", dnsLength);
    char header[512] = "POST /dns-query HTTP/1.1\r\nHost: ";

    strcat(header, httpargs->doh.host);
    strcat(header, ":");
    strcat(header, httpargs->doh.port);
    strcat(header, "\r\nConnection: close\r\nAccept: application/dns-message\r\nContent-type: application/dns-message\r\nContent-Length: ");
    strcat(header, dnsStringLength);
    strcat(header, "\r\n\r\n");
    int formatLength = strlen(header) + dnsLength;
    memcpy(header + strlen(header), dnsStructure, dnsLength + 2);

    memcpy(query, header, formatLength + 1);
    free(dnsStructure);

    return formatLength;
}

int makeDNSStructure(char *host, uint8_t *dnsStructure, struct dohContext *ctx) {
    //Esta ACA: https://www.ietf.org/rfc/rfc1035.txt


    //HEADER
    size_t i = 0;
    //ID
    dnsStructure[i++] = 0;
    dnsStructure[i++] = 0;
    //Flags y cosas raras
    dnsStructure[i++] = 3; //00000011 es el flag de recursion y de si el server puede hacer recursion
    dnsStructure[i++] = 0;
    //COUNTS
    dnsStructure[i++] = 0;
    dnsStructure[i++] = 1;
    dnsStructure[i++] = 0;
    dnsStructure[i++] = 0;
    dnsStructure[i++] = 0;
    dnsStructure[i++] = 0;
    dnsStructure[i++] = 0;
    dnsStructure[i++] = 0;
    //END-HEADER

    //QUESTION SECTION FORMAT
    ctx->querySize = parseQuestionFormat((u_int8_t *) host, dnsStructure + i,&ctx->sin_port) + 4; //ya le sumo lo que voy a poner despues
    i += ctx->querySize - 4;
    //QTYPE y QCLASS
    dnsStructure[i++] = 0;
    if(ctx->familyFlag == 0)
        dnsStructure[i++] = 1;
    else
        dnsStructure[i++] = 28;
    //IN
    dnsStructure[i++] = 0;
    dnsStructure[i++] = 1;

    dnsStructure[i++] = '\r';
    dnsStructure[i++] = '\n';

    return i;
}

int parseQuestionFormat(u_int8_t *url, u_int8_t *questionFormat,int * sin_port) {
    int i = 0;
    int j = 0;
    while (url[i] != 0 && url[i] != ':') {

        if (url[i] == '.') {
            questionFormat[j] = i - j;
            while (j < i) {
                questionFormat[j + 1] = url[j];
                j++;
            }
            j++;
        }
        i++;

    }
    if (url[i] == ':') {

        *sin_port = atoi((char *) url + i + 1 );
    }else
    {
        *sin_port = 80;
    }

    questionFormat[j] = i - j;

    while (j < i) {
        questionFormat[j + 1] = url[j];
        j++;
    }

    questionFormat[j + 1] = 0;
    return j + 2;
}




void freeDoH(struct dohContext * ctx) {
    if(ctx->address)
        free(ctx->address);
    if(ctx->address6)
        free(ctx->address6);
    free(ctx->doh_query);
    free(ctx);
}