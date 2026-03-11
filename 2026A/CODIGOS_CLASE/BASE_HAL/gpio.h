/*
 * gpio.h
 *
 * Created: 10/03/2026
 * Author: USER
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

/* Niveles lógicos */
#define HIGH    1U
#define LOW     0U

/* Dirección del pin */
#define INPUT   0U
#define OUTPUT  1U

/* =========================================
   MACROS GENERALES
   ========================================= */

/* Configurar un pin como entrada o salida */
#define GPIO_PIN_MODE(ddr, pin, mode)         \
    do {                                      \
        if ((mode) == INPUT)                  \
            (ddr) &= ~(1U << (pin));          \
        else                                  \
            (ddr) |=  (1U << (pin));          \
    } while (0)

/* Escribir un valor lógico en un pin */
#define GPIO_WRITE(port, pin, value)          \
    do {                                      \
        if ((value) == LOW)                   \
            (port) &= ~(1U << (pin));         \
        else                                  \
            (port) |=  (1U << (pin));         \
    } while (0)

/* Alternar el estado de un pin */
#define GPIO_TOGGLE(port, pin)                \
    do {                                      \
        (port) ^= (1U << (pin));              \
    } while (0)

/* Leer un pin */
#define GPIO_READ(pinreg, pin) \
    ((((pinreg) & (1U << (pin))) != 0U) ? HIGH : LOW)

/* Activar o desactivar pull-up interno
   Nota: usar cuando el pin esté configurado como INPUT */
#define GPIO_PULLUP(port, pin, enable)        \
    do {                                      \
        if ((enable) == HIGH)                 \
            (port) |=  (1U << (pin));         \
        else                                  \
            (port) &= ~(1U << (pin));         \
    } while (0)

/* =========================================
   MACROS ESPECÍFICAS POR PUERTO
   ========================================= */

/* Configuración por bit */
#define GPIO_PIN_MODE_PORTA(pin, mode) GPIO_PIN_MODE(DDRA, pin, mode)
#define GPIO_PIN_MODE_PORTB(pin, mode) GPIO_PIN_MODE(DDRB, pin, mode)
#define GPIO_PIN_MODE_PORTC(pin, mode) GPIO_PIN_MODE(DDRC, pin, mode)
#define GPIO_PIN_MODE_PORTD(pin, mode) GPIO_PIN_MODE(DDRD, pin, mode)

/* Escritura por bit */
#define GPIO_WRITE_PORTA(pin, value) GPIO_WRITE(PORTA, pin, value)
#define GPIO_WRITE_PORTB(pin, value) GPIO_WRITE(PORTB, pin, value)
#define GPIO_WRITE_PORTC(pin, value) GPIO_WRITE(PORTC, pin, value)
#define GPIO_WRITE_PORTD(pin, value) GPIO_WRITE(PORTD, pin, value)

/* Toggle por bit */
#define GPIO_TOGGLE_PORTA(pin) GPIO_TOGGLE(PORTA, pin)
#define GPIO_TOGGLE_PORTB(pin) GPIO_TOGGLE(PORTB, pin)
#define GPIO_TOGGLE_PORTC(pin) GPIO_TOGGLE(PORTC, pin)
#define GPIO_TOGGLE_PORTD(pin) GPIO_TOGGLE(PORTD, pin)

/* Lectura por bit */
#define GPIO_READ_PORTA(pin) GPIO_READ(PINA, pin)
#define GPIO_READ_PORTB(pin) GPIO_READ(PINB, pin)
#define GPIO_READ_PORTC(pin) GPIO_READ(PINC, pin)
#define GPIO_READ_PORTD(pin) GPIO_READ(PIND, pin)

/* Pull-up por bit */
#define GPIO_PULLUP_PORTA(pin, enable) GPIO_PULLUP(PORTA, pin, enable)
#define GPIO_PULLUP_PORTB(pin, enable) GPIO_PULLUP(PORTB, pin, enable)
#define GPIO_PULLUP_PORTC(pin, enable) GPIO_PULLUP(PORTC, pin, enable)
#define GPIO_PULLUP_PORTD(pin, enable) GPIO_PULLUP(PORTD, pin, enable)

/* =========================================
   ACCESO A NIVEL DE PUERTO COMPLETO
   ========================================= */

/* Escritura completa de puerto */
#define GPIO_PORT_WRITE(port, value)      \
    do {                                  \
        (port) = (value);                 \
    } while (0)

/* Lectura completa de puerto */
#define GPIO_PORT_READ(pinreg)            ((pinreg))

/* Toggle completo del puerto */
#define GPIO_PORT_TOGGLE(port)            \
    do {                                  \
        (port) ^= 0xFFU;                  \
    } while (0)

/* Configuración completa del DDR */
#define GPIO_PORT_MODE(ddr, value)        \
    do {                                  \
        (ddr) = (value);                  \
    } while (0)

/* Acceso específico por puerto completo */
#define GPIO_PORT_WRITE_A(value) GPIO_PORT_WRITE(PORTA, value)
#define GPIO_PORT_WRITE_B(value) GPIO_PORT_WRITE(PORTB, value)
#define GPIO_PORT_WRITE_C(value) GPIO_PORT_WRITE(PORTC, value)
#define GPIO_PORT_WRITE_D(value) GPIO_PORT_WRITE(PORTD, value)

#define GPIO_PORT_READ_A()      GPIO_PORT_READ(PINA)
#define GPIO_PORT_READ_B()      GPIO_PORT_READ(PINB)
#define GPIO_PORT_READ_C()      GPIO_PORT_READ(PINC)
#define GPIO_PORT_READ_D()      GPIO_PORT_READ(PIND)

#define GPIO_PORT_MODE_A(value) GPIO_PORT_MODE(DDRA, value)
#define GPIO_PORT_MODE_B(value) GPIO_PORT_MODE(DDRB, value)
#define GPIO_PORT_MODE_C(value) GPIO_PORT_MODE(DDRC, value)
#define GPIO_PORT_MODE_D(value) GPIO_PORT_MODE(DDRD, value)

#endif /* GPIO_H_ */