#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

#include "SEVENSEG/SEVENSEG_LIB.h"

#define te 10

int contador=2576, u, d, c, m, resi;


void multiplex_display(){
	PORTC = (1<<0);
	SevenSeg_DisplayACValue(m);
	_delay_ms(te);
	
	PORTC = (1<<1);
	SevenSeg_DisplayACValue(c);
	_delay_ms(te);
	
	PORTC = (1<<2);
	SevenSeg_DisplayACValue(d);
	_delay_ms(te);
	
	PORTC = (1<<3);
	SevenSeg_DisplayACValue(u);
	_delay_ms(te);
}



int main(void)
{
	
	SevenSeg_Init();
	DDRC = 0x0F;
	DDRB &= ~( (1<<0) | (1<<1) );
	
	while (1)
	{
		multiplex_display();
		
		if( ( PINB & (1<<0) ) ){
			while( ( PINB & (1<<0) ) ){
				multiplex_display();
			}
			contador++;
		}
		if( ( PINB & (1<<1) ) ){
			while( ( PINB & (1<<1) ) ){
				multiplex_display();
			}
			contador--;
		}
		
		if(contador > 9999){
			contador = 9999;
		}
		
		if(contador < 0){
			contador = 0;
		}
		
		m = contador / 1000;
		resi = contador % 1000;
		c    = resi /100;
		resi = resi % 100;
		
		d = resi / 10;
		u = resi % 10;
		
		
		
	}
}

