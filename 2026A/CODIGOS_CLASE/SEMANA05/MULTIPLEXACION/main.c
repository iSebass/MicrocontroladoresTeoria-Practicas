#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "gpio.h"
#include "SEVENSEG/SEVENSEG_LIB.h"


#define DISPLAY1  0b1110
#define DISPLAY2  0b1101
#define DISPLAY3  0b1011
#define DISPLAY4  0b0111

int contador =3846, unidad, decena, centena, unidadmil;
int residuo, result;
int contadorseg=0;

int main(void){
    displayInit();
	
	GPIO_PIN_MODE_PORTC(PINC0, OUTPUT);
	GPIO_PIN_MODE_PORTC(PINC1, OUTPUT);
	GPIO_PIN_MODE_PORTC(PINC2, OUTPUT);
	GPIO_PIN_MODE_PORTC(PINC3, OUTPUT);
	
    while(1){
		
		unidadmil = contador / 1000;
		residuo   = contador % 1000;
		centena   = residuo/100;
		residuo   = residuo%100;
		decena    = residuo/10;
		unidad    = residuo%10;
		
		GPIO_PORT_WRITE_C(DISPLAY1);
		setDisplayCC(unidadmil);
		_delay_ms(10);
		GPIO_PORT_WRITE_C(DISPLAY2);
		setDisplayCC(centena);
		_delay_ms(10);
		GPIO_PORT_WRITE_C(DISPLAY3);
		setDisplayCC(decena);
		_delay_ms(10);
		GPIO_PORT_WRITE_C(DISPLAY4);
		setDisplayCC(unidad);
		_delay_ms(10);
		
		contadorseg++;
		if(contadorseg>25){
			contador+=1;
			contadorseg=0;
		}
		
		
    }
}

