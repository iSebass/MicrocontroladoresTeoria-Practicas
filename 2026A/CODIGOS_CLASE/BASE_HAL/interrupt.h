/*
 * interrupt.h
 *
 * HAL - Capa de abstraccion de interrupciones
 * Targets soportados: ATmega328P, ATmega1284P
 *
 * ----------------------------------------------------------------
 * QUE ES UNA HAL?
 * ----------------------------------------------------------------
 * Una Hardware Abstraction Layer (HAL) es una capa de codigo que
 * "traduce" operaciones de hardware en funciones y macros con
 * nombres claros. En lugar de escribir:
 *
 *     EIMSK |= (1 << INT0);   // habilitar INT0 (crudo)
 *
 * escribimos:
 *
 *     HAL_EXT_INT_ENABLE(HAL_EXT_INT0);  // habilitar INT0 (HAL)
 *
 * El resultado en el microcontrolador es identico, pero el codigo
 * es mas facil de leer, mantener y portar a otro target.
 * ----------------------------------------------------------------
 *
 * ENFOQUE DE ESTE MODULO:
 *   - Macros    -> control global y configuracion de registros
 *                  (se resuelven en tiempo de compilacion, cero overhead)
 *   - Callbacks -> funciones que el usuario registra para que sean
 *                  llamadas automaticamente cuando ocurre una IRQ
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
   El compilador avr-gcc define automaticamente el simbolo del
   micro segun el flag -mmcu que uses al compilar. Por ejemplo:
       -mmcu=atmega328p  ->  define __AVR_ATmega328P__
       -mmcu=atmega1284p ->  define __AVR_ATmega1284P__

   Usamos esos simbolos para activar o desactivar secciones de
   codigo segun el hardware real. Si compilas para un micro no
   soportado, el compilador te avisa con un error claro.
   ========================================= */

#if defined(__AVR_ATmega328P__)
    #define HAL_TARGET_328P
#elif defined(__AVR_ATmega1284P__)
    #define HAL_TARGET_1284P
#else
    #error "interrupt.h: Target no soportado. Usa -mmcu=atmega328p o -mmcu=atmega1284p"
#endif

/* =========================================
   2. TIPOS DEFINIDOS POR LA HAL
   =========================================
   Definimos nuestros propios tipos para que el codigo que use
   esta HAL no dependa de los nombres de registros del datasheet.
   Esto hace el codigo mas legible y mas facil de portar.
   ========================================= */

/*
 * hal_irq_callback_t
 *
 * Un "callback" es una funcion que tu le pasas a otra funcion
 * para que la llame cuando ocurra algo. Aqui lo usamos para
 * asociar tu codigo a una interrupcion especifica.
 *
 * Este tipo representa un puntero a cualquier funcion que:
 *   - no recibe parametros  (void)
 *   - no retorna nada       (void)
 *
 * Ejemplo de funcion compatible:
 *   void mi_funcion(void) { ... }
 */
typedef void (*hal_irq_callback_t)(void);

/*
 * hal_irq_sense_t — Modo de disparo de una interrupcion externa
 *
 * Controla QUE evento electrico en el pin dispara la IRQ.
 * Esto se configura en los bits ISCxy del registro EICRA.
 *
 *   LOW    -> IRQ mientras el pin este en nivel bajo (0V)
 *             Util para: detectar que un dispositivo esta activo
 *   CHANGE -> IRQ en cualquier cambio de estado (0->1 o 1->0)
 *             Util para: encoders, lectura de bus
 *   FALL   -> IRQ solo en el flanco de bajada (5V -> 0V)
 *             Util para: botones con pull-up (lo mas comun)
 *   RISE   -> IRQ solo en el flanco de subida (0V -> 5V)
 *             Util para: sensores que activan con nivel alto
 */
typedef enum {
    HAL_IRQ_SENSE_LOW    = 0x00U,
    HAL_IRQ_SENSE_CHANGE = 0x01U,
    HAL_IRQ_SENSE_FALL   = 0x02U,
    HAL_IRQ_SENSE_RISE   = 0x03U
} hal_irq_sense_t;

/*
 * hal_ext_int_t — Fuentes de interrupcion externa disponibles
 *
 * El ATmega328P tiene INT0 e INT1.
 * El ATmega1284P agrega INT2.
 *
 * HAL_EXT_INT_COUNT no es una fuente real: es un truco de C para
 * saber cuantos elementos tiene el enum sin escribir el numero
 * a mano. Se usa para dimensionar arreglos internos.
 */
