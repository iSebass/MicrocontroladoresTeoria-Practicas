#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "LIBS/LCD_LIB.h"


int hh=21, mm=5, ss=52;

char fig1[] = {
	0b00100,
	0b01110,
	0b10001,
	0b01110,
	0b01110,
	0b10001,
	0b10001,
	0b00000
};

int main(void)
{
    lcd_init();
	lcd_disable_blink();
	
	lcd_custom_char(0,  fig1);
	
	lcd_puts("Clock");
	lcd_set_cursor(4,1);
	lcd_printf("%02d : %02d : %02d", hh, mm, ss);
	
	lcd_set_cursor(3,3);
	lcd_write(0);
	
	
	
    while (1) 
    {
		
    }
}

