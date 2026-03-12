#ifndef SEVENSEG_LIB_H_
#define SEVENSEG_LIB_H_


#include "../gpio.h"

//DEFINIR LOS PINES DEL DISPLAY
#define sega   0
#define segb   1
#define segc   2
#define segd   3
#define sege   4
#define segf   5
#define segg   6
#define segdp  7

//DEFINIR LOS PUERTOS A USAR EN EL DISPLAY
#define CONFIG_DISPLAY(pin, mode)  GPIO_PIN_MODE_PORTA(pin, mode)
#define WRITE_DISPLAY(pin, value)  GPIO_WRITE_PORTA(pin, value)

//DEFINIMOS LOS METODOS DE FUNCIONAMIENTO
void displayInit(void);
void setDisplayCC(int value);
void setDisplayAC(int value);




#endif /* SEVENSEG_LIB_H_ */