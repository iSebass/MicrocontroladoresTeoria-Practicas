#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

#include "SEVENSEG/SEVENSEG_LIB.h"

#define te 10

int contador=0, mm=59, ss=35;



int main(void)
{
	
	SevenSeg_Init();
	DDRC = 0x0F;
	
	while (1)
	{
		PORTC = (1<<0);
		SevenSeg_DisplayACValue(mm/10);
		_delay_ms(te);
		
		PORTC = (1<<1);
		SevenSeg_DisplayACValue(mm%10);
		_delay_ms(te);
		
		PORTC = (1<<2);
		SevenSeg_DisplayACValue(ss/10);
		_delay_ms(te);
		
		PORTC = (1<<3);
		SevenSeg_DisplayACValue(ss%10);
		_delay_ms(te);
		
		
		contador++;
		if(contador==25){
			contador=0;
			ss++;
		}
		
		
		
		if(ss>59){
			ss=0;
			mm++;
		}
		if(mm>59){
			 mm=0;
		}
		
		
		
		
	}
}

