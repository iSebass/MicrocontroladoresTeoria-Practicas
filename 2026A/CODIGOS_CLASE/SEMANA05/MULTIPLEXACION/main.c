#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "gpio.h"
#include "SEVENSEG/SEVENSEG_LIB.h"


int contador =0;

int main(void){
    displayInit();
	
    while(1){
		setDisplayCC(contador);
		(contador == 9) ? contador=0:contador++;
		_delay_ms(300);
    }
}

