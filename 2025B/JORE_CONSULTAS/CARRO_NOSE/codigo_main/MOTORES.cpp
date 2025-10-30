
#include "MOTORES.h"

void Avanzar(){
    analogWrite(MA, 200); // 0% POTENCIA -> 0 DE VALOR ^ 100% POTENCIA -> 255 DE VALOR
    analogWrite(MB, 200);
}

void Derecha(){
    analogWrite(MA, 80); // 0% POTENCIA -> 0 DE VALOR ^ 100% POTENCIA -> 255 DE VALOR
    analogWrite(MB, 255);
}


void Izquierda(){
    analogWrite(MA, 255); // 0% POTENCIA -> 0 DE VALOR ^ 100% POTENCIA -> 255 DE VALOR
    analogWrite(MB, 80);
}

void Frenar(){
    analogWrite(MA, 0); // 0% POTENCIA -> 0 DE VALOR ^ 100% POTENCIA -> 255 DE VALOR
    analogWrite(MB, 0);
}