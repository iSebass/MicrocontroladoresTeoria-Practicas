/*
 * interrupt.h
 *
 * HAL - Capa de abstraccion de interrupciones
 * Targets soportados: ATmega328P, ATmega1284P
 *
 * Enfoque hibrido:
 *   - Macros para control global y secciones criticas (cero overhead)
 *   - Callbacks registrables para INT externas y PCINT
 *
 * Estilo: C99 limpio
 *
 * Created: 13/03/2026
 * Author: USER
 */

#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stddef.h>

/* =========================================
   DETECCION DE TARGET
   ========================================= */

#if defined(__AVR_ATmega328P__)
    #define HAL_TARGET_328P
#elif defined(__AVR_ATmega1284P__)
    #define HAL_TARGET_1284P
#else
    #error "interrupt.h: Target AVR no soportado. Use ATmega328P o ATmega1284P."
#endif

/* =========================================
   TIPOS BASE
   ========================================= */

/** Puntero a funcion de callback para interrupciones */
typedef void (*hal_irq_callback_t)(void);

/** Modos de disparo para interrupciones externas */
typedef enum {
    HAL_IRQ_SENSE_LOW    = 0x00U,   /* Nivel bajo                  */
    HAL_IRQ_SENSE_CHANGE = 0x01U,   /* Cualquier cambio de flanco  */
    HAL_IRQ_SENSE_FALL   = 0x02U,   /* Flanco de bajada            */
    HAL_IRQ_SENSE_RISE   = 0x03U    /* Flanco de subida            */
} hal_irq_sense_t;

/** Fuentes de interrupcion externa disponibles segun target */
typedef enum {
    HAL_EXT_INT0 = 0U,
    HAL_EXT_INT1 = 1U,
#if defined(HAL_TARGET_1284P)
    HAL_EXT_INT2 = 2U,
#endif
    HAL_EXT_INT_COUNT   /* Sentinel: NO usar como fuente */
} hal_ext_int_t;

/** Bancos PCINT disponibles segun target */
typedef enum {
    HAL_PCINT_BANK0 = 0U,   /* PCINT0..7   */
    HAL_PCINT_BANK1 = 1U,   /* PCINT8..15  */
    HAL_PCINT_BANK2 = 2U,   /* PCINT16..23 */
#if defined(HAL_TARGET_1284P)
    HAL_PCINT_BANK3 = 3U,   /* PCINT24..31 — solo 1284P */
#endif
    HAL_PCINT_BANK_COUNT    /* Sentinel: NO usar como banco */
} hal_pcint_bank_t;

/* =========================================
   MACROS: CONTROL GLOBAL (cero overhead)
   ========================================= */

/** Habilita interrupciones globales */
#define HAL_IRQ_ENABLE()     sei()

/** Deshabilita interrupciones globales */
#define HAL_IRQ_DISABLE()    cli()

/**
 * Inicia una seccion critica guardando SREG.
 * Usar siempre en par con HAL_CRITICAL_EXIT().
 *
 * Ejemplo:
 *   uint8_t sreg;
 *   HAL_CRITICAL_ENTER(sreg);
 *   // codigo critico
 *   HAL_CRITICAL_EXIT(sreg);
 */
#define HAL_CRITICAL_ENTER(sreg_var)    \
    do {                                \
        (sreg_var) = SREG;              \
        cli();                          \
    } while (0)

/** Restaura el estado de interrupciones guardado por HAL_CRITICAL_ENTER() */
#define HAL_CRITICAL_EXIT(sreg_var)     \
    do {                                \
        SREG = (sreg_var);              \
    } while (0)

/* =========================================
   MACROS: INTERRUPCIONES EXTERNAS
   ========================================= */

/**
 * Configura el modo de disparo de una INT externa.
 * Escribe directamente sobre EICRx — usar antes de habilitar.
 *
 * @param int_num  0 = INT0, 1 = INT1, 2 = INT2 (solo 1284P)
 * @param sense    hal_irq_sense_t
 */
#define HAL_EXT_INT_SET_SENSE(int_num, sense)                           \
    do {                                                                 \
        if ((int_num) <= 1U) {                                           \
            EICRA = (uint8_t)((EICRA & ~(0x03U << ((int_num) * 2U)))    \
                    | (((uint8_t)(sense)) << ((int_num) * 2U)));         \
        }                                                                \
        _Pragma("GCC diagnostic ignored \"-Wtype-limits\"")             \
        if ((int_num) == 2U) {                                           \
            EICRA = (uint8_t)((EICRA & ~(0x30U)) |                      \
                    ((((uint8_t)(sense)) & 0x03U) << 4U));               \
        }                                                                \
    } while (0)

/** Habilita una interrupcion externa en EIMSK */
#define HAL_EXT_INT_ENABLE(int_num)                                     \
    do {                                                                 \
        EIMSK |= (uint8_t)(1U << (int_num));                            \
    } while (0)

/** Deshabilita una interrupcion externa en EIMSK */
#define HAL_EXT_INT_DISABLE(int_num)                                    \
    do {                                                                 \
        EIMSK &= (uint8_t)(~(1U << (int_num)));                         \
    } while (0)

/** Limpia el flag de interrupcion externa pendiente en EIFR */
#define HAL_EXT_INT_CLEAR_FLAG(int_num)                                 \
    do {                                                                 \
        EIFR |= (uint8_t)(1U << (int_num));                             \
    } while (0)

/* =========================================
   MACROS: PIN CHANGE INTERRUPTS (PCINT)
   ========================================= */

/** Habilita el banco PCINT (PCICR) */
#define HAL_PCINT_BANK_ENABLE(bank)                                     \
    do {                                                                 \
        PCICR |= (uint8_t)(1U << (bank));                               \
    } while (0)

