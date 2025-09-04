#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "Config.h"


void SecuenciaA();
void SecuenciaB();
void SecuenciaC();

enum{
	secA=0,
	secB,
	secC
};

int currentS=secC;


int main(void){
    
	//CONFIGURAR EL PUERTO
	DDRD  = 0xFF;
	PORTD = 0x00;  //0011 0101
	
	DDRAbits.BIT0 = 0; // RA0 COMO ENTRADA

    while(1){	
		
		switch(currentS){
			case secA: SecuenciaA(); break;
			case secB: SecuenciaB(); break;
			case secC: SecuenciaC(); break;
		}
		
		if(PINAbits.BIT0 == 1){
			
			while(PINAbits.BIT0 == 1);
			
			
			currentS++;
			if(currentS>secC) currentS=secA;
		}
		
    }
}



void SecuenciaA(){
	for(int i=0; i<=3; i++){
		PORTD |= (1<<i)|(1<< (7-i) );
		_delay_ms(100);
	}
	for(int i=0; i<=3; i++){
		PORTD &= ~ ( (1<<i)|(1<< (7-i) ) );
		_delay_ms(100);
	}
}
void SecuenciaB(){
	PORTD = 0b10101010;
	_delay_ms(100);
	PORTD = 0b01010101;
	_delay_ms(100);
}
void SecuenciaC(){
	PORTD = 0x00;
	PORTDbits.BIT0 = 1;
	_delay_ms(100);
	PORTDbits.BIT1 = 1;
	_delay_ms(100);
	PORTDbits.BIT2 = 1;
	_delay_ms(100);
	PORTDbits.BIT3 = 1;
	_delay_ms(100);
	PORTDbits.BIT4 = 1;
	_delay_ms(100);
	PORTDbits.BIT5 = 1;
	_delay_ms(100);
	PORTDbits.BIT6 = 1;
	_delay_ms(100);
	PORTDbits.BIT7 = 1;
	_delay_ms(100);
	
	PORTDbits.BIT0 = 0;
	_delay_ms(100);
	PORTDbits.BIT1 = 0;
	_delay_ms(100);
	PORTDbits.BIT2 = 0;
	_delay_ms(100);
	PORTDbits.BIT3 = 0;
	_delay_ms(100);
	PORTDbits.BIT4 = 0;
	_delay_ms(100);
	PORTDbits.BIT5 = 0;
	_delay_ms(100);
	PORTDbits.BIT6 = 0;
	_delay_ms(100);
	PORTDbits.BIT7 = 0;
	_delay_ms(100);
	
}


