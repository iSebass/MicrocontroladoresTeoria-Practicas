#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>


int main(void)
{
	//CONFIGURAMOS LE PUERTO A DE SALIDA
    DDRA = 0xFF;
	
	//INICIAMOS EL PORT A TODO APAGADO
	PORTA = 0x00;
	

    while (1) 
    {
		PORTA = 0b00000001;
		_delay_ms(100);
		
		PORTA = 0b00000011;
		_delay_ms(100);
		
		PORTA = 0b00000111;
		_delay_ms(100);
		
		PORTA = 0b00001111;
		_delay_ms(100);
		
		PORTA = 0b00011111;
		_delay_ms(100);
		
		PORTA = 0b00111111;
		_delay_ms(100);
		
		PORTA = 0b01111111;
		_delay_ms(100);
		
		PORTA = 0b11111111;
		_delay_ms(100);
		
		//APAGAR LEDS
		
		
		PORTA = 0b11111110;
		_delay_ms(100);
		
		PORTA = 0b11111100;
		_delay_ms(100);
		
		PORTA = 0b11111000;
		_delay_ms(100);
		
		PORTA = 0b11110000;
		_delay_ms(100);
		
		PORTA = 0b11100000;
		_delay_ms(100);
		
		PORTA = 0b11000000;
		_delay_ms(100);
		
		PORTA = 0b10000000;
		_delay_ms(100);
		
		PORTA = 0b00000000;
		_delay_ms(100);
		
		
		
    }
}

