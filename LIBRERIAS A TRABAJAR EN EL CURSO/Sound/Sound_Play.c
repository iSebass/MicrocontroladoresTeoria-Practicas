#include "Sound_Play.h"
#include <util/delay.h>

// Variables globales para el puerto y pin del buzzer
static volatile uint8_t *sound_port;
static uint8_t sound_pin;

void Sound_Init(volatile uint8_t *port, uint8_t pin) {
	// Guardar las referencias al puerto y pin
	sound_port = port;
	sound_pin = pin;

	// Configurar el pin como salida
	*(port - 1) |= (1 << pin);  // *(port - 1) apunta al DDR correspondiente
}

// Función personalizada para retardos en microsegundos
void Delay_Custom(uint32_t delay_us) {
	for (uint32_t i = 0; i < delay_us; i++) {
		// NOP genera un retardo aproximado de 1 microsegundo por iteración
		asm volatile ("nop");
	}
}

void Sound_Play(uint16_t frequency, uint16_t duration_ms) {
	uint32_t period_us = 1000000UL / frequency;   // Período en microsegundos
	uint32_t half_period_us = period_us / 2;      // Media onda en microsegundos
	uint32_t cycles = (duration_ms * 1000UL) / period_us;  // Número de ciclos

	for (uint32_t i = 0; i < cycles; i++) {
		*sound_port ^= (1 << sound_pin);  // Alternar el estado del pin
		Delay_Custom(half_period_us);    // Retardo personalizado
	}

	// Asegurarse de que el pin esté apagado al final
	*sound_port &= ~(1 << sound_pin);
}