#ifndef SEVEN_SEG_LIB_H_
#define SEVEN_SEG_LIB_H_

#include <avr/io.h>

#define SEGA_DDR   DDRB
#define SEGB_DDR   DDRB
#define SEGC_DDR   DDRB
#define SEGD_DDR   DDRB
#define SEGE_DDR   DDRB
#define SEGF_DDR   DDRB
#define SEGG_DDR   DDRB

#define SEGA_PORT   PORTB
#define SEGB_PORT   PORTB
#define SEGC_PORT   PORTB
#define SEGD_PORT   PORTB
#define SEGE_PORT   PORTB
#define SEGF_PORT   PORTB
#define SEGG_PORT   PORTB

#define SEGA   7
#define SEGB   1 
#define SEGC   2 
#define SEGD   3 
#define SEGE   4 
#define SEGF   5 
#define SEGG   6 

void SevenSegInit();
void DecoAC(int val);
void DecoCC(int val);



#endif /* SEVEN_SEG_LIB_H_ */