typedef enum {
    HAL_EXT_INT0 = 0U,
    HAL_EXT_INT1 = 1U,
#if defined(HAL_TARGET_1284P)
    HAL_EXT_INT2 = 2U,
#endif
    HAL_EXT_INT_COUNT   /* NO usar como fuente — solo para contar */
} hal_ext_int_t;

/*
 * hal_pcint_bank_t — Bancos de Pin Change Interrupts
 *
 * A diferencia de INT0/INT1 que tienen pines dedicados, las PCINT
 * se agrupan en "bancos" de 8 pines. Cualquier pin del banco puede
 * disparar la IRQ, pero todos comparten el mismo vector.
 *
 *   Banco 0 -> PCINT0..7   (Puerto B en 328P)
 *   Banco 1 -> PCINT8..15  (Puerto C en 328P)
 *   Banco 2 -> PCINT16..23 (Puerto D en 328P)
 *   Banco 3 -> PCINT24..31 (Puerto A en 1284P, no existe en 328P)
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
   sei() y cli() son instrucciones de avr-gcc que generan las
   instrucciones ensamblador SEI y CLI directamente.
   Las envolvemos en macros con nombre mas descriptivo.
   ========================================= */

/** Habilita todas las interrupciones (activa el bit I del registro SREG) */
#define HAL_IRQ_ENABLE()     sei()

/** Deshabilita todas las interrupciones (limpia el bit I del registro SREG) */
#define HAL_IRQ_DISABLE()    cli()

/*
 * HAL_CRITICAL_ENTER / HAL_CRITICAL_EXIT
 * ---------------------------------------
 * Una "seccion critica" es un bloque de codigo que NO puede ser
 * interrumpido a mitad de camino, porque trabaja con datos que
 * se comparten entre el codigo principal y una ISR.
 *
 * El problema de usar solo cli()/sei():
 *   Si las interrupciones ya estaban deshabilitadas antes de entrar,
 *   el sei() al salir las habilitaria cuando no deberia.
 *
 * La solucion: guardar el estado actual de SREG antes de deshabilitar,
 * y restaurarlo al salir. Asi se respeta el estado previo.
 *
 * Uso correcto:
 *   uint8_t sreg;                    <- variable local para guardar SREG
 *   HAL_CRITICAL_ENTER(sreg);        <- guarda SREG y hace cli()
 *       contador_compartido++;       <- operacion atomica protegida
 *   HAL_CRITICAL_EXIT(sreg);         <- restaura SREG como estaba
 *
 * Por que do { } while (0)?
 *   Es un idioma clasico en C para que la macro se comporte como
 *   una sola sentencia. Permite escribir:
 *       if (condicion)
 *           HAL_CRITICAL_ENTER(sreg);   <- funciona sin llaves
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
   Las interrupciones externas se controlan con tres registros:

     EICRA  -> configura el modo de disparo (sense) de INT0..INT2
     EIMSK  -> habilita o deshabilita cada INT individualmente
     EIFR   -> flags de interrupciones pendientes

   Cada INT ocupa 2 bits en EICRA para su modo de disparo:
     INT0 -> bits 1:0
     INT1 -> bits 3:2
     INT2 -> bits 5:4  (solo ATmega1284P)
   ========================================= */

/*
 * HAL_EXT_INT_ENABLE(int_num)
 * ----------------------------
 * Habilita la interrupcion externa numero int_num.
 *
 * Internamente escribe un 1 en el bit correspondiente de EIMSK.
 * Por ejemplo, para INT0 (int_num = 0):
 *     EIMSK |= (1 << 0);   ->  habilita INT0
 *
 * Uso:
 *     HAL_EXT_INT_ENABLE(HAL_EXT_INT0);
 */
#define HAL_EXT_INT_ENABLE(int_num)                             \
    do {                                                        \
        EIMSK |= (uint8_t)(1U << (uint8_t)(int_num));           \
    } while (0)

/*
 * HAL_EXT_INT_DISABLE(int_num)
 * -----------------------------
 * Deshabilita la interrupcion externa numero int_num.
 * No borra el callback registrado, solo silencia la IRQ.
 */
