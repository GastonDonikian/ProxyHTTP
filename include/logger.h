//
// Created by gaston on 26/5/21.
//
#ifndef _LOGGER_
#define _LOGGER_
void startLog();
void closeLog();
void appendTime();
void logGeneric(int level, const char *msg);
void logError(const char * msg);
void logInfo(const char * msg);
void logWarning(const char * msg);
#endif
