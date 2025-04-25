/*
 * KEYPAD_LIB.h
 *
 * Created: 12/03/2025 08:28:51 p. m.
 *  Author: USER
 */ 


#ifndef KEYPAD_LIB_H_
#define KEYPAD_LIB_H_

#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

#define KEYPAD_DDR   DDRA
#define KEYPAD_PORT  PORTA
#define KEYPAD_PIN   PINA

#define KEYPAD_F1  0
#define KEYPAD_F2  1
#define KEYPAD_F3  2
#define KEYPAD_F4  3

#define KEYPAD_C1  4
#define KEYPAD_C2  5
#define KEYPAD_C3  6
#define KEYPAD_C4  7

#define getStatusC1    ( KEYPAD_PIN & (1<<KEYPAD_C1) )
#define getStatusC2    ( KEYPAD_PIN & (1<<KEYPAD_C2) )
#define getStatusC3    ( KEYPAD_PIN & (1<<KEYPAD_C3) )
#define getStatusC4    ( KEYPAD_PIN & (1<<KEYPAD_C4) )

void KeyPad_Init();
char KeyPad_KeyPress();
char KeyPad_KeyClick();



#endif /* KEYPAD_LIB_H_ */