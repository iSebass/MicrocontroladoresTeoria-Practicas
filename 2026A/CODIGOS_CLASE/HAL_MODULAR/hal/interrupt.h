/*
 * Autor: iSebas
 *
 * interrupt.h
 *
 * HAL - Capa de abstraccion de interrupciones
 * Targets soportados: ATmega328P, ATmega1284P
 *
 * ----------------------------------------------------------------
 * QUE CONTIENE ESTE ARCHIVO?
 * ----------------------------------------------------------------
 * Este header expone al usuario:
 *   - Tipos          (hal_irq_callback_t, hal_irq_sense_t, ...)
 *   - Macros         (HAL_IRQ_ENABLE, HAL_CRITICAL_ENTER, ...)
 *   - Declaraciones  (prototipos de HAL_EXT_INT_RegisterCallback, ...)
 *
 * La implementacion (ISR, arreglos internos, cuerpo de funciones)
 * vive en interrupt.c. Esto permite incluir este header desde
 * cualquier modulo sin causar errores de "multiple definition".
 *
 * Uso tipico:
 *   #include "interrupt.h"   <- en cualquier .c que necesite IRQ
 * ----------------------------------------------------------------
 *
 * Created: 13/03/2026
 * Author: USER
 */

#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include <avr/io.h>          /* Registros del microcontrolador (EIMSK, EICRA...) */
#include <avr/interrupt.h>   /* sei(), cli(), ISR()                               */
#include <stdint.h>          /* uint8_t, int8_t                                   */
#include <stddef.h>          /* NULL                                              */

/* =========================================
   1. DETECCION AUTOMATICA DEL TARGET
   =========================================
   avr-gcc define el simbolo del micro automaticamente segun
   el flag -mmcu usado al compilar:
       -mmcu=atmega328p  ->  define __AVR_ATmega328P__
       -mmcu=atmega1284p ->  define __AVR_ATmega1284P__

   Usamos esos simbolos para activar o desactivar codigo segun
   el hardware. Si compilas para un micro no soportado, el
   compilador lo indica con un error claro.
   ========================================= */

#if defined(__AVR_ATmega328P__)
    #define HAL_TARGET_328P
#elif defined(__AVR_ATmega1284P__)
    #define HAL_TARGET_1284P
#else
    #error "interrupt.h: Target no soportado. Usa -mmcu=atmega328p o -mmcu=atmega1284p"
#endif

/* =========================================
   2. TIPOS
   ========================================= */

/*
 * hal_irq_callback_t
 *
 * Tipo para un puntero a funcion de callback.
 * Representa cualquier funcion que no recibe ni retorna nada:
 *
 *   void mi_funcion(void) { ... }   <- compatible con este tipo
 *
 * Se usa para registrar el codigo del usuario que se ejecutara
 * automaticamente cuando ocurra una interrupcion.
 */
typedef void (*hal_irq_callback_t)(void);

/*
 * hal_irq_sense_t — Modo de disparo de una interrupcion externa
 *
 * Define QUE evento electrico en el pin dispara la IRQ.
 * Se configura en los bits ISCxy del registro EICRA.
 *
 *   LOW    -> IRQ mientras el pin este en 0V  (nivel bajo continuo)
 *   CHANGE -> IRQ en cualquier cambio         (0->1 o 1->0)
 *   FALL   -> IRQ solo al bajar de 5V a 0V   (flanco de bajada)
 *   RISE   -> IRQ solo al subir de 0V a 5V   (flanco de subida)
 */
typedef enum {
    HAL_IRQ_SENSE_LOW    = 0x00U,
    HAL_IRQ_SENSE_CHANGE = 0x01U,
    HAL_IRQ_SENSE_FALL   = 0x02U,
    HAL_IRQ_SENSE_RISE   = 0x03U
} hal_irq_sense_t;

/*
 * hal_ext_int_t — Fuentes de interrupcion externa
 *
 * ATmega328P : INT0, INT1
 * ATmega1284P: INT0, INT1, INT2
 *
 * HAL_EXT_INT_COUNT no es una fuente real. Es un valor centinela
 * que indica cuantos elementos tiene el enum. Se usa en interrupt.c
 * para dimensionar el arreglo interno de callbacks.
 */
typedef enum {
    HAL_EXT_INT0  = 0U,
    HAL_EXT_INT1  = 1U,
#if defined(HAL_TARGET_1284P)
    HAL_EXT_INT2  = 2U,
#endif
    HAL_EXT_INT_COUNT   /* NO usar como fuente — solo para contar */
} hal_ext_int_t;

