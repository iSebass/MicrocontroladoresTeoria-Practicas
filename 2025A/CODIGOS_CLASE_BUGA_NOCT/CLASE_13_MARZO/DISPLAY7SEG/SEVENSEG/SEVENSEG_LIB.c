

#include "SEVENSEG_LIB.h"

char decoAC[10]={
	64,   //0
	121,  //1
	36,   //2
	48,	  //3
	25,   //4
	18,   //5
	2,    //6
	120,  //7
	0,    //8
	24    //9
};


void SevenSeg_Init(void){
	 DISPLAYDDR |= (1<<SEGA)|
				   (1<<SEGB)|
				   (1<<SEGC)|
				   (1<<SEGD)|
				   (1<<SEGE)|
				   (1<<SEGF)|
				   (1<<SEGG);
}

void SevenSeg_DisplayCCValue(char val){
	
}

void SevenSeg_DisplayACValue(char val){
	
	if( (decoAC[val] & 1  ) != 0) DISPLAYPORT |= (1<<SEGA); else DISPLAYPORT &= ~(1<<SEGA);
	if( (decoAC[val] & 2  ) != 0) DISPLAYPORT |= (1<<SEGB); else DISPLAYPORT &= ~(1<<SEGB);
	if( (decoAC[val] & 4  ) != 0) DISPLAYPORT |= (1<<SEGC); else DISPLAYPORT &= ~(1<<SEGC);
	if( (decoAC[val] & 8  ) != 0) DISPLAYPORT |= (1<<SEGD); else DISPLAYPORT &= ~(1<<SEGD);
	if( (decoAC[val] & 16 ) != 0) DISPLAYPORT |= (1<<SEGE); else DISPLAYPORT &= ~(1<<SEGE);
	if( (decoAC[val] & 32 ) != 0) DISPLAYPORT |= (1<<SEGF); else DISPLAYPORT &= ~(1<<SEGF);
	if( (decoAC[val] & 64 ) != 0) DISPLAYPORT |= (1<<SEGG); else DISPLAYPORT &= ~(1<<SEGG);
	
	
}