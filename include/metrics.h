//
// Created by gaston on 14/6/21.
//

#ifndef PC_2021A_03_METRICS_H
#define PC_2021A_03_METRICS_H

#include <unistd.h>


enum metric_types{
    HISTORICAL_CONNECTIONS = 0,
    CONCURRENT_CONNECTIONS = 1,
    SENT_BYTES = 2
};
//Devuelve la metrica que le pasas
size_t getMetric(int metric);

//Setea la metrica definitivamente, peligrosa de usar porque pisa los datos
void setMetric(int metric , size_t number);

//Agrega a la metrica el number
int addMetric(int metric , size_t number);

//Resta a la metrica el number
size_t substractMetric(int metric, size_t number);

#endif //PC_2021A_03_METRICS_H
