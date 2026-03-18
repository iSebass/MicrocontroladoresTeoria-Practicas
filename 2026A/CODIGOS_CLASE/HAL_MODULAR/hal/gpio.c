/*
 * gpio.c
 *
 * HAL - Implementacion de funciones de alto nivel de GPIO
 * Targets soportados: ATmega328P, ATmega1284P
 *
 * ----------------------------------------------------------------
 * POR QUE EXISTEN ESTAS FUNCIONES SI YA HAY MACROS?
 * ----------------------------------------------------------------
 * Las macros de gpio.h son la forma mas rapida de operar un pin:
 * se resuelven en tiempo de compilacion y no generan overhead.
 * Son ideales dentro de ISR o loops criticos de tiempo.
 *
 * Pero tienen una limitacion: no pueden validar sus argumentos.
 * Si un estudiante escribe GPIO_PIN_MODE_PORTB(9, OUTPUT), el
 * compilador no se queja — pero el pin 9 no existe en el puerto B
 * y el comportamiento es indefinido.
 *
 * Las funciones de este archivo resuelven eso:
 *   - Validan puerto y pin antes de operar
 *   - Retornan -1 si algo es invalido (facil de verificar)
 *   - Son mas legibles en logica de aplicacion de alto nivel
 *   - Permiten pasar el puerto como caracter ('B', 'C'...)
 *     lo que es util en drivers genericos y codigo de prueba
 *
 * Regla practica:
 *   Usa macros dentro de ISR y loops de tiempo critico.
 *   Usa funciones HAL_GPIO_* en inicializacion y logica de app.
 * ----------------------------------------------------------------
 *
 * Created: 10/03/2026
 * Author: USER
 */

#include "gpio.h"

/* =========================================
   HELPER INTERNO: resolver puerto por letra
   =========================================
   Esta funcion privada (static = solo visible en este .c)
   traduce la letra del puerto a los tres registros AVR
   correspondientes: DDRx, PORTx, PINx.

   Retorna 0 si el puerto es valido, -1 si no existe.
   Los punteros ddr, port y pin_reg se llenan con la
   direccion del registro correspondiente.
   ========================================= */
static int8_t get_port_regs(char          port_char,
                             volatile uint8_t **ddr,
                             volatile uint8_t **port_reg,
                             volatile uint8_t **pin_reg)
{
    switch (port_char) {

#if defined(HAL_TARGET_1284P)
        /*
         * Puerto A — disponible solo en ATmega1284P.
         * En el 328P este case no se compila, por lo que
         * intentar usar 'A' retornara -1 correctamente.
         */
        case 'A':
        case 'a':
            *ddr      = &DDRA;
            *port_reg = &PORTA;
            *pin_reg  = &PINA;
            return 0;
#endif

        case 'B':
        case 'b':
            *ddr      = &DDRB;
            *port_reg = &PORTB;
            *pin_reg  = &PINB;
            return 0;

        case 'C':
        case 'c':
            *ddr      = &DDRC;
            *port_reg = &PORTC;
            *pin_reg  = &PINC;
            return 0;

        case 'D':
        case 'd':
            *ddr      = &DDRD;
            *port_reg = &PORTD;
            *pin_reg  = &PIND;
            return 0;

        default:
            return -1;  /* Puerto no existe en este target */
    }
}

/* =========================================
   HELPER INTERNO: validar numero de pin
   =========================================
   Los puertos AVR tienen pines del 0 al 7.
   Un pin fuera de ese rango produciria un shift mayor a 7,
   lo cual en uint8_t es comportamiento indefinido en C.
   ========================================= */
static int8_t is_valid_pin(uint8_t pin)
{
    return (pin <= 7U) ? 0 : -1;
}

/* =========================================
   IMPLEMENTACION DE FUNCIONES PUBLICAS
   ========================================= */

/*
 * HAL_GPIO_ConfigPin
 *
 * Resuelve el puerto, valida el pin, y escribe en DDRx.
 * Usa la macro GPIO_PIN_MODE internamente para consistencia
 * con el resto de la HAL.
 */
int8_t HAL_GPIO_ConfigPin(char port, uint8_t pin, uint8_t mode)
{
    volatile uint8_t *ddr, *port_reg, *pin_reg;

    if (get_port_regs(port, &ddr, &port_reg, &pin_reg) != 0) {
        return -1;
    }
    if (is_valid_pin(pin) != 0) {
        return -1;
    }

    GPIO_PIN_MODE(*ddr, pin, mode);
    return 0;
}

/*
 * HAL_GPIO_WritePin
 *
 * Escribe HIGH o LOW en un pin de salida.
 * Si el pin es INPUT, escribir HIGH activa el pull-up —
 * para eso usar HAL_GPIO_SetPullup que es mas explicito.
 */
