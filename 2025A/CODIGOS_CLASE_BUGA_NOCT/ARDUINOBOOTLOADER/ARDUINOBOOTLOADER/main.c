#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>


int main(void)
{
    DDRB = 0xFF;
	
	
    while (1) 
    {
		PORTB = 0b00100000;
		_delay_ms(500);
		PORTB = 0b00000000;
		_delay_ms(500);
    }
}

