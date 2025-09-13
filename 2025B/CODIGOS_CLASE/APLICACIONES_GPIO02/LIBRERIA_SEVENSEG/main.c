#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

#include "SEVENSEG/SEVEN_SEG_LIB.h"

int contador=0;

int main(void)
{
    SevenSegInit();
	
    while (1) 
    {
		DecoCC_(contador);
		contador += 1;
		contador = contador>9 ? 0 : contador;
		_delay_ms(500);
    }
}