/** Deshabilita el banco PCINT (PCICR) */
#define HAL_PCINT_BANK_DISABLE(bank)                                    \
    do {                                                                 \
        PCICR &= (uint8_t)(~(1U << (bank)));                            \
    } while (0)

/**
 * Habilita una mascara de pines dentro de un banco PCINT.
 * PCMSKx controla que pines del banco disparan la IRQ.
 *
 * @param pcmsk  Registro PCMSKx (ej: PCMSK0, PCMSK1...)
 * @param mask   Mascara de bits de pines a habilitar
 */
#define HAL_PCINT_PIN_ENABLE(pcmsk, mask)                               \
    do {                                                                 \
        (pcmsk) |= (uint8_t)(mask);                                     \
    } while (0)

/** Deshabilita pines de un banco PCINT */
#define HAL_PCINT_PIN_DISABLE(pcmsk, mask)                              \
    do {                                                                 \
        (pcmsk) &= (uint8_t)(~(uint8_t)(mask));                         \
    } while (0)

/** Limpia el flag de un banco PCINT en PCIFR */
#define HAL_PCINT_CLEAR_FLAG(bank)                                      \
    do {                                                                 \
        PCIFR |= (uint8_t)(1U << (bank));                               \
    } while (0)

/* =========================================
   API DE CALLBACKS (runtime)
   ========================================= */

/**
 * Registra un callback para una interrupcion externa (INTx).
 *
 * No habilita la IRQ — llamar HAL_EXT_INT_ENABLE() por separado.
 *
 * @param source   HAL_EXT_INT0, HAL_EXT_INT1 [, HAL_EXT_INT2 en 1284P]
 * @param sense    Modo de disparo (HAL_IRQ_SENSE_*)
 * @param callback Puntero a funcion void(void), o NULL para limpiar
 * @return         0 si OK, -1 si parametro invalido
 */
int8_t HAL_EXT_INT_RegisterCallback(hal_ext_int_t   source,
                                     hal_irq_sense_t sense,
                                     hal_irq_callback_t callback);

/**
 * Registra un callback para un banco PCINT.
 *
 * No habilita el banco — llamar HAL_PCINT_BANK_ENABLE() por separado.
 *
 * @param bank     HAL_PCINT_BANK0 .. BANK2 [BANK3 en 1284P]
 * @param callback Puntero a funcion void(void), o NULL para limpiar
 * @return         0 si OK, -1 si parametro invalido
 */
int8_t HAL_PCINT_RegisterCallback(hal_pcint_bank_t   bank,
                                   hal_irq_callback_t callback);

/* =========================================
   IMPLEMENTACION INLINE
   (en header para mantener el modulo header-only)
   ========================================= */

/* Tablas internas de callbacks */
static volatile hal_irq_callback_t _hal_ext_callbacks[HAL_EXT_INT_COUNT];
static volatile hal_irq_callback_t _hal_pcint_callbacks[HAL_PCINT_BANK_COUNT];

static inline int8_t HAL_EXT_INT_RegisterCallback(hal_ext_int_t      source,
                                                    hal_irq_sense_t    sense,
                                                    hal_irq_callback_t callback)
{
    if (source >= HAL_EXT_INT_COUNT) {
        return -1;
    }

    HAL_EXT_INT_SET_SENSE((uint8_t)source, sense);

    uint8_t sreg;
    HAL_CRITICAL_ENTER(sreg);
    _hal_ext_callbacks[source] = callback;
    HAL_CRITICAL_EXIT(sreg);

    return 0;
}

static inline int8_t HAL_PCINT_RegisterCallback(hal_pcint_bank_t   bank,
                                                  hal_irq_callback_t callback)
{
    if (bank >= HAL_PCINT_BANK_COUNT) {
        return -1;
    }

    uint8_t sreg;
    HAL_CRITICAL_ENTER(sreg);
    _hal_pcint_callbacks[bank] = callback;
    HAL_CRITICAL_EXIT(sreg);

    return 0;
}

/* =========================================
   ISR: INTERRUPCIONES EXTERNAS
   ========================================= */

ISR(INT0_vect)
{
    if (_hal_ext_callbacks[HAL_EXT_INT0] != NULL) {
        _hal_ext_callbacks[HAL_EXT_INT0]();
    }
}

ISR(INT1_vect)
{
    if (_hal_ext_callbacks[HAL_EXT_INT1] != NULL) {
        _hal_ext_callbacks[HAL_EXT_INT1]();
    }
}

#if defined(HAL_TARGET_1284P)
ISR(INT2_vect)
{
    if (_hal_ext_callbacks[HAL_EXT_INT2] != NULL) {
        _hal_ext_callbacks[HAL_EXT_INT2]();
    }
}
#endif

/* =========================================
   ISR: PIN CHANGE INTERRUPTS
   ========================================= */

ISR(PCINT0_vect)
{
    if (_hal_pcint_callbacks[HAL_PCINT_BANK0] != NULL) {
        _hal_pcint_callbacks[HAL_PCINT_BANK0]();
    }
}

ISR(PCINT1_vect)
{
    if (_hal_pcint_callbacks[HAL_PCINT_BANK1] != NULL) {
        _hal_pcint_callbacks[HAL_PCINT_BANK1]();
    }
}

ISR(PCINT2_vect)
{
    if (_hal_pcint_callbacks[HAL_PCINT_BANK2] != NULL) {
        _hal_pcint_callbacks[HAL_PCINT_BANK2]();
    }
}

#if defined(HAL_TARGET_1284P)
ISR(PCINT3_vect)
{
    if (_hal_pcint_callbacks[HAL_PCINT_BANK3] != NULL) {
        _hal_pcint_callbacks[HAL_PCINT_BANK3]();
    }
}
#endif

#endif /* INTERRUPT_H_ */
