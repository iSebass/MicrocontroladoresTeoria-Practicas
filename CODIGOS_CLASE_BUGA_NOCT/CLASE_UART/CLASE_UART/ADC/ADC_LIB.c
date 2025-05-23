
#include "ADC_LIB.h"

void ADC_Init(){
	//JUSTIFICAMOS A LA DERECHA
	ADMUX &= ~(1<<ADLAR);
	
	//DESHABILITAMO SEL AUTODISPARO
	ADCSRA &= ~(1<<ADATE);
	
	// SELECCIONAMOS EL VREF AVCC;
	ADMUX &= ~(1 << REFS1);
	ADMUX |=  (1 << REFS0);
	
	//DESHABILITAMOS EL CONVERSOR
	ADCSRA &= ~(1<<ADEN);
	
	//AJUSTAMOS EL PREESCALADOR  FOSC/DIV
	ADCSRA = ADCSRA & 0xF8;    //LIMPIAMOS LOS BITS DEL PREESCLASER B2B1B0
	ADCSRA = ADCSRA | 0b111; //AJUSTAMOS EL DIVISOR
	
	//CONFIGURAMOS TODOS LOS PINES COMO DIGITALES Y ASI EVITAR GASTO ENERGETICO;
	DIDR0 = 0x00;
}
int ADC_Read(uint8_t ch){
	//CONFIGURAMOS EL PIN COMO ANALOGICO
	DIDR0 |= (1<<ch);
	
	//SELECCIONAMOS EL CANAL
	ADMUX = (ADMUX & 0xE0) | (ch & 0x0F);
	
	//HABILITAMOS EL CONVERSOR
	ADCSRA |= (1<<ADEN);
	
	//INICIAMOS LA CONVERSION
	ADCSRA |= (1 << ADSC);

	// ESPERAMOS A QUE LA CONVERSION TERMINE
	while( (ADCSRA & (1<<ADSC)) !=0 );
	
	//DESHABILITAMOS EL CONVERSOR
	ADCSRA &= ~(1<<ADEN);
	
	//CONFIGURAMOS TODOS LOS PINES COMO DIGITALES Y ASI EVITAR GASTO ENERGETICO;
	DIDR0 = 0x00;
	
	//REGRESAMOS EL VALOR DE LA CONVERSION DE 10 BITS
	return ADC;
}


float ADC_Map(int conv1, float conv_min, float conv_max, float sal_min, float sal_max){
	
	float m;
	
	m = (sal_max-sal_min)/(conv_max-conv_min);
	
	return (m*conv1-m*conv_min+sal_min);
	
}
