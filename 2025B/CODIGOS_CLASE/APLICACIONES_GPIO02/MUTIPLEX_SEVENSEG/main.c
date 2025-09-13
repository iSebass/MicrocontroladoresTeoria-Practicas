#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

#include "SEVENSEG/SEVEN_SEG_LIB.h"

#define te 10

#define  UNIDADES     ~(1<<3)
#define  DECENAS      ~(1<<2)
#define  CENTENAS     ~(1<<1)
#define  UNIDADES_MIL ~(1<<0)

int numero = 2108;
int dig_unidad, dig_decena, dig_centena, dig_unidad_mil;
int resi;

int main(void){
    for(int i=0; i<=3; i++){
		DDRD |= (1<<i);
	}
	SevenSegInit();

    while(1){
		dig_unidad_mil = numero / 1000;
		resi = numero%1000;
		
		dig_centena = resi / 100;
		resi = resi % 100;
		
		dig_decena = resi / 10;
		dig_unidad = resi % 10;
		
		PORTD = UNIDADES_MIL;
		DecoCC_(dig_unidad_mil);
		_delay_ms(te);
		
		PORTD = CENTENAS;
		DecoCC_(dig_centena);
		_delay_ms(te);
		
		PORTD = DECENAS;
		DecoCC_(dig_decena);
		_delay_ms(te);
		
		PORTD = UNIDADES;
		DecoCC_(dig_unidad);
		_delay_ms(te);
		
		
		
    }
}

