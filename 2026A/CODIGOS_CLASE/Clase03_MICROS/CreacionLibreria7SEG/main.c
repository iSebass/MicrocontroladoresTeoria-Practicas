#include "Config.h"
#include "SevenSegLib/SevenSegLib.h"

char contador=0;

void SecuA(){
	PORTA = 0xAA;
	_delay_ms(300);
	PORTA = 0x55;
	_delay_ms(300);
}
void SecuB(){
	PORTA =0;
	for(int i=0; i<=7; i++){
		PORTA |= (1<<i);
		_delay_ms(100);
	}
	for(int i=0; i<=7; i++){
		PORTA &= ~(1<<i);
		_delay_ms(100);
	}
}


int main(void){
	DDRA = 0xFF;
	
	
	while(1){
		SecuB();
	}
}


/*
int main(void){
    
	
	
	//PIN_OUTPUT(DIR_DISPLAY, SEGA);
	//PIN_LOW(PORT_DISPLAY, SEGA);
	
    while(1){
		displayCC(contador);
		contador++;
		if (contador==10) contador=0;
		_delay_ms(200);
    }
}
*/
