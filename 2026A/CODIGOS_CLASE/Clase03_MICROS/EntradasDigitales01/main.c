#include "Config.h"


int main(void)
{
    //CONFIGURAMOS PUERTO B COMO ENTRADA
	PIN_INPUT(PORTB,0);
	
	//CONFIGURAR EL PUERTO A COMO SALIDA
	DDRA = 0xFF;
	
	//LIMPIAMOS EL PORTA ANTES DE COMENZAR
	PORTA = 0x00;
	
    while (1) 
    {
			if( PIN_READ(PINB, 0)  ){
				PIN_HIGH(PORTA, 0);
			}
			else{
				PIN_LOW(PORTA,0);
			}
		
    }
}