#define HAL_EXT_INT_DISABLE(int_num)                            \
    do {                                                        \
        EIMSK &= (uint8_t)(~(1U << (uint8_t)(int_num)));        \
    } while (0)

/*
 * HAL_EXT_INT_CLEAR_FLAG(int_num)
 * --------------------------------
 * Limpia el flag de interrupcion pendiente en EIFR.
 *
 * IMPORTANTE: en AVR, los flags de interrupcion se limpian
 * escribiendo un 1 en el bit (no un 0). Es contra-intuitivo
 * pero es como funciona el hardware.
 *
 * Util para descartar eventos que ocurrieron antes de que
 * habilitaras la interrupcion.
 */
#define HAL_EXT_INT_CLEAR_FLAG(int_num)                         \
    do {                                                        \
        EIFR |= (uint8_t)(1U << (uint8_t)(int_num));            \
    } while (0)

/* =========================================
   5. MACROS: PIN CHANGE INTERRUPTS (PCINT)
   =========================================
   Las PCINT tienen dos niveles de control:

     NIVEL 1 — Banco completo (PCICR):
       Habilita o deshabilita el banco entero.
       Un banco deshabilitado no genera ninguna IRQ,
       sin importar los pines habilitados en el nivel 2.

     NIVEL 2 — Pin individual (PCMSKx):
       Dentro de un banco habilitado, controla que pines
       especificos pueden disparar la IRQ.
       PCMSK0 -> banco 0, PCMSK1 -> banco 1, etc.

   Ejemplo tipico: habilitar PCINT en PB3 (banco 0, pin 3)
     HAL_PCINT_PIN_ENABLE(PCMSK0, (1 << PCINT3));
     HAL_PCINT_BANK_ENABLE(HAL_PCINT_BANK0);
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

/*
 * HAL_PCINT_PIN_ENABLE(pcmsk, mask)
 * ----------------------------------
 * Habilita uno o varios pines dentro de un banco PCINT.
 *
 * @param pcmsk  Registro de mascara del banco: PCMSK0, PCMSK1...
 * @param mask   Mascara de bits. Usa (1 << PCINTn) del datasheet.
 *
 * Ejemplo — habilitar PCINT3 y PCINT5 en banco 0:
 *   HAL_PCINT_PIN_ENABLE(PCMSK0, (1 << PCINT3) | (1 << PCINT5));
 */
#define HAL_PCINT_PIN_ENABLE(pcmsk, mask)                       \
    do {                                                        \
        (pcmsk) |= (uint8_t)(mask);                             \
    } while (0)

/** Deshabilita pines especificos dentro de un banco PCINT */
#define HAL_PCINT_PIN_DISABLE(pcmsk, mask)                      \
    do {                                                        \
        (pcmsk) &= (uint8_t)(~(uint8_t)(mask));                 \
    } while (0)

/*
 * HAL_PCINT_CLEAR_FLAG(bank)
 * ---------------------------
 * Limpia el flag pendiente del banco en PCIFR.
 * Mismo principio que EIFR: se limpia escribiendo un 1.
 */
#define HAL_PCINT_CLEAR_FLAG(bank)                              \
    do {                                                        \
        PCIFR |= (uint8_t)(1U << (uint8_t)(bank));              \
    } while (0)

/* =========================================
   6. TABLAS INTERNAS DE CALLBACKS
   =========================================
   Arreglos estaticos que guardan los punteros a las funciones
   del usuario. Son "volatile" porque se acceden tanto desde
   el codigo principal como desde las ISR, y el compilador
   no debe optimizarlas asumiendo que no cambian.

   "static" las hace visibles solo dentro de este archivo.
   ========================================= */
static volatile hal_irq_callback_t _hal_ext_cb[HAL_EXT_INT_COUNT];
static volatile hal_irq_callback_t _hal_pcint_cb[HAL_PCINT_BANK_COUNT];

/* =========================================
   7. FUNCIONES DE REGISTRO DE CALLBACKS
   =========================================
   Estas funciones son "static inline": el compilador las expande
   en el punto de llamada (como una macro) pero con la ventaja
   de tener tipos verificados y ser mas faciles de leer.
   ========================================= */

