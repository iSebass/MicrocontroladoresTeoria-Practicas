#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

#include "LCD/LCD_LIB.h"


char customChar0[] = {
	0b01110,
	0b01110,
	0b01010,
	0b01110,
	0b11011,
	0b11111,
	0b10101,
	0b00000
};
char customChar1[] = {
	0b01110,
	0b11011,
	0b10001,
	0b10001,
	0b10001,
	0b10001,
	0b11111,
	0b00000
};


int main(void)
{
    lcd_init();
	lcd_disable_blink();
	lcd_disable_cursor();
	
	lcd_custom_char(0, customChar0);
	lcd_custom_char(1, customChar1);
	
	lcd_puts("Iniciando");
	for(int i=0; i<=5; i++){
		_delay_ms(300);
		lcd_write('.');
		
	}
	lcd_clear();
	
	
	
    while (1) 
    {
		
		lcd_set_cursor(3,5);
		lcd_write(0);
		lcd_puts(" iSebas ");
		lcd_write(1);
		
		for(int i=0; i<=4; i++){
			_delay_ms(300);
			lcd_scroll_right();
		}
		for(int i=0; i<=4; i++){
			_delay_ms(300);
			lcd_scroll_left();
		}
		
		
    }
}

