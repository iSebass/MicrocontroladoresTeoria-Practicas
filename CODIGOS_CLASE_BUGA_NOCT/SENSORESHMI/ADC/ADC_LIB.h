
#ifndef ADC_LIB_H_
#define ADC_LIB_H_

#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

void ADC_Init();
int ADC_Read(uint8_t ch);
float ADC_Map(int conv1, float conv_min, float conv_max, float sal_min, float sal_max);



#endif /* ADC_LIB_H_ */