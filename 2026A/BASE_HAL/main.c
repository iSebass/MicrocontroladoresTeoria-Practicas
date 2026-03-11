#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "gpio.h"


int main(void)
{
    GPIO_PIN_MODE_PORTA(PORTA0, INPUT);
	GPIO_PIN_MODE_PORTA(PORTA5, OUTPUT);
    
	while (1) 
    {
		if(GPIO_READ_PORTA(PORTA0) ){
			GPIO_TOGGLE_PORTA(PORTA5);
			_delay_ms(100);
		}
		_delay_ms(100);
			
    }
}

