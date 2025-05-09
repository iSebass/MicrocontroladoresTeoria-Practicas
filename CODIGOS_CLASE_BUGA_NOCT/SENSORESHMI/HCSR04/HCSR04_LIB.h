
/*
 * HCSR04_LIB.h
 *
 * Created: 12/09/2024 08:40:50 p. m.
 *  Author: USER
 */ 


#ifndef HCSR04_LIB_H_
#define HCSR04_LIB_H_


#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

// Definición de pines para Trigger y Echo;
#define TRIG_DDR   DDRB
#define TRIG_PORT  PORTB
#define TRIG_PIN   PINB2

#define ECHO_DDR   DDRB
#define ECHO_PIN   PINB
#define ECHO_INPUT PINB3

// Funciones de la librería;
void HCSR04_init(void);
void HCSR04_trigger(void);
uint16_t HCSR04_readDistance(void);



#endif /* HCSR04_LIB_H_ */
