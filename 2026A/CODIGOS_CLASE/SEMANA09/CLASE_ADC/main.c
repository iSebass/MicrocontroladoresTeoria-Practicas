#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "LIBS/LCD_LIB.h"

#include "hal/adc.h"

uint16_t conv, mvTemp; 
float  temp;

int main(void)
{
	lcd_init();
	lcd_disable_blink();
	lcd_disable_cursor();
	
	hal_adc_config_t config_adc = {
		         .reference = HAL_ADC_REF_AVCC,
		         .prescaler = HAL_ADC_PRESCALER_128,
		         .vref_mv   = 5000U,
		         .use_irq   = HAL_ADC_MODE_POLLING
	};
	HAL_ADC_Init(&config_adc);
	
		
	while (1)
	{
		//ADQUI DATA SENSORES
		HAL_ADC_ReadMillivolts(3, &conv);
		HAL_ADC_ReadMillivolts(2, &mvTemp);
		
		//ADECUACION DE LA SENSOR
		temp = (150.0/1500.0)*mvTemp;
		
		lcd_set_cursor(1,1);
		lcd_printf("CONV: %4d mV", conv); 
		lcd_set_cursor(2,1);
		lcd_printf("TEMP: %5.1f °C", temp);
		
		_delay_ms(100);
	}
}