/*
 * Sound_Play.h
 *
 * Created: 01/01/2025 08:31:48 p. m.
 *  Author: USER
 */ 


#ifndef SOUND_PLAY_H_
#define SOUND_PLAY_H_


#include <avr/io.h>

// Prototipos de funciones
void Sound_Init(volatile uint8_t *port, uint8_t pin);
void Sound_Play(uint16_t frequency, uint16_t duration_ms);


#endif /* SOUND_PLAY_H_ */