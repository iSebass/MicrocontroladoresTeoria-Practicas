#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

#include "SEVENSEG/SEVENSEG_LIB.h"
char contador=0;

int main(void)
{
  
	SevenSeg_Init();
	
    while (1) 
    {
			SevenSeg_DisplayACValue(contador);
			contador++;
			_delay_ms(500);
			if(contador>9) contador=0;
    }
}

