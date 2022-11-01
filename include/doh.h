//
// Created by gaston on 5/6/21.
//

#ifndef PC_2021A_03_DOH_H
#define PC_2021A_03_DOH_H
#include <netdb.h>

struct serverAddress {
    socklen_t ai_addrlen;
    struct sockaddr * ai_addr;

    int ai_family;
    struct serverAddress * ai_next;
};



struct serverAddress6{
    socklen_t ai_addrlen;
    struct sockaddr_in6 * ai_addr;
    int ai_family;
    struct serverAddress6 * ai_next;
};

struct dohContext {
    size_t querySize;
    int sin_port; //POR DEFAULT USO EL 80

    int pendingBytes;
    int sentBytes;
    char * doh_query;

    int familyFlag;

    struct in_addr * address;
    int index4;

    struct in6_addr * address6;
    int index6;
};

//TODO SUPER IMPORTANTE HACER TAMBIEN EL METEODO IPV6

//Esto es un main de testeo.
int connectToDoH();

int handleIPv4(u_int8_t *answerSection, struct dohContext *ctx,int i,int rdlength);
int handleIPv6(u_int8_t *answerSection, struct dohContext *ctx,int i,int rdlength);

int prepareQuery(char *domain, char *query, struct dohContext *ctx);

int parseQuestionFormat(u_int8_t *url, u_int8_t *questionFormat,int * sin_port);

int makeDNSStructure(char *host, uint8_t *dnsStructure, struct dohContext *ctx);

int parseAnswerFormat(u_int8_t * answerSection, struct dohContext * ctx);

int getAnswerSectionName(const u_int8_t * answerSection);

struct serverAddress6 * getResults6(struct dohContext * ctx);

int sendRequest(int dohSocket, struct dohContext *ctx);

int makeResponseStructure(u_int8_t *dnsPackage, struct dohContext *ctx);

int makeRequest(int dohSocket, char *domain, struct dohContext *ctx);

//Dado el host y service del server DoH, le hago un pedido para resolver @domain y devuelvo los resultados en @results. Devuelve -1 en caso de error

void freeDoH(struct dohContext *);

__attribute__((unused)) struct dohContext * initDohContext();

struct serverAddress * getResults( struct dohContext * ctx);


#endif //PC_2021A_03_DOH_H
