#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#include "ADC/ADC_LIB.h"
#include "LCD/LCD_LIB.h"

int conver;
float lm35, mcp9700, pot1, pot2;

int main(void)
{
	lcd_init();
	lcd_disable_cursor();
	lcd_disable_blink();
	
    ADC_init();
	
	while (1) 
    {
		conver = ADC_read(A0);
		lm35 = 150.0/307.0*conver;
		
		lcd_set_cursor(1,1);
		lcd_printf("Lm35: %5.1f", lm35);
		_delay_ms(100);
		
    }
}

