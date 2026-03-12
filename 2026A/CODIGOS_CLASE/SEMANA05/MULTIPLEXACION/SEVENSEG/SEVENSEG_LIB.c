#include "SEVENSEG_LIB.h"

const char sevenSegCC[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
const char sevenSegCA[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
	
void displayInit(void){
	CONFIG_DISPLAY(sega,  OUTPUT);
	CONFIG_DISPLAY(segb,  OUTPUT);
	CONFIG_DISPLAY(segc,  OUTPUT);
	CONFIG_DISPLAY(segd,  OUTPUT);
	CONFIG_DISPLAY(sege,  OUTPUT);
	CONFIG_DISPLAY(segf,  OUTPUT);
	CONFIG_DISPLAY(segg,  OUTPUT);
	CONFIG_DISPLAY(segdp, OUTPUT);	
}

void setDisplayCC(int value){
	 if( (sevenSegCC[value] &   1) != 0 )   WRITE_DISPLAY(sega,  HIGH); else WRITE_DISPLAY(sega,  LOW);
	 if( (sevenSegCC[value] &   2) != 0 )   WRITE_DISPLAY(segb,  HIGH); else WRITE_DISPLAY(segb,  LOW);
	 if( (sevenSegCC[value] &   4) != 0 )   WRITE_DISPLAY(segc,  HIGH); else WRITE_DISPLAY(segc,  LOW);
	 if( (sevenSegCC[value] &   8) != 0 )   WRITE_DISPLAY(segd,  HIGH); else WRITE_DISPLAY(segd,  LOW);
	 if( (sevenSegCC[value] &  16) != 0 )   WRITE_DISPLAY(sege,  HIGH); else WRITE_DISPLAY(sege,  LOW);
	 if( (sevenSegCC[value] &  32) != 0 )   WRITE_DISPLAY(segf,  HIGH); else WRITE_DISPLAY(segf,  LOW);
	 if( (sevenSegCC[value] &  64) != 0 )   WRITE_DISPLAY(segg,  HIGH); else WRITE_DISPLAY(segg,  LOW);
	 if( (sevenSegCC[value] & 128) != 0 )   WRITE_DISPLAY(segdp, HIGH); else WRITE_DISPLAY(segdp, LOW);
	 
}
void setDisplayAC(int value){
	if( (sevenSegCA[value] &   1) != 0 )   WRITE_DISPLAY(sega,  HIGH); else WRITE_DISPLAY(sega,  LOW);
	if( (sevenSegCA[value] &   2) != 0 )   WRITE_DISPLAY(segb,  HIGH); else WRITE_DISPLAY(segb,  LOW);
	if( (sevenSegCA[value] &   4) != 0 )   WRITE_DISPLAY(segc,  HIGH); else WRITE_DISPLAY(segc,  LOW);
	if( (sevenSegCA[value] &   8) != 0 )   WRITE_DISPLAY(segd,  HIGH); else WRITE_DISPLAY(segd,  LOW);
	if( (sevenSegCA[value] &  16) != 0 )   WRITE_DISPLAY(sege,  HIGH); else WRITE_DISPLAY(sege,  LOW);
	if( (sevenSegCA[value] &  32) != 0 )   WRITE_DISPLAY(segf,  HIGH); else WRITE_DISPLAY(segf,  LOW);
	if( (sevenSegCA[value] &  64) != 0 )   WRITE_DISPLAY(segg,  HIGH); else WRITE_DISPLAY(segg,  LOW);
	if( (sevenSegCA[value] & 128) != 0 )   WRITE_DISPLAY(segdp, HIGH); else WRITE_DISPLAY(segdp, LOW);
}