#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>


int main(void){
    DDRA  = 0b00000001; // RA0 como SALIDA
	
	
    while(1){
		
		PORTA = 0b00000001;
		_delay_ms(500);
		PORTA = 0b00000000;
		_delay_ms(500);
		
    }
}