/*
 * hal_pcint_bank_t — Bancos de Pin Change Interrupts
 *
 * Las PCINT se agrupan en bancos de 8 pines. Todos los pines
 * del mismo banco comparten un unico vector de interrupcion.
 *
 *   ATmega328P :  Banco 0 (PB), Banco 1 (PC), Banco 2 (PD)
 *   ATmega1284P:  Banco 0 (PB), Banco 1 (PC), Banco 2 (PD), Banco 3 (PA)
 */
typedef enum {
    HAL_PCINT_BANK0 = 0U,
    HAL_PCINT_BANK1 = 1U,
    HAL_PCINT_BANK2 = 2U,
#if defined(HAL_TARGET_1284P)
    HAL_PCINT_BANK3 = 3U,
#endif
    HAL_PCINT_BANK_COUNT    /* NO usar como banco — solo para contar */
} hal_pcint_bank_t;

/* =========================================
   3. MACROS: CONTROL GLOBAL DE INTERRUPCIONES
   =========================================
   sei() activa el bit I de SREG  -> habilita todas las IRQ.
   cli() limpia el bit I de SREG  -> deshabilita todas las IRQ.
   ========================================= */

/** Habilita todas las interrupciones globalmente */
#define HAL_IRQ_ENABLE()     sei()

/** Deshabilita todas las interrupciones globalmente */
#define HAL_IRQ_DISABLE()    cli()

/*
 * HAL_CRITICAL_ENTER / HAL_CRITICAL_EXIT
 * ---------------------------------------
 * Protegen un bloque de codigo que no puede ser interrumpido,
 * porque accede a datos compartidos entre el main y una ISR.
 *
 * Por que no basta con cli()/sei()?
 *   Si las IRQ ya estaban desactivadas antes de entrar, el sei()
 *   al salir las activaria incorrectamente. La solucion es guardar
 *   el estado de SREG y restaurarlo al salir.
 *
 * Uso:
 *   uint8_t sreg;
 *   HAL_CRITICAL_ENTER(sreg);
 *       dato_compartido++;        <- operacion protegida
 *   HAL_CRITICAL_EXIT(sreg);
 *
 * Por que do { } while (0)?
 *   Hace que la macro se comporte como una sola sentencia de C,
 *   evitando bugs si se usa dentro de un if sin llaves.
 */
#define HAL_CRITICAL_ENTER(sreg_var)    \
    do {                                \
        (sreg_var) = SREG;              \
        cli();                          \
    } while (0)

#define HAL_CRITICAL_EXIT(sreg_var)     \
    do {                                \
        SREG = (sreg_var);              \
    } while (0)

/* =========================================
   4. MACROS: INTERRUPCIONES EXTERNAS (INTx)
   =========================================
   Registros involucrados:
     EICRA  -> modo de disparo (sense) — 2 bits por INT
     EIMSK  -> habilitar / deshabilitar cada INT
     EIFR   -> flags de IRQ pendientes
   ========================================= */

/**
 * Habilita la interrupcion externa int_num en EIMSK.
 * Ejemplo: HAL_EXT_INT_ENABLE(HAL_EXT_INT0);
 */
#define HAL_EXT_INT_ENABLE(int_num)                             \
    do {                                                        \
        EIMSK |= (uint8_t)(1U << (uint8_t)(int_num));           \
    } while (0)

/**
 * Deshabilita la interrupcion externa int_num en EIMSK.
 * No borra el callback registrado.
 */
#define HAL_EXT_INT_DISABLE(int_num)                            \
    do {                                                        \
        EIMSK &= (uint8_t)(~(1U << (uint8_t)(int_num)));        \
    } while (0)

/**
 * Limpia el flag de IRQ pendiente en EIFR.
 *
 * ATENCION: en AVR los flags se limpian escribiendo un 1 (no un 0).
 * Util para descartar eventos ocurridos antes de habilitar la IRQ.
 */
#define HAL_EXT_INT_CLEAR_FLAG(int_num)                         \
    do {                                                        \
        EIFR |= (uint8_t)(1U << (uint8_t)(int_num));            \
    } while (0)

/* =========================================
   5. MACROS: PIN CHANGE INTERRUPTS (PCINT)
   =========================================
   Control en dos niveles:

   NIVEL 1 — Banco (PCICR):
     Habilita o deshabilita el banco completo. Si el banco esta
     deshabilitado, ningun pin del banco genera IRQ.

   NIVEL 2 — Pin individual (PCMSKx):
     Dentro de un banco activo, selecciona que pines disparan la IRQ.
     PCMSK0 -> Banco 0 | PCMSK1 -> Banco 1 | PCMSK2 -> Banco 2

   Secuencia tipica de configuracion:
     HAL_PCINT_PIN_ENABLE(PCMSK0, (1 << PCINT3));  // habilitar pin
     HAL_PCINT_BANK_ENABLE(HAL_PCINT_BANK0);        // habilitar banco
   ========================================= */

