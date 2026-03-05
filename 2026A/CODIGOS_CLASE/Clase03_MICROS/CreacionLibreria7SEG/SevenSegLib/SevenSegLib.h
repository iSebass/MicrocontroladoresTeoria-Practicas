/*
 * SevenSegLib.h
 *
 * Created: 04/03/2026 09:02:43 p. m.
 *  Author: USER
 */ 


#ifndef SEVENSEGLIB_H_
#define SEVENSEGLIB_H_

#include "../Config.h"

#define  DIR_DISPLAY  DDRA
#define  PORT_DISPLAY PORTA

#define SEGA 0
#define SEGB 1
#define SEGC 2
#define SEGD 3
#define SEGE 4
#define SEGF 5
#define SEGG 6

void displayAC(char n);
void displayCC(char n);
 



#endif /* SEVENSEGLIB_H_ */