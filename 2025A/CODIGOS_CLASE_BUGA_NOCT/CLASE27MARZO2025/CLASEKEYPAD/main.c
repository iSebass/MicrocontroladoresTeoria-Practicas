#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

#include "LCD/LCD_LIB.h"
#include "KEYPAD/KEYPAD_LIB.h"

char tecla;

int main(void)
{
    lcd_init();
	lcd_disable_cursor();
	lcd_disable_blink();
	
	KeyPad_Init();
	
    while (1) 
    {
		tecla = KeyPad_KeyClick();
		lcd_set_cursor(1,1);
		lcd_printf("tecla: %c",tecla);
		_delay_ms(50);
    }
}

