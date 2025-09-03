
#include "HCSR04_LIB.h"

void HCSR04_init(void) {
	// Configura el pin de Trigger como salida;
	TRIG_DDR |= (1 << TRIG_PIN);
	// Configura el pin de Echo como entrada;
	ECHO_DDR &= ~(1 << ECHO_INPUT);
}

void HCSR04_trigger(void) {
	// Genera un pulso de 10 microsegundos en el pin Trigger;
	TRIG_PORT &= ~(1 << TRIG_PIN); // Asegúrate de que Trigger está bajo;
	_delay_us(2);
	TRIG_PORT |= (1 << TRIG_PIN);  // Activa el pulso;
	_delay_us(10);                 // Pulso de 10 microsegundos;
	TRIG_PORT &= ~(1 << TRIG_PIN); // Desactiva el pulso;
}

uint16_t HCSR04_readDistance(void) {
	uint16_t duration = 0;

	// Dispara el pulso ultrasónico;
	HCSR04_trigger();

	// Espera hasta que el pin Echo se ponga alto (comienzo del eco);
	while (!(ECHO_PIN & (1 << ECHO_INPUT)));

	// Mide el ancho del pulso de Echo;
	while (ECHO_PIN & (1 << ECHO_INPUT)) {
		_delay_us(58);
		duration++;
	}

	// Calcula la distancia (en centímetros);
	// Fórmula: distancia (cm) = (duración * velocidad del sonido (343 m/s) / 2) / 10000;
	// La fórmula simplificada es: distancia (cm) = (duración / 58);
	uint16_t distance = duration;

	// Devuelve la distancia medida;
	return distance;
}
