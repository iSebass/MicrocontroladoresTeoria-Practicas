#include "SEVEN_SEG_LIB.h"

char DecoCC[10]={191, 134, 219, 207, 230, 237, 253, 135, 255, 231};
char DecoAC[10]={64, 121, 36, 48, 25, 18, 2, 120, 0, 24};


void SevenSegInit(){
	SEGA_DDR |= (1<<SEGA);
	SEGB_DDR |= (1<<SEGB);
	SEGC_DDR |= (1<<SEGC);
	SEGD_DDR |= (1<<SEGD);
	SEGE_DDR |= (1<<SEGE);
	SEGF_DDR |= (1<<SEGF);
	SEGG_DDR |= (1<<SEGG);	
}

void DecoAC_(int val){
	
	
}
void DecoCC_(int val){
	
	if( (DecoCC[val] & 1 ) == 0) SEGA_PORT &= ~(1<<SEGA); else SEGA_PORT |= (1<<SEGA);
	if( (DecoCC[val] & 2 ) == 0) SEGB_PORT &= ~(1<<SEGB); else SEGB_PORT |= (1<<SEGB);
	if( (DecoCC[val] & 4 ) == 0) SEGC_PORT &= ~(1<<SEGC); else SEGC_PORT |= (1<<SEGC);
	if( (DecoCC[val] & 8 ) == 0) SEGD_PORT &= ~(1<<SEGD); else SEGD_PORT |= (1<<SEGD);
	if( (DecoCC[val] & 16) == 0) SEGE_PORT &= ~(1<<SEGE); else SEGE_PORT |= (1<<SEGE);
	if( (DecoCC[val] & 32) == 0) SEGF_PORT &= ~(1<<SEGF); else SEGF_PORT |= (1<<SEGF);
	if( (DecoCC[val] & 64) == 0) SEGG_PORT &= ~(1<<SEGG); else SEGG_PORT |= (1<<SEGG);
	
}
