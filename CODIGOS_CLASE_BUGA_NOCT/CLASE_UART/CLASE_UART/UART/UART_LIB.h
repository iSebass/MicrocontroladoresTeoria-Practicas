/*
 * UART_LIB.h
 *
 * Created: 23/04/2025 07:35:54 p. m.
 *  Author: USER
 */ 


#ifndef UART_LIB_H_
#define UART_LIB_H_

#define F_CPU 20000000UL
#include <avr/io.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

void UART1_Init(unsigned long baudrate);
void UART1_Transmit_Char(unsigned char data);
void UART1_Transmit_Text(char *text);
void UART1_Transmit_Printf(char *str, ...);

char UART1_Read(void);
bool UART1_DataAvailable(void);


#endif /* UART_LIB_H_ */