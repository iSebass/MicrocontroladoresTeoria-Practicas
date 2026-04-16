#include "LCD_LIB.h"

static uint8_t _lcd_params;
static char _lcd_buffer_[_LCD_nCOL_+1];

void _write_nibble(uint8_t nibble);
void _send_byte(uint8_t value, uint8_t mode);


void _send_byte(uint8_t value, uint8_t mode){
	
	switch(mode){
		case _DATA_:    _LCD_PORT(_LCD_RS, HIGH); break;
		case _COMMAND_: _LCD_PORT(_LCD_RS, LOW);  break;
	}
	
	_write_nibble( (value & 0xF0) >> 4);
	_write_nibble( (value & 0x0F) >> 0);
	
}

void _write_nibble(uint8_t nibble){
	if( (nibble & 1) == 0 ) _LCD_PORT(_LCD_D4, LOW); else _LCD_PORT(_LCD_D4, HIGH);
	if( (nibble & 2) == 0 ) _LCD_PORT(_LCD_D5, LOW); else _LCD_PORT(_LCD_D5, HIGH);
	if( (nibble & 4) == 0 ) _LCD_PORT(_LCD_D6, LOW); else _LCD_PORT(_LCD_D6, HIGH);
	if( (nibble & 8) == 0 ) _LCD_PORT(_LCD_D7, LOW); else _LCD_PORT(_LCD_D7, HIGH);
	
	_LCD_PORT(_LCD_EN, LOW);
	_LCD_PORT(_LCD_EN, HIGH);
	_LCD_PORT(_LCD_EN, LOW);
	_delay_ms(0.3);
	
}

void lcd_write(uint8_t letra){
	_send_byte(letra, _DATA_);
}

void lcd_command(uint8_t cmd){
	_send_byte(cmd, _COMMAND_);
}

void lcd_init(void){
	//CONFIGURAR LOS PINES RS, EN, D4..D7 DE SALIDA
	_LCD_DDR(_LCD_RS, OUTPUT);
	_LCD_DDR(_LCD_EN, OUTPUT);
	_LCD_DDR(_LCD_D4, OUTPUT);
	_LCD_DDR(_LCD_D5, OUTPUT);
	_LCD_DDR(_LCD_D6, OUTPUT);
	_LCD_DDR(_LCD_D7, OUTPUT);
	
	//INICIALIZAMOS LOS PINES RS, EN D4..D7 EN CERO
	_LCD_PORT(_LCD_RS, LOW);
	_LCD_PORT(_LCD_EN, LOW);
	_LCD_PORT(_LCD_D4, LOW);
	_LCD_PORT(_LCD_D5, LOW);
	_LCD_PORT(_LCD_D6, LOW);
	_LCD_PORT(_LCD_D7, LOW);
	
	
	_delay_ms(15);
	_write_nibble(0x03);
	_delay_ms(5);
	_write_nibble(0x03);
	_delay_us(100);
	_write_nibble(0x03);
	_write_nibble(0x02);
	
	
	//CONFIGURAR LA LCD MODO 4 BITS 2 LINEAS 5x8 MATRIZ
	lcd_command(_LCD_FUNTIONSET | _LCD_4BITMODE | _LCD_5x8DOTS | _LCD_2LINE);
	_delay_us(37);
	
	//ENCENDER LA LCD
	_lcd_params = _LCD_DISPLAY_ON | _LCD_CURSOR_ON | _LCD_BLINK_ON;
	lcd_command(_LCD_DISPLAYCONTROL | _lcd_params);
	_delay_us(37);
	
	lcd_command(_LCD_CLEARDISPLAY);
	_delay_ms(2);
}

void  lcd_puts(char *str){
	while(*str){
		lcd_write(*str);
		str++;
	}
}

void lcd_set_cursor(uint8_t row, uint8_t col){
	static uint8_t local_mem[4]={0x00,0x40,0x14,0x54};
	lcd_command(_LCD_SET_DDRAM_ADDR | (local_mem[row-1]+(col-1)) );
}

void lcd_printf(char *str, ...){
	va_list args;
	va_start(args,str);
	vsnprintf(_lcd_buffer_, _LCD_nCOL_+1, str, args);
	va_end(args);
	lcd_puts(_lcd_buffer_);
}

void lcd_clear(void){
	lcd_command(_LCD_CLEARDISPLAY);
	_delay_ms(2);
}
void lcd_return_home(void){
	lcd_command(_LCD_RETURNHOME);
	_delay_ms(2);
}

void lcd_on(void){
	_lcd_params |=  _LCD_DISPLAY_ON;
	lcd_command(_LCD_DISPLAYCONTROL |_lcd_params);
	_delay_us(37);
}
void lcd_off(void){
	_lcd_params &=  ~_LCD_DISPLAY_ON;
	lcd_command(_LCD_DISPLAYCONTROL |_lcd_params);
	_delay_us(37);
}

void lcd_enable_blink(void){
	_lcd_params |= _LCD_BLINK_ON;
	lcd_command(_LCD_DISPLAYCONTROL |_lcd_params);
	_delay_us(37);
}


void lcd_disable_blink(void){
	_lcd_params &= ~_LCD_BLINK_ON;
	lcd_command(_LCD_DISPLAYCONTROL |_lcd_params);
	_delay_us(37);
}

void lcd_enable_cursor(void){
	_lcd_params |= _LCD_CURSOR_ON;
	lcd_command(_LCD_DISPLAYCONTROL |_lcd_params);
	_delay_us(37);
}
void lcd_disable_cursor(void){
	_lcd_params &= ~_LCD_CURSOR_ON;
	lcd_command(_LCD_DISPLAYCONTROL |_lcd_params);
	_delay_us(37);
}

void lcd_scroll_left(void){
	lcd_command(_LCD_CURSORDISPLAYSHIFT | _LCD_DISPLAY_SHIFT | _LCD_MOVELEFT);
	_delay_us(37);
}
void lcd_scroll_right(void){
	lcd_command(_LCD_CURSORDISPLAYSHIFT | _LCD_DISPLAY_SHIFT | _LCD_MOVERIGHT);
	_delay_us(37);
}

void lcd_custom_char(uint8_t mem, uint8_t *charmap){
	lcd_command(_LCD_SET_CGRAM_ADDR |  ((mem&0x07)<<3) );
	for(uint8_t i=0; i<=7; i++){
		lcd_write(charmap[i]);
	}
	lcd_command(_LCD_SET_DDRAM_ADDR);
	_delay_us(37);
}