int8_t HAL_GPIO_WritePin(char port, uint8_t pin, uint8_t value)
{
    volatile uint8_t *ddr, *port_reg, *pin_reg;

    if (get_port_regs(port, &ddr, &port_reg, &pin_reg) != 0) {
        return -1;
    }
    if (is_valid_pin(pin) != 0) {
        return -1;
    }

    GPIO_WRITE(*port_reg, pin, value);
    return 0;
}

/*
 * HAL_GPIO_ReadPin
 *
 * Lee el nivel logico del pin desde PINx.
 * Retorna HIGH, LOW, o -1 si los parametros son invalidos.
 *
 * Nota: el cast a int8_t es seguro porque HIGH=1 y LOW=0
 * caben perfectamente en int8_t.
 */
int8_t HAL_GPIO_ReadPin(char port, uint8_t pin)
{
    volatile uint8_t *ddr, *port_reg, *pin_reg;

    if (get_port_regs(port, &ddr, &port_reg, &pin_reg) != 0) {
        return -1;
    }
    if (is_valid_pin(pin) != 0) {
        return -1;
    }

    return (int8_t)GPIO_READ(*pin_reg, pin);
}

/*
 * HAL_GPIO_TogglePin
 *
 * Invierte el estado del pin usando XOR sobre PORTx.
 */
int8_t HAL_GPIO_TogglePin(char port, uint8_t pin)
{
    volatile uint8_t *ddr, *port_reg, *pin_reg;

    if (get_port_regs(port, &ddr, &port_reg, &pin_reg) != 0) {
        return -1;
    }
    if (is_valid_pin(pin) != 0) {
        return -1;
    }

    GPIO_TOGGLE(*port_reg, pin);
    return 0;
}

/*
 * HAL_GPIO_SetPullup
 *
 * Activa o desactiva el pull-up interno.
 * Usar solo en pines configurados como INPUT.
 */
int8_t HAL_GPIO_SetPullup(char port, uint8_t pin, uint8_t enable)
{
    volatile uint8_t *ddr, *port_reg, *pin_reg;

    if (get_port_regs(port, &ddr, &port_reg, &pin_reg) != 0) {
        return -1;
    }
    if (is_valid_pin(pin) != 0) {
        return -1;
    }

    GPIO_PULLUP(*port_reg, pin, enable);
    return 0;
}

/* =========================================
   EJEMPLOS DE USO
   =========================================
   Comentados para referencia de los estudiantes.
   ========================================= */

/*
 * -----------------------------------------------------------
 * EJEMPLO 1: LED y boton — usando macros (tiempo critico)
 * -----------------------------------------------------------
 * Las macros son la mejor opcion dentro de loops o ISR
 * porque no generan overhead de llamada a funcion.
 *
 * #include "gpio.h"
 *
 * int main(void) {
 *     // Configurar LED en PB5 como salida
 *     GPIO_PIN_MODE_PORTB(5, OUTPUT);
 *
 *     // Configurar boton en PD2 como entrada con pull-up
 *     GPIO_PIN_MODE_PORTD(2, INPUT);
 *     GPIO_PULLUP_PORTD(2, 1U);     // 1U = activar pull-up
 *
 *     while (1) {
 *         // Si el boton esta presionado (LOW), encender LED
 *         if (GPIO_READ_PORTD(2) == LOW) {
 *             GPIO_WRITE_PORTB(5, HIGH);
 *         } else {
 *             GPIO_WRITE_PORTB(5, LOW);
 *         }
 *     }
 * }
 * -----------------------------------------------------------
 *
 * EJEMPLO 2: inicializacion con funciones HAL (alto nivel)
 * -----------------------------------------------------------
 * Las funciones son ideales en la inicializacion del sistema
 * porque validan los parametros y el codigo queda muy legible.
 *
 * #include "gpio.h"
 *
 * int main(void) {
 *     int8_t res;
 *
 *     res = HAL_GPIO_ConfigPin('B', 5, OUTPUT);
 *     if (res != 0) { // manejar error }
 *
 *     res = HAL_GPIO_ConfigPin('D', 2, INPUT);
 *     res = HAL_GPIO_SetPullup('D', 2, 1U);
 *
 *     HAL_GPIO_WritePin('B', 5, HIGH);   // encender LED
 *
 *     int8_t nivel = HAL_GPIO_ReadPin('D', 2);
 *     if (nivel == LOW) { ... }
 * }
 * -----------------------------------------------------------
 *
 * EJEMPLO 3: puerto completo — bus de datos de 8 bits
 * -----------------------------------------------------------
 * Util para controlar LCDs, matrices de LEDs, o cualquier
 * dispositivo que use un bus paralelo de 8 bits.
 *
 * #include "gpio.h"
 *
 * int main(void) {
 *     // Configurar todo PORTD como salida
 *     GPIO_PORT_MODE_D(0xFF);
 *
 *     // Enviar el byte 0xA5 por PORTD
 *     GPIO_PORT_WRITE_D(0xA5);    // 1010 0101
 *
 *     // Leer todo PORTB de una vez
 *     uint8_t bus = GPIO_PORT_READ_B();
 * }
 * -----------------------------------------------------------
 */
