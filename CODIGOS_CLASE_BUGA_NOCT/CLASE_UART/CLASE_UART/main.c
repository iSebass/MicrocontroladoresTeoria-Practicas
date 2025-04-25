/*
 * CLASE_UART.c
 *
 * Created: 24/04/2025 08:05:19 p. m.
 * Author : USER
 */ 
#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include <stdio.h>
#include <string.h>
#include <string.h>

#include "UART/UART_LIB.h"
#include "ADC/ADC_LIB.h"


char bufferIn[20]=" ";
int idx = 0, banderaRx=0;

int conver;
float s1, s2, s3;


int main(void)
{
	DDRC |= (1<<PINC0) | (1<<PINC1) | (1<<PINC2) | (1<<PINC3) | (1<<PINC4);
    UART1_Init(9600);
	ADC_Init();
	
	UART1_Transmit_Text("HOLA COMPAÑEROS");

	UCSR0B |= (1<<RXCIE0);
	sei();

    while (1) 
    {
		
		conver = ADC_Read(0);
		s1 = ADC_Map(conver,0,307.0,0.0,150.0);
		
		conver = ADC_Read(1);
		s2 = ADC_Map(conver,0,1023.0,0,5.0);
		
		conver = ADC_Read(2);
		s3 = ADC_Map(conver,0,1023.0,0.0,1200.0);
		
		UART1_Transmit_Printf("* %f / %f / %f #",s1, s2, s3);
		UART1_Transmit_Text("\r\n");
		
		if(banderaRx == 1){
			
			//logica;
			if( strstr(bufferIn,"LED1ON" ) ){
				PORTC |= (1<<PINC0);
			}
			if( strstr(bufferIn,"LED1OFF" )  ){
				PORTC &= ~(1<<PINC0);
			}
			if( strstr(bufferIn,"LED2ON" )  ){
				PORTC |= (1<<PINC1);
			}
			if( strstr(bufferIn,"LED2OFF" )  ){
				PORTC &= ~(1<<PINC1);
			}
			if( strstr(bufferIn,"LED3ON" )  ){
				PORTC |= (1<<PINC2);
			}
			if( strstr(bufferIn,"LED3OFF" )  ){
				PORTC &= ~(1<<PINC2);
			}
			if( strstr(bufferIn,"MOTORDER" )  ){
				PORTC &= ~(1<<PINC3);
				PORTC |=  (1<<PINC4);
			}
			if( strstr(bufferIn,"MOTORIZQ" )  ){
				PORTC &= ~(1<<PINC4);
				PORTC |=  (1<<PINC3);
			}
			if( strstr(bufferIn,"MOTORPARAR" )  ){
				PORTC &= ~(1<<PINC4);
				PORTC &= ~(1<<PINC3);
			}
			
			//limpiar la memoria;
			memset(bufferIn,' ',14);
			banderaRx=0;
			idx=0;
			
		}
		
		_delay_ms(100);
    }
}

ISR(USART0_RX_vect){
	char letra = UART1_Read();
	if( letra != '#' ){
		bufferIn[idx]= letra;
		idx++;
	}
	else{
		banderaRx=1;
		
	}
}

