
#ifndef SEVENSEG_LIB_H_
#define SEVENSEG_LIB_H_

#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

#define DISPLAYDDR    DDRA
#define DISPLAYPORT   PORTA

#define SEGA  0
#define SEGB  1
#define SEGC  2
#define SEGD  3
#define SEGE  4
#define SEGF  5
#define SEGG  6

void SevenSeg_Init(void);
void SevenSeg_DisplayCCValue(char val);
void SevenSeg_DisplayACValue(char val);

#endif /* SEVENSEG_LIB_H_ */