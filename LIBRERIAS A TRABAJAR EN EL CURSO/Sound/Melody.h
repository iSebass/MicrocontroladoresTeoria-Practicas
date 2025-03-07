#ifndef MELODY_H_
#define MELODY_H_

#include <stdint.h>

// Definición de las notas musicales
#define NOTE_C4  261
#define NOTE_D4  294
#define NOTE_E4  329
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  493
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880

#define NOTE_C6  1047
#define NOTE_D6  1175
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_G6  1568
#define NOTE_A6  1760

// Definición de las notas musicales
#define NOTE_BB4 466
#define NOTE_EB4 311
#define NOTE_EB5 622
#define NOTE_GB4 370


#define NOTE_REST 0  // Silencio

// Prototipo de la función para reproducir la melodía
void PlayMelody(const uint16_t melody[][2], uint16_t notes);



/*
//JINGLE BELLS MELODY
const uint16_t jingle_bells_melody[][2] = {
	{NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_E4, 800},
	{NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_E4, 800},
	{NOTE_E4, 400}, {NOTE_G4, 400}, {NOTE_C4, 400}, {NOTE_D4, 400}, {NOTE_E4, 800},
	{NOTE_F4, 400}, {NOTE_F4, 400}, {NOTE_F4, 400}, {NOTE_F4, 400}, {NOTE_F4, 400},
	{NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_D4, 400}, {NOTE_D4, 400},
	{NOTE_E4, 400}, {NOTE_D4, 400}, {NOTE_G4, 800}
};

#define JINGLE_BELLS_NOTES (sizeof(jingle_bells_melody) / sizeof(jingle_bells_melody[0]))
*/


#endif /* MELODY_H_ */