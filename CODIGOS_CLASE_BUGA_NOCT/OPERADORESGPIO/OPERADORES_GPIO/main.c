#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>

#define BTNA 0
#define BTNB 1

#define getStatusBTNA()   PINB & (1<<BTNA)
#define getStatusBTNB()   PINB & (1<<BTNB)

int main(void){
    DDRA = 0xFF;
	DDRB &= ~( (1<<BTNA) | (1<<BTNB) );
	
	PORTA = 0x00;
	
	//PONER A UNO UN BIT
	//PORTA |=  (1<<2) | (1<<3) | (1<<7) ;
	
	//PORTA &= ~( (1<<2) | (1<<3) | (1<<7) );
	
    while(1){
		if( getStatusBTNA() != 0 ){
			PORTA |= (1<<6);
		}
		else{
			PORTA  &= ~(1<<6);
		}
		
		if( ( PINB & (1<<BTNB) ) != 0 ){
			PORTA |= (1<<7);
		}
		else{
			PORTA  &= ~(1<<7);
		}
    }
}

