#include "UART_LIB.h"


static char _uart_buffer_[50];

void UART1_Init(unsigned long baudrate){	
	unsigned int ubrr_val = F_CPU/(16*baudrate)-1;
	UBRR0H = (unsigned char) (ubrr_val>>8);
	UBRR0L = (unsigned char) (ubrr_val);
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);	
}
void UART1_Transmit_Char(unsigned char data){
	while(!( UCSR0A & (1<<UDRE0) ) );
	UDR0 = data;
}

void UART1_Transmit_Text(char *text){
	while(*text){
		UART1_Transmit_Char(*text);
		text++;
	}
}

void UART1_Transmit_Printf(char *str, ...){
	va_list args;
	va_start(args,str);
	vsnprintf(_uart_buffer_, 51, str, args);
	va_end(args);
	UART1_Transmit_Text(_uart_buffer_);
}

bool UART1_DataAvailable(void){
	return ( UCSR0A &(1<<RXC0) );
}


char UART1_Read(void){
	while(!( UCSR0A & (1<<RXC0) ) );
	return UDR0;
}
