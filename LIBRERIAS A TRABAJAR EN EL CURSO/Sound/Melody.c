#include "Melody.h"
#include "Sound_Play.h"

#include <util/delay.h>

void PlayMelody(const uint16_t melody[][2], uint16_t notes) {
	for (uint16_t i = 0; i < notes; i++) {
		uint16_t frequency = melody[i][0];  // Frecuencia de la nota
		uint16_t duration = melody[i][1];  // Duración de la nota

		if (frequency == NOTE_REST) {
			// Pausa para silencios
			for (uint16_t j = 0; j < duration; j++) {
				_delay_ms(1);
			}
			} else {
			// Reproducir la nota
			Sound_Play(frequency, duration);
		}

		// Pausa entre notas para separación
		_delay_ms(50);
	}
}