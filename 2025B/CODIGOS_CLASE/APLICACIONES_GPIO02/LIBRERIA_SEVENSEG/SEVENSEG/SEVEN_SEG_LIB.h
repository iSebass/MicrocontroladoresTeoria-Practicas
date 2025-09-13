#ifndef SEVEN_SEG_LIB_H_
#define SEVEN_SEG_LIB_H_

#include <avr/io.h>

#define SEGA_DDR   DDRD
#define SEGB_DDR   DDRB
#define SEGC_DDR   DDRB
#define SEGD_DDR   DDRB
#define SEGE_DDR   DDRB
#define SEGF_DDR   DDRB
#define SEGG_DDR   DDRB

#define SEGA_PORT   PORTD
#define SEGB_PORT   PORTB
#define SEGC_PORT   PORTB
#define SEGD_PORT   PORTB
#define SEGE_PORT   PORTB
#define SEGF_PORT   PORTB
#define SEGG_PORT   PORTB

#define SEGA   2
#define SEGB   0 
#define SEGC   7 
#define SEGD   3 
#define SEGE   4 
#define SEGF   5 
#define SEGG   6 

void SevenSegInit();
void DecoAC_(int val);
void DecoCC_(int val);



#endif /* SEVEN_SEG_LIB_H_ */