/*
 * ADC_LIB.h
 *
 * Created: 11/09/2024 09:20:44 p. m.
 *  Author: USER
 */ 


#ifndef ADC_LIB_H_
#define ADC_LIB_H_

#include <avr/io.h>
#include <stdint.h>

#define A0 0
#define A1 1
#define A2 2
#define A3 3
#define A4 4
#define A5 5
#define A6 6
#define A7 7

#define _DIV2A   0
#define _DIV2B   1
#define _DIV4    2
#define _DIV8    3
#define _DIV16   4
#define _DIV32   5
#define _DIV64   6
#define _DIV128  7

#define MAX_CONV  1023.0

void ADC_init();
uint16_t ADC_read(uint8_t channel);
float ADC_Map(int conv1, float conv_min, float conv_max, float sal_min, float sal_max);

#endif /* ADC_LIB_H_ */