/*
 * HAL_EXT_INT_RegisterCallback
 * -----------------------------
 * Asocia una funcion del usuario a una interrupcion externa.
 *
 * Pasos internos:
 *   1. Valida que la fuente sea valida para este target
 *   2. Configura el modo de disparo en EICRA
 *   3. Guarda el puntero en la tabla de callbacks
 *      (dentro de seccion critica para evitar condiciones de carrera)
 *
 * NOTA: esta funcion NO habilita la interrupcion automaticamente.
 * Despues de llamarla debes hacer:
 *   HAL_EXT_INT_ENABLE(fuente);
 *   HAL_IRQ_ENABLE();
 *
 * @param source    Fuente: HAL_EXT_INT0, HAL_EXT_INT1 [, HAL_EXT_INT2]
 * @param sense     Modo de disparo: HAL_IRQ_SENSE_FALL, etc.
 * @param callback  Tu funcion void(void), o NULL para desregistrar
 * @return          0 si todo OK, -1 si la fuente es invalida
 */
static inline int8_t HAL_EXT_INT_RegisterCallback(hal_ext_int_t      source,
                                                   hal_irq_sense_t    sense,
                                                   hal_irq_callback_t callback)
{
    if (source >= HAL_EXT_INT_COUNT) {
        return -1;  /* Fuente invalida para este target */
    }

    /*
     * Configurar sense mode en EICRA.
     * Cada INT ocupa 2 bits: INT0->bits[1:0], INT1->bits[3:2]
     *
     * La operacion en dos pasos:
     *   1. Limpiar los 2 bits de esta INT: EICRA & ~(0x03 << pos)
     *   2. Escribir el nuevo sense en esos 2 bits: | (sense << pos)
     */
    uint8_t shift = (uint8_t)((uint8_t)source * 2U);
    EICRA = (uint8_t)((EICRA & ~(uint8_t)(0x03U << shift))
                     | ((uint8_t)sense << shift));

    /* Guardar callback en seccion critica */
    uint8_t sreg;
    HAL_CRITICAL_ENTER(sreg);
    _hal_ext_cb[source] = callback;
    HAL_CRITICAL_EXIT(sreg);

    return 0;
}

/*
 * HAL_PCINT_RegisterCallback
 * ---------------------------
 * Asocia una funcion del usuario a un banco PCINT completo.
 *
 * Recuerda que un banco entero comparte un solo vector de IRQ.
 * Tu callback debe leer el puerto para saber que pin cambio.
 *
 * NOTA: esta funcion NO habilita el banco automaticamente.
 * Despues de llamarla debes hacer:
 *   HAL_PCINT_PIN_ENABLE(PCMSKx, mascara_de_pines);
 *   HAL_PCINT_BANK_ENABLE(banco);
 *   HAL_IRQ_ENABLE();
 *
 * @param bank      Banco: HAL_PCINT_BANK0 .. BANK2 [BANK3 en 1284P]
 * @param callback  Tu funcion void(void), o NULL para desregistrar
 * @return          0 si todo OK, -1 si el banco es invalido
 */
static inline int8_t HAL_PCINT_RegisterCallback(hal_pcint_bank_t   bank,
                                                 hal_irq_callback_t callback)
{
    if (bank >= HAL_PCINT_BANK_COUNT) {
        return -1;  /* Banco invalido para este target */
    }

    uint8_t sreg;
    HAL_CRITICAL_ENTER(sreg);
    _hal_pcint_cb[bank] = callback;
    HAL_CRITICAL_EXIT(sreg);

    return 0;
}

/* =========================================
   8. ISR — VECTORES DE INTERRUPCION
   =========================================
   ISR(VECTORx_vect) es la forma de definir una rutina de
   interrupcion en avr-gcc. El compilador genera el prologo y
   epilogo automaticamente (guardar/restaurar registros).

   Cada ISR llama al callback registrado por el usuario, si
   existe. Verificar NULL evita un crash si la IRQ se dispara
   sin haber registrado un callback previamente.
   ========================================= */

ISR(INT0_vect)
{
    if (_hal_ext_cb[HAL_EXT_INT0] != NULL) {
        _hal_ext_cb[HAL_EXT_INT0]();
    }
}

ISR(INT1_vect)
{
    if (_hal_ext_cb[HAL_EXT_INT1] != NULL) {
        _hal_ext_cb[HAL_EXT_INT1]();
    }
}

#if defined(HAL_TARGET_1284P)
ISR(INT2_vect)
{
    if (_hal_ext_cb[HAL_EXT_INT2] != NULL) {
        _hal_ext_cb[HAL_EXT_INT2]();
    }
}
#endif

