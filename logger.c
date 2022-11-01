//
// Created by gaston on 26/5/21.
//
// Como no tengo suficiente experiencia creando logs voy a mandar las cosas a syslog y tambien crear una carpeta que adentro tenga el log

#include <syslog.h>
#include <bits/types/FILE.h>
#include <stdio.h>
#include <bits/types/time_t.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include "include/logger.h"



void appendTime();

FILE *fp;
int started;
void startLog() {
    //El LOG_PID nos sirve si nos forkeamos lo dejo y si no nos forkeamos lo sacamos
    started = 1;
    openlog("proxyTCPServer",LOG_PID,LOG_DAEMON);
    fp = fopen("./proxyServer.log","a");
}

//Hago 4 funciones distintas asi mis compañeros no se tienen que meter a ver este codigo y le pueden mandar el string y listo
void closeLog() {
    closelog(); //esto es de syslog.h
    fclose(fp);
}

void logGeneric(int level, const char *msg){
    appendTime(fp);
    if(!started){
        startLog();
    }
    switch (level) {
        case 0:
            logInfo(msg);
            break;
        case 1:
            logWarning(msg);
            break;
        case 2:
            logError(msg);
            break;
    }
}

void logError(const char * msg) {
    syslog(LOG_ERR,"ERROR: %s\n",msg);
    fprintf(fp,"ERROR: %s - %s\n",msg, strerror(errno));
}

void logInfo(const char * msg){
    syslog(LOG_INFO,"INFO: %s\n",msg);
    fprintf(fp,"INFO: %s\n",msg);
}
void logWarning(const char * msg) {
    syslog(LOG_WARNING,"WARNING: %s\n",msg);
    fprintf(fp,"WARNING: %s\n",msg);
}

void appendTime(FILE * fp) {
    time_t now;
    time(&now);

    char *p = strtok(ctime(&now), "\n");
    fprintf(fp,"[%s] ", p);
}
