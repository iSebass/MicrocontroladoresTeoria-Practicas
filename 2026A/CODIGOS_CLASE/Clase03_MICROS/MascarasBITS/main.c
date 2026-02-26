/*
 * MascarasBITS.c
 *
 * Created: 25/02/2026 07:49:33 p. m.
 * Author : USER
 */ 
#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "Config.h"


int main(void){
    
	// LED PB0 como salida
	PIN_OUTPUT(DDRA, 0);
	

	
	while(1){
		PIN_LOW(PORTA, 0);
		_delay_ms(500);
		PIN_HIGH(PORTA,0);
		_delay_ms(500);
		
			
    }
}



