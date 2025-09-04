
#ifndef CONFIG_H_
#define CONFIG_H_

#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>

typedef unsigned char byte;

typedef union{
	struct{
		byte BIT0:  1;
		byte BIT1:  1;
		byte BIT2:  1;
		byte BIT3:  1;
		byte BIT4:  1;
		byte BIT5:  1;
		byte BIT6:  1;
		byte BIT7:  1;
	};
	struct{
		byte NibbleL: 4;
		byte NibbleH: 4;
	};
}REG8_t;

#define MACRO_MAPEO(STRUCT, SFR) (*(volatile STRUCT *)_SFR_MEM_ADDR(SFR) )

#define PORTDbits MACRO_MAPEO(REG8_t, PORTD)
#define PORTBbits MACRO_MAPEO(REG8_t, PORTB)
#define PORTCbits MACRO_MAPEO(REG8_t, PORTC)
#define PORTAbits MACRO_MAPEO(REG8_t, PORTA)

#define DDRDbits MACRO_MAPEO(REG8_t,  DDRD)
#define DDRBbits MACRO_MAPEO(REG8_t,  DDRB)
#define DDRCbits MACRO_MAPEO(REG8_t,  DDRC)
#define DDRAbits MACRO_MAPEO(REG8_t,  DDRA)

#define PINDbits MACRO_MAPEO(REG8_t, PIND)
#define PINBbits MACRO_MAPEO(REG8_t, PINB)
#define PINCbits MACRO_MAPEO(REG8_t, PINC)
#define PINAbits MACRO_MAPEO(REG8_t, PINA)



#endif /* CONFIG_H_ */