ISR(PCINT0_vect)
{
    if (_hal_pcint_cb[HAL_PCINT_BANK0] != NULL) {
        _hal_pcint_cb[HAL_PCINT_BANK0]();
    }
}

ISR(PCINT1_vect)
{
    if (_hal_pcint_cb[HAL_PCINT_BANK1] != NULL) {
        _hal_pcint_cb[HAL_PCINT_BANK1]();
    }
}

ISR(PCINT2_vect)
{
    if (_hal_pcint_cb[HAL_PCINT_BANK2] != NULL) {
        _hal_pcint_cb[HAL_PCINT_BANK2]();
    }
}

#if defined(HAL_TARGET_1284P)
ISR(PCINT3_vect)
{
    if (_hal_pcint_cb[HAL_PCINT_BANK3] != NULL) {
        _hal_pcint_cb[HAL_PCINT_BANK3]();
    }
}
#endif

/* =========================================
   9. EJEMPLOS DE USO
   =========================================
   Estos ejemplos estan comentados para que no compilen
   automaticamente. Sirvete de ellos como referencia.
   ========================================= */

/*
 * -----------------------------------------------------------
 * EJEMPLO 1: Boton en INT0 con flanco de bajada
 * -----------------------------------------------------------
 * Esquema: boton conectado entre PD2 (INT0) y GND,
 *          pull-up interno activado.
 * Cuando el boton se presiona: PD2 pasa de 5V a 0V (bajada).
 *
 * #include "interrupt.h"
 * #include "gpio.h"
 *
 * static void on_boton(void) {
 *     GPIO_TOGGLE_PORTB(5);   // togglear LED en PB5
 * }
 *
 * int main(void) {
 *     GPIO_PIN_MODE_PORTB(5, OUTPUT);
 *     GPIO_PIN_MODE_PORTD(2, INPUT);
 *     GPIO_PULLUP_PORTD(2, HIGH);             // activar pull-up en PD2
 *
 *     HAL_EXT_INT_RegisterCallback(HAL_EXT_INT0, HAL_IRQ_SENSE_FALL, on_boton);
 *     HAL_EXT_INT_CLEAR_FLAG(HAL_EXT_INT0);   // descartar flancos previos
 *     HAL_EXT_INT_ENABLE(HAL_EXT_INT0);
 *     HAL_IRQ_ENABLE();
 *
 *     while (1) { }
 * }
 * -----------------------------------------------------------
 *
 * EJEMPLO 2: Encoder rotativo en PCINT (banco 0, pines PB0 y PB1)
 * -----------------------------------------------------------
 * Las PCINT son ideales para encoders porque necesitas detectar
 * cambios en dos pines distintos con un solo vector.
 *
 * #include "interrupt.h"
 *
 * static void on_encoder(void) {
 *     uint8_t estado = GPIO_PORT_READ(PINB) & 0x03U;  // leer PB0 y PB1
 *     // logica de decodificacion del encoder aqui
 * }
 *
 * int main(void) {
 *     HAL_PCINT_RegisterCallback(HAL_PCINT_BANK0, on_encoder);
 *     HAL_PCINT_PIN_ENABLE(PCMSK0, (1 << PCINT0) | (1 << PCINT1));
 *     HAL_PCINT_BANK_ENABLE(HAL_PCINT_BANK0);
 *     HAL_IRQ_ENABLE();
 *
 *     while (1) { }
 * }
 * -----------------------------------------------------------
 *
 * EJEMPLO 3: Seccion critica para variable compartida
 * -----------------------------------------------------------
 * Problema: una ISR modifica una variable que el loop principal
 * tambien lee. Si la ISR interrumpe a mitad de una lectura de
 * 16 bits, puedes leer un valor corrupto.
 *
 * static volatile uint16_t contador = 0U;
 *
 * static void on_pulso(void) {
 *     contador++;   // solo la ISR escribe, es seguro
 * }
 *
 * int main(void) {
 *     // ...registro y habilitacion...
 *     while (1) {
 *         uint8_t sreg;
 *         uint16_t copia;
 *
 *         HAL_CRITICAL_ENTER(sreg);
 *         copia = contador;             // lectura atomica protegida
 *         HAL_CRITICAL_EXIT(sreg);
 *
 *         procesar(copia);
 *     }
 * }
 * -----------------------------------------------------------
 */

#endif /* INTERRUPT_H_ */