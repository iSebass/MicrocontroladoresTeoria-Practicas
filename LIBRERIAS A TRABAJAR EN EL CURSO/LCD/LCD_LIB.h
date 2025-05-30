#ifndef _LCD_LIB
#define _LCD_LIB


#define F_CPU 20000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdarg.h>


/************************************************************************/
/* Comportamiento de RS                                                 */
/************************************************************************/
#define _COMMAND_      0
#define _DATA_         1

/************************************************************************/
/* DEFINIR EL PUERTO DONDE SE VA CONECTAR LA LCD                        */
/************************************************************************/

#define _LCD_DDR    DDRD
#define _LCD_PORT   PORTD

#define _LCD_RS 0
#define _LCD_EN 1
#define _LCD_D4 2
#define _LCD_D5 3
#define _LCD_D6 4
#define _LCD_D7 5

#if( !defined _LCD_DDR || !defined _LCD_PORT )
	#warning "Puerto de la LCD asignado por defecto PORTD"
	#define _LCD_DDR    DDRD
	#define _LCD_PORT   PORTD
#endif

#ifndef _LCD_RS
	#warning "SE ASIGNAN PIN \" RS \" POR DEFECTO EN EL PIN 0"
	#define _LCD_RS    0
#endif

#ifndef _LCD_EN
	#warning "SE ASIGNAN PIN \" EN \"  POR DEFECTO EN EL PIN 1"
	#define _LCD_EN    1
#endif

#ifndef _LCD_D4
	#warning "SE ASIGNAN PIN \" D4 \" POR DEFECTO EN EL PIN 1"
	#define _LCD_D4    2
#endif

#ifndef _LCD_D5
	#warning "SE ASIGNAN PIN \" D5 \" POR DEFECTO EN EL PIN 3"
	#define _LCD_D5   3
#endif

#ifndef _LCD_D6
	#warning "SE ASIGNAN PIN \" D6 \" POR DEFECTO EN EL PIN 4"
	#define _LCD_D6   4
#endif

#ifndef _LCD_D7
	#warning "SE ASIGNAN PIN \" D7 \" POR DEFECTO EN EL PIN 5"
	#define _LCD_D7   5
#endif

#define _LCD_nCOL_       20
#define _LCD_nROW_       4

/************************************************************************/
/*  MODO CELAR DISPLAY:  D7 D6 D5 D4 D3 D2 D1 D0                        */
/*                       0  0  0  0  0  0  0  1                         */
/************************************************************************/
#define _LCD_CLEARDISPLAY 0x01

/************************************************************************/
/*  MODO CELAR RETURN HOME:  D7 D6 D5 D4 D3 D2 D1 D0                    */
/*                           0  0  0  0  0  0  1  0                     */
/************************************************************************/
#define _LCD_RETURNHOME     0x02

/************************************************************************/
/*      ENTRAMOS EN MODE SET:  D7 D6 D5 D4 D3 D2 D1  D0                 */
/*      					   0  0  0  0  0  1  I/D  S                 */
/*----------------------------------------------------------------------*/
/*      I/D = 1: Inc                                                    */
/*		      0: Dec                                                    */
/*		S   = 1: SHIFT ON                                               */
/*            0: SHIFT OFF                                              */
/************************************************************************/
#define _LCD_ENTRYMODESET   0x04
#define _LCD_INCREMENT      0x02
#define _LCD_DECREMENT      0x00
#define _LCD_SHIFT_ON       0x01
#define _LCD_SHIFT_OFF      0x00

/************************************************************************/
/*      ENTRAMOS EN DISPLAY CONTROL:  D7 D6 D5 D4  D3 D2 D1 D0          */
/*      						      0  0  0  0   1  D  U  B           */
/*----------------------------------------------------------------------*/
/*      D   = 1: DISPLAY ON                                             */
/*		      0: DISPLAY OFF                                            */
/*		U   = 1: CURSOR ON                                              */
/*		      0: CURSOR OFF                                             */
/*		B   = 1: BLINK                                                  */
/*		      0: NO BLINK                                               */
/************************************************************************/
#define _LCD_DISPLAYCONTROL 0x08
#define _LCD_DISPLAY_ON     0x04
#define _LCD_DISPLAY_OFF    0x00
#define _LCD_CURSOR_ON      0x02
#define _LCD_CURSOR_OFF     0x00
#define _LCD_BLINK_ON       0x01
#define _LCD_BLINK_OFF      0x00

/************************************************************************/
/* ENTRAMOS EN CURSOR OR DISPLAY SHIFT:  D7 D6 D5 D4  D3 D2  D1 D0      */
/*      						         0  0  0  1  S/C R/L  *  *      */
/*----------------------------------------------------------------------*/
/*      S/C = 1: display shift                                          */
/*		      0: cursor move                                            */
/*		R/L = 1: shift to the right                                     */
/*		      0: shift to the left                                      */
/************************************************************************/
#define _LCD_CURSORDISPLAYSHIFT 0x10
#define _LCD_DISPLAY_SHIFT      0x08
#define _LCD_CURSOR_SHIFT       0x00
#define _LCD_MOVERIGHT          0x04
#define _LCD_MOVELEFT           0x00

/************************************************************************/
/*      ENTRAMOS EN FUNTIONS SET:  D7 D6 D5 D4  D3 D2 D1 D0             */
/*      						   0  0  1  D/L  N  F  *  *             */
/*----------------------------------------------------------------------*/
/*      D/L = 1: modo 8 bits                                            */
/*		      0: modo 4 btis                                            */
/*		N   = 1: MODO 2 Lineas                                          */
/*		      0: MODO 1 Linea                                           */
/*		F   = 1: MATRIZ 5x10                                            */
/*		      0: MATRIZ 5x7/5x8                                         */
/************************************************************************/
#define _LCD_FUNTIONSET 0x20
#define _LCD_8BITMODE   0x10
#define _LCD_4BITMODE   0x00
#define _LCD_2LINE      0x08
#define _LCD_1LINE      0x00
#define _LCD_5x10DOTS   0x04
#define _LCD_5x8DOTS    0x00


/************************************************************************/
/*      SET CGRAM:  D7 D6  D5  D4   D3   D2   D1   D0                   */
/*      			0  1   ACG ACG  ACG  ACG  ACG  ACG                  */
/*----------------------------------------------------------------------*/
/*      ACG -> CGRAM ADDRESS                                            */
/************************************************************************/
#define _LCD_SET_CGRAM_ADDR  0x40

/************************************************************************/
/*      SET DDRAM:  D7 D6  D5  D4   D3   D2   D1   D0                   */
/*      			1  0   ADD ADD  ADD  ADD  ADD  ADD                  */
/*----------------------------------------------------------------------*/
/*      ADD -> DDRAM ADDRESS                                            */
/************************************************************************/
#define _LCD_SET_DDRAM_ADDR  0x80

/************************************************************************/
/* METODOS DE LIBRERIA                                                  */
/************************************************************************/

void lcd_init(void);
void lcd_write(uint8_t letra);
void lcd_command(uint8_t cmd);

void lcd_puts(char *str);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_printf(char *str, ...);

void lcd_clear(void);
void lcd_return_home(void);
void lcd_on(void);
void lcd_off(void);

void lcd_enable_blink(void);
void lcd_disable_blink(void);

void lcd_enable_cursor(void);
void lcd_disable_cursor(void);

void lcd_scroll_left(void);
void lcd_scroll_right(void);

void lcd_custom_char(uint8_t mem, uint8_t *charmap);

#endif