/** Habilita el banco PCINT completo en PCICR */
#define HAL_PCINT_BANK_ENABLE(bank)                             \
    do {                                                        \
        PCICR |= (uint8_t)(1U << (uint8_t)(bank));              \
    } while (0)

/** Deshabilita el banco PCINT completo en PCICR */
#define HAL_PCINT_BANK_DISABLE(bank)                            \
    do {                                                        \
        PCICR &= (uint8_t)(~(1U << (uint8_t)(bank)));           \
    } while (0)

/**
 * Habilita pines individuales dentro de un banco PCINT (PCMSKx).
 *
 * @param pcmsk  Registro del banco: PCMSK0, PCMSK1, PCMSK2...
 * @param mask   Mascara de pines: usa (1 << PCINTn) del datasheet
 *
 * Ejemplo — habilitar PCINT3 y PCINT5 en banco 0:
 *   HAL_PCINT_PIN_ENABLE(PCMSK0, (1 << PCINT3) | (1 << PCINT5));
 */
#define HAL_PCINT_PIN_ENABLE(pcmsk, mask)                       \
    do {                                                        \
        (pcmsk) |= (uint8_t)(mask);                             \
    } while (0)

/** Deshabilita pines individuales dentro de un banco PCINT */
#define HAL_PCINT_PIN_DISABLE(pcmsk, mask)                      \
    do {                                                        \
        (pcmsk) &= (uint8_t)(~(uint8_t)(mask));                 \
    } while (0)

/**
 * Limpia el flag pendiente de un banco PCINT en PCIFR.
 * Mismo principio que EIFR: se limpia escribiendo un 1.
 */
#define HAL_PCINT_CLEAR_FLAG(bank)                              \
    do {                                                        \
        PCIFR |= (uint8_t)(1U << (uint8_t)(bank));              \
    } while (0)

/* =========================================
   6. DECLARACIONES DE FUNCIONES
   =========================================
   Los cuerpos de estas funciones estan en interrupt.c.
   Aqui solo se declaran los prototipos para que cualquier
   modulo que incluya este header pueda llamarlas.
   ========================================= */

/**
 * HAL_EXT_INT_RegisterCallback
 * -----------------------------
 * Asocia una funcion del usuario a una interrupcion externa (INTx).
 *
 * Esta funcion configura el sense mode pero NO habilita la IRQ.
 * Luego de llamarla debes hacer:
 *   HAL_EXT_INT_CLEAR_FLAG(source);
 *   HAL_EXT_INT_ENABLE(source);
 *   HAL_IRQ_ENABLE();
 *
 * @param source    HAL_EXT_INT0, HAL_EXT_INT1 [, HAL_EXT_INT2 en 1284P]
 * @param sense     HAL_IRQ_SENSE_FALL / RISE / CHANGE / LOW
 * @param callback  Funcion void(void) a ejecutar, o NULL para desregistrar
 * @return          0 si OK,  -1 si source es invalido para este target
 */
int8_t HAL_EXT_INT_RegisterCallback(hal_ext_int_t      source,
                                     hal_irq_sense_t    sense,
                                     hal_irq_callback_t callback);

/**
 * HAL_PCINT_RegisterCallback
 * ---------------------------
 * Asocia una funcion del usuario a un banco PCINT completo.
 *
 * Como todos los pines del banco comparten el mismo vector, tu
 * callback debe leer el puerto para saber cual pin cambio.
 *
 * Esta funcion NO habilita el banco. Luego de llamarla debes hacer:
 *   HAL_PCINT_PIN_ENABLE(PCMSKx, mascara);
 *   HAL_PCINT_BANK_ENABLE(bank);
 *   HAL_IRQ_ENABLE();
 *
 * @param bank      HAL_PCINT_BANK0..BANK2 [BANK3 en 1284P]
 * @param callback  Funcion void(void) a ejecutar, o NULL para desregistrar
 * @return          0 si OK,  -1 si bank es invalido para este target
 */
int8_t HAL_PCINT_RegisterCallback(hal_pcint_bank_t   bank,
                                   hal_irq_callback_t callback);

#endif /* INTERRUPT_H_ */
