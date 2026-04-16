/*
 * interrupt.c
 *
 * HAL - Implementacion de la capa de interrupciones
 * Targets soportados: ATmega328P, ATmega1284P
 *
 * ----------------------------------------------------------------
 * POR QUE EXISTE ESTE ARCHIVO?
 * ----------------------------------------------------------------
 * Las ISR (rutinas de interrupcion) y los arreglos de callbacks
 * deben compilarse UNA SOLA VEZ en el proyecto. Si estuvieran
 * en el .h, cada .c que lo incluyera generaria su propia copia
 * y el linker fallaria con "multiple definition of ISR(...)".
 *
 * Separar la implementacion en este .c resuelve ese problema:
 * el compilador lo procesa una vez y el linker lo enlaza donde
 * sea necesario.
 * ----------------------------------------------------------------
 *
 * Created: 13/03/2026
 * Author: USER
 */

#include "interrupt.h"

/* =========================================
   ARREGLOS INTERNOS DE CALLBACKS
   =========================================
   Cada posicion del arreglo corresponde a una fuente de IRQ.
   Se inicializan en NULL: sin callback registrado por defecto.

   "static" -> visibles solo dentro de este archivo (.c).
               El usuario no puede acceder a ellos directamente,
               solo a traves de las funciones de registro.

   "volatile" -> necesario porque estos arreglos se leen dentro
                 de las ISR y se escriben desde el codigo principal.
                 Sin volatile, el compilador podria optimizarlos
                 incorrectamente asumiendo que no cambian.
   ========================================= */
static volatile hal_irq_callback_t _hal_ext_cb[HAL_EXT_INT_COUNT]      = {NULL};
static volatile hal_irq_callback_t _hal_pcint_cb[HAL_PCINT_BANK_COUNT] = {NULL};

/* =========================================
   IMPLEMENTACION: REGISTRO DE CALLBACKS
   ========================================= */

/*
 * HAL_EXT_INT_RegisterCallback
 *
 * Pasos internos:
 *   1. Validar que la fuente exista en este target
 *   2. Calcular la posicion en EICRA y escribir el sense mode
 *   3. Guardar el puntero al callback en seccion critica
 *      (evita que una IRQ lea el arreglo a mitad de la escritura)
 */
int8_t HAL_EXT_INT_RegisterCallback(hal_ext_int_t      source,
                                     hal_irq_sense_t    sense,
                                     hal_irq_callback_t callback)
{
    if (source >= HAL_EXT_INT_COUNT) {
        return -1;  /* Fuente invalida para este target */
    }

    /*
     * Configurar sense mode en EICRA.
     *
     * Cada INT ocupa 2 bits en EICRA:
     *   INT0 -> bits [1:0]  (shift = 0)
     *   INT1 -> bits [3:2]  (shift = 2)
     *   INT2 -> bits [5:4]  (shift = 4, solo 1284P)
     *
     * Operacion:
     *   1. Limpiar los 2 bits de esta INT:  EICRA & ~(0x03 << shift)
     *   2. Escribir el nuevo sense:         | (sense << shift)
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
 *
 * Solo guarda el puntero al callback. La configuracion de
 * pines (PCMSKx) y banco (PCICR) queda a cargo del usuario
 * con las macros HAL_PCINT_PIN_ENABLE / HAL_PCINT_BANK_ENABLE,
 * ya que cada aplicacion habilita pines distintos.
 */
int8_t HAL_PCINT_RegisterCallback(hal_pcint_bank_t   bank,
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
   ISR: INTERRUPCIONES EXTERNAS (INTx)
   =========================================
   ISR(VECTORx_vect) define la rutina que el hardware ejecuta
   cuando ocurre la interrupcion. avr-gcc genera automaticamente
   el prologo (guardar registros) y el epilogo (restaurarlos).

   El patron es siempre el mismo:
     - Verificar que hay un callback registrado (no NULL)
     - Llamarlo
   Verificar NULL protege contra IRQ disparadas antes de que
   el usuario registre su funcion.
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

/* =========================================
   ISR: PIN CHANGE INTERRUPTS (PCINTx)
   =========================================
   Cada banco tiene su propio vector, pero dentro del banco
   no se sabe que pin cambio — eso lo determina el usuario
   leyendo el puerto en su callback.
   ========================================= */

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
   EJEMPLOS DE USO
   =========================================
   Ejemplos comentados como referencia para los estudiantes.
   No forman parte de la compilacion.
   ========================================= */

/*
 * -----------------------------------------------------------
 * EJEMPLO 1: Boton en INT0 con flanco de bajada
 * -----------------------------------------------------------
 * Circuito: boton entre PD2 (INT0) y GND, pull-up interno activo.
 * Al presionar: PD2 baja de 5V a 0V -> flanco de bajada.
 *
 * #include "interrupt.h"
 * #include "gpio.h"
 *
 * static void on_boton(void) {
 *     GPIO_TOGGLE_PORTB(5);           // togglear LED en PB5
 * }
 *
 * int main(void) {
 *     GPIO_PIN_MODE_PORTB(5, OUTPUT);
 *     GPIO_PIN_MODE_PORTD(2, INPUT);
 *     GPIO_PULLUP_PORTD(2, HIGH);
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
 * cambios en dos pines con un solo vector.
 *
 * #include "interrupt.h"
 *
 * static void on_encoder(void) {
 *     uint8_t estado = GPIO_PORT_READ(PINB) & 0x03U;  // leer PB0 y PB1
 *     // logica de decodificacion aqui
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
 * Problema tipico: la ISR incrementa un contador de 16 bits.
 * En AVR, leer 16 bits toma dos instrucciones. Si la ISR
 * interrumpe entre las dos lecturas, el valor leido es corrupto.
 * La seccion critica lo evita.
 *
 * static volatile uint16_t contador = 0U;
 *
 * static void on_pulso(void) {
 *     contador++;
 * }
 *
 * int main(void) {
 *     // ...registro y habilitacion...
 *     while (1) {
 *         uint8_t  sreg;
 *         uint16_t copia;
 *
 *         HAL_CRITICAL_ENTER(sreg);
 *         copia = contador;           // lectura atomica y segura
 *         HAL_CRITICAL_EXIT(sreg);
 *
 *         procesar(copia);
 *     }
 * }
 * -----------------------------------------------------------
 */
