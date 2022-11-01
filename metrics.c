//
// Created by gaston on 14/6/21.
//

#include "include/metrics.h"
size_t metrics[] = { 0 , 0 , 0 };


//Devuelve la metrica que le pasas
size_t getMetric(int metric){
    return metrics[metric];
}

//Setea la metrica definitivamente, peligrosa de usar porque pisa los datos returnea la metrica
void setMetric(int metric, size_t number) {
    metrics[metric] = number;
}


//Agrega a la metrica el number
int addMetric(int metric, size_t number){
    return metrics[metric] += number; 
}


//Resta a la metrica el number
size_t substractMetric(int metric, size_t number){
    if ( metrics[metric] == 0 ){
        return 0;
    }
    return metrics[metric] -= number;
}