#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "LCD/LCD_LIB.h"

int contador=0;

int main(void)
{
	DDRD &= ~(1<<2);
	lcd_init();
	lcd_disable_blink();
	lcd_disable_cursor();
	
	//configuramos la interrupcion
	EICRA &= ~(1<<ISC00);
	EICRA |=  (1<<ISC01);
	EIMSK |= (1<<INT0);
	
	sei(); 
	
    while (1) 
    {
		
		lcd_set_cursor(1,1);
		lcd_printf("Contador: %d", contador );
		_delay_ms(2000);
    }
}

ISR( INT0_vect ){
	contador++;
}
