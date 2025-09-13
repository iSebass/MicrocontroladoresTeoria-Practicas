#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

#define getBtnUP()      ( PINA & (1<<0) ) 
#define getBtnDown()    ( PINA & (1<<1) ) 

extern char DecoCC[10]={191, 134, 219, 207, 230, 237, 253, 135, 255, 231};
extern char DecoAC[10]={64, 121, 36, 48, 25, 18, 2, 120, 0, 24};
	
int contador=0;

int main(void)
{
    DDRB = 0xFF;
	DDRD = (1<<0)|(1<<1);
	
	DDRA &= ~( (1<<0) | (1<<1) );
	
	
    while (1) 
    {
		if( getBtnDown() == 0 ){
			while( getBtnDown() == 0 );
			contador--;
		}
		if( getBtnUP() == 0 ){
			while( getBtnUP() == 0 );
			contador++;
		}
		
		PORTB = DecoCC[contador];
		
		if( contador > 9 ) contador=0;
		if( contador < 0 ) contador = 9;
		
    }
}

