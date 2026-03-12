#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "gpio.h"
#include "SEVENSEG/SEVENSEG_LIB.h"


int main(void){
    displayInit();
	setDisplayCC(5);
    while(1){
		
    }
}

