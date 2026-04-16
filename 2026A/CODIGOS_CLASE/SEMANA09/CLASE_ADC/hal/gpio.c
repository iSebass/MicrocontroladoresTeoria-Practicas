/*
 * gpio.h
 *
 * HAL - Capa de abstraccion de GPIO (General Purpose Input/Output)
 * Targets soportados: ATmega328P, ATmega1284P
 *
 * ----------------------------------------------------------------
 * QUE ES GPIO?
 * ----------------------------------------------------------------
 * GPIO son los pines digitales de proposito general del micro.
 * Cada pin puede configurarse como entrada o salida, y leerse
 * o escribirse individualmente o en grupos (puertos completos).
 *
 * En AVR, cada puerto tiene tres registros asociados:
 *
 *   DDRx  -> Data Direction Register
 *            Controla la DIRECCION del pin.
 *            0 = entrada (INPUT), 1 = salida (OUTPUT)
 *
 *   PORTx -> Port Output Register
 *            Si el pin es OUTPUT: escribe el nivel logico (0 o 1)
 *            Si el pin es INPUT:  activa (1) o desactiva (0) el pull-up
 *
 *   PINx  -> Port Input Register
 *            Registra el nivel logico actual en el pin (solo lectura)
 *            En AVR, escribir un 1 en PINx tambien hace toggle del pin
 *
 * Ejemplo para PB5 (pin 13 del Arduino Uno):
 *   DDRB  |= (1 << 5);   // configurar PB5 como salida
 *   PORTB |= (1 << 5);   // poner PB5 en HIGH (5V)
 *   PORTB &= ~(1 << 5);  // poner PB5 en LOW  (0V)
 *   uint8_t v = PINB & (1 << 5);  // leer nivel actual de PB5
 *
 * Esta HAL envuelve esas operaciones en macros con nombres claros.
 * ----------------------------------------------------------------
 *
 * Created: 10/03/2026
 * Author: USER
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <avr/io.h>   /* Registros del micro: DDRA, PORTA, PINA, etc. */
#include <stdint.h>   /* uint8_t                                       */

/* =========================================
   1. DETECCION AUTOMATICA DEL TARGET
   =========================================
   avr-gcc define el simbolo del micro segun el flag -mmcu:
       -mmcu=atmega328p  ->  define __AVR_ATmega328P__
       -mmcu=atmega1284p ->  define __AVR_ATmega1284P__

   Diferencia importante entre los dos targets:
     ATmega328P : puertos B, C, D         (sin puerto A)
     ATmega1284P: puertos A, B, C, D      (con puerto A completo)

   Las macros de Puerto A quedan disponibles solo para el 1284P.
   Intentar usarlas en el 328P causara un error de compilacion,
   lo cual es exactamente lo que queremos ? falla temprano y claro.
   ========================================= */

#if defined(__AVR_ATmega328P__)
    #define HAL_TARGET_328P
#elif defined(__AVR_ATmega1284P__)
    #define HAL_TARGET_1284P
#else
    #error "gpio.h: Target no soportado. Usa -mmcu=atmega328p o -mmcu=atmega1284p"
#endif

/* =========================================
   2. CONSTANTES DE NIVEL Y DIRECCION
   =========================================
   Definimos nombres claros para los valores que mas se repiten.
   Usar HIGH/LOW/INPUT/OUTPUT en lugar de 1U/0U hace el codigo
   mucho mas legible para quien no conoce el hardware.
   ========================================= */

/** Nivel logico alto  (5V en sistemas de 5V) */
#define HIGH    1U

/** Nivel logico bajo  (0V / GND) */
#define LOW     0U

/** Direccion de pin: entrada ? el pin lee el mundo exterior */
#define INPUT   0U

/** Direccion de pin: salida ? el pin controla algo en el exterior */
#define OUTPUT  1U

/* =========================================
   3. MACROS GENERALES DE UN PIN
   =========================================
   Estas macros operan sobre cualquier puerto y pin.
   Reciben el registro directamente (DDRx, PORTx, PINx).

   Son la base de todas las macros especificas por puerto
   que se definen mas adelante.

   Por que do { } while (0)?
     Hace que la macro se comporte como una sola sentencia,
     evitando bugs al usarla dentro de if/else sin llaves.
   ========================================= */

/*
 * GPIO_PIN_MODE(ddr, pin, mode)
 * ------------------------------
 * Configura la direccion de un pin individual.
 *
 * @param ddr   Registro DDRx del puerto  (ej: DDRB)
 * @param pin   Numero de pin             (0..7)
 * @param mode  INPUT (0) u OUTPUT (1)
 *
 * Internamente:
 *   INPUT  -> pone a 0 el bit 'pin' de DDRx: ddr &= ~(1 << pin)
 *   OUTPUT -> pone a 1 el bit 'pin' de DDRx: ddr |=  (1 << pin)
 */
#define GPIO_PIN_MODE(ddr, pin, mode)           \
    do {                                        \
        if ((mode) == INPUT)                    \
            (ddr) &= (uint8_t)(~(1U << (pin))); \
        else                                    \
            (ddr) |=  (uint8_t)(1U << (pin));   \
    } while (0)

/*
 * GPIO_WRITE(port, pin, value)
 * -----------------------------
 * Escribe un nivel logico en un pin configurado como OUTPUT.
 *
 * @param port   Registro PORTx   (ej: PORTB)
 * @param pin    Numero de pin    (0..7)
 * @param value  HIGH (1) o LOW (0)
 *
 * ATENCION: si el pin esta configurado como INPUT, escribir HIGH
 * activa el pull-up interno. Usa GPIO_PULLUP en ese caso para
 * dejar clara la intencion.
 */
#define GPIO_WRITE(port, pin, value)                \
    do {                                            \
        if ((value) == LOW)                         \
            (port) &= (uint8_t)(~(1U << (pin)));    \
        else                                        \
            (port) |=  (uint8_t)(1U << (pin));      \
    } while (0)

/*
 * GPIO_TOGGLE(port, pin)
 * -----------------------
 * Invierte el estado actual de un pin de salida.
 *
 * Usa XOR con 1: si el pin estaba en HIGH pasa a LOW, y viceversa.
 * Es una operacion atomica en AVR (una sola instruccion).
 *
 * Tip: en AVR tambien se puede hacer toggle escribiendo 1 en PINx,
 * pero este metodo es mas portable y explicito.
 */
#define GPIO_TOGGLE(port, pin)                      \
    do {                                            \
        (port) ^= (uint8_t)(1U << (pin));           \
    } while (0)

/*
 * GPIO_READ(pinreg, pin)
 * -----------------------
 * Lee el nivel logico actual de un pin.
 *
 * @param pinreg  Registro PINx  (ej: PINB)
 * @param pin     Numero de pin  (0..7)
 * @return        HIGH (1) si el pin esta en 5V, LOW (0) si esta en 0V
 *
 * La expresion ternaria garantiza que el resultado sea siempre
 * exactamente HIGH o LOW, no cualquier valor distinto de cero.
 */
#define GPIO_READ(pinreg, pin)                              \
    ((((pinreg) & (uint8_t)(1U << (pin))) != 0U) ? HIGH : LOW)

/*
 * GPIO_PULLUP(port, pin, enable)
 * -------------------------------
 * Activa o desactiva la resistencia pull-up interna de un pin INPUT.
 *
 * @param port    Registro PORTx  (ej: PORTB)
 * @param pin     Numero de pin   (0..7)
 * @param enable  1U para activar, 0U para desactivar
 *
 * Que es un pull-up?
 *   Es una resistencia interna (~50kOhm) que conecta el pin a VCC.
 *   Cuando el pin esta "flotando" (sin nada conectado), el pull-up
 *   lo mantiene en HIGH. Al conectar un boton a GND, presionarlo
 *   lleva el pin a LOW ? es el esquema de boton mas comun en AVR.
 *
 * IMPORTANTE: el pin debe estar configurado como INPUT antes
 * de usar esta macro, de lo contrario estarias escribiendo en
 * el registro de salida, no activando el pull-up.
 */
#define GPIO_PULLUP(port, pin, enable)              \
    do {                                            \
        if ((enable) != 0U)                         \
            (port) |=  (uint8_t)(1U << (pin));      \
        else                                        \
            (port) &= (uint8_t)(~(1U << (pin)));    \
    } while (0)

/* =========================================
   4. MACROS ESPECIFICAS POR PUERTO ? PIN
   =========================================
   Envuelven las macros generales con el registro correcto
   para cada puerto. Hacen el codigo de aplicacion mas conciso:
     GPIO_PIN_MODE_PORTB(5, OUTPUT)
   en lugar de:
     GPIO_PIN_MODE(DDRB, 5, OUTPUT)
   ========================================= */

/* --- Puerto A (solo ATmega1284P) --- */
#if defined(HAL_TARGET_1284P)
#define GPIO_PIN_MODE_PORTA(pin, mode)   GPIO_PIN_MODE(DDRA,  pin, mode)
#define GPIO_WRITE_PORTA(pin, value)     GPIO_WRITE(PORTA,    pin, value)
#define GPIO_TOGGLE_PORTA(pin)           GPIO_TOGGLE(PORTA,   pin)
#define GPIO_READ_PORTA(pin)             GPIO_READ(PINA,      pin)
#define GPIO_PULLUP_PORTA(pin, enable)   GPIO_PULLUP(PORTA,   pin, enable)
#endif

/* --- Puerto B (ambos targets) --- */
#define GPIO_PIN_MODE_PORTB(pin, mode)   GPIO_PIN_MODE(DDRB,  pin, mode)
#define GPIO_WRITE_PORTB(pin, value)     GPIO_WRITE(PORTB,    pin, value)
#define GPIO_TOGGLE_PORTB(pin)           GPIO_TOGGLE(PORTB,   pin)
#define GPIO_READ_PORTB(pin)             GPIO_READ(PINB,      pin)
#define GPIO_PULLUP_PORTB(pin, enable)   GPIO_PULLUP(PORTB,   pin, enable)

/* --- Puerto C (ambos targets) --- */
#define GPIO_PIN_MODE_PORTC(pin, mode)   GPIO_PIN_MODE(DDRC,  pin, mode)
#define GPIO_WRITE_PORTC(pin, value)     GPIO_WRITE(PORTC,    pin, value)
#define GPIO_TOGGLE_PORTC(pin)           GPIO_TOGGLE(PORTC,   pin)
#define GPIO_READ_PORTC(pin)             GPIO_READ(PINC,      pin)
#define GPIO_PULLUP_PORTC(pin, enable)   GPIO_PULLUP(PORTC,   pin, enable)

/* --- Puerto D (ambos targets) --- */
#define GPIO_PIN_MODE_PORTD(pin, mode)   GPIO_PIN_MODE(DDRD,  pin, mode)
#define GPIO_WRITE_PORTD(pin, value)     GPIO_WRITE(PORTD,    pin, value)
#define GPIO_TOGGLE_PORTD(pin)           GPIO_TOGGLE(PORTD,   pin)
#define GPIO_READ_PORTD(pin)             GPIO_READ(PIND,      pin)
#define GPIO_PULLUP_PORTD(pin, enable)   GPIO_PULLUP(PORTD,   pin, enable)

/* =========================================
   5. MACROS DE PUERTO COMPLETO
   =========================================
   Operan sobre los 8 bits del puerto a la vez.
   Utiles para inicializar buses, matrices de LEDs,
   o cualquier caso donde se controlen varios pines juntos.
   ========================================= */

/*
 * GPIO_PORT_WRITE(port, value)
 * -----------------------------
 * Escribe un byte completo en el puerto de salida.
 * Cada bit del valor corresponde a un pin del puerto.
 *
 * Ejemplo ? encender los 4 pines altos de PORTB:
 *   GPIO_PORT_WRITE(PORTB, 0xF0);   // 1111 0000
 */
#define GPIO_PORT_WRITE(port, value)        \
    do {                                    \
        (port) = (uint8_t)(value);          \
    } while (0)

/*
 * GPIO_PORT_READ(pinreg)
 * -----------------------
 * Lee el byte completo del registro de entrada del puerto.
 * Retorna el estado de los 8 pines como una mascara de bits.
 *
 * Ejemplo ? leer todo PINB y aislar los 4 pines bajos:
 *   uint8_t estado = GPIO_PORT_READ(PINB) & 0x0FU;
 */
#define GPIO_PORT_READ(pinreg)              ((uint8_t)(pinreg))

/*
 * GPIO_PORT_TOGGLE(port)
 * -----------------------
 * Invierte el estado de los 8 pines del puerto simultaneamente.
 */
#define GPIO_PORT_TOGGLE(port)              \
    do {                                    \
        (port) ^= 0xFFU;                    \
    } while (0)

/*
 * GPIO_PORT_MODE(ddr, value)
 * ---------------------------
 * Configura la direccion de los 8 pines del puerto de una vez.
 * Cada bit del value: 0 = INPUT, 1 = OUTPUT.
 *
 * Ejemplo ? configurar PORTB completo como salida:
 *   GPIO_PORT_MODE(DDRB, 0xFF);
 *
 * Ejemplo ? pines 0..3 como entrada, 4..7 como salida:
 *   GPIO_PORT_MODE(DDRB, 0xF0);
 */
#define GPIO_PORT_MODE(ddr, value)          \
    do {                                    \
        (ddr) = (uint8_t)(value);           \
    } while (0)

/* --- Acceso de puerto completo por nombre (ambos targets) --- */
#define GPIO_PORT_WRITE_B(value)    GPIO_PORT_WRITE(PORTB, value)
#define GPIO_PORT_WRITE_C(value)    GPIO_PORT_WRITE(PORTC, value)
#define GPIO_PORT_WRITE_D(value)    GPIO_PORT_WRITE(PORTD, value)

#define GPIO_PORT_READ_B()          GPIO_PORT_READ(PINB)
#define GPIO_PORT_READ_C()          GPIO_PORT_READ(PINC)
#define GPIO_PORT_READ_D()          GPIO_PORT_READ(PIND)

#define GPIO_PORT_MODE_B(value)     GPIO_PORT_MODE(DDRB, value)
#define GPIO_PORT_MODE_C(value)     GPIO_PORT_MODE(DDRC, value)
#define GPIO_PORT_MODE_D(value)     GPIO_PORT_MODE(DDRD, value)

/* --- Puerto A completo (solo ATmega1284P) --- */
#if defined(HAL_TARGET_1284P)
#define GPIO_PORT_WRITE_A(value)    GPIO_PORT_WRITE(PORTA, value)
#define GPIO_PORT_READ_A()          GPIO_PORT_READ(PINA)
#define GPIO_PORT_MODE_A(value)     GPIO_PORT_MODE(DDRA, value)
#endif

/* =========================================
   6. DECLARACIONES DE FUNCIONES (gpio.c)
   =========================================
   Funciones de alto nivel implementadas en gpio.c.
   Son mas costosas que las macros (llamada a funcion, validacion)
   pero mas seguras y legibles para logica de aplicacion.
   ========================================= */

/**
 * HAL_GPIO_ConfigPin
 * -------------------
 * Configura un pin como INPUT u OUTPUT de forma segura.
 * Valida que el puerto y pin sean validos antes de operar.
 *
 * @param port  Letra del puerto: 'B', 'C', 'D' [, 'A' en 1284P]
 * @param pin   Numero de pin: 0..7
 * @param mode  INPUT (0U) u OUTPUT (1U)
 * @return      0 si OK, -1 si puerto o pin invalido
 */
int8_t HAL_GPIO_ConfigPin(char port, uint8_t pin, uint8_t mode);

/**
 * HAL_GPIO_WritePin
 * ------------------
 * Escribe un nivel logico en un pin de salida.
 *
 * @param port   Letra del puerto: 'B', 'C', 'D' [, 'A' en 1284P]
 * @param pin    Numero de pin: 0..7
 * @param value  HIGH (1U) o LOW (0U)
 * @return       0 si OK, -1 si puerto o pin invalido
 */
int8_t HAL_GPIO_WritePin(char port, uint8_t pin, uint8_t value);

/**
 * HAL_GPIO_ReadPin
 * -----------------
 * Lee el nivel logico actual de un pin.
 *
 * @param port  Letra del puerto: 'B', 'C', 'D' [, 'A' en 1284P]
 * @param pin   Numero de pin: 0..7
 * @return      HIGH (1U), LOW (0U), o -1 si parametro invalido
 */
int8_t HAL_GPIO_ReadPin(char port, uint8_t pin);

/**
 * HAL_GPIO_TogglePin
 * -------------------
 * Invierte el estado actual de un pin de salida.
 *
 * @param port  Letra del puerto: 'B', 'C', 'D' [, 'A' en 1284P]
 * @param pin   Numero de pin: 0..7
 * @return      0 si OK, -1 si puerto o pin invalido
 */
int8_t HAL_GPIO_TogglePin(char port, uint8_t pin);

/**
 * HAL_GPIO_SetPullup
 * -------------------
 * Activa o desactiva el pull-up interno de un pin INPUT.
 *
 * @param port    Letra del puerto: 'B', 'C', 'D' [, 'A' en 1284P]
 * @param pin     Numero de pin: 0..7
 * @param enable  1U para activar, 0U para desactivar
 * @return        0 si OK, -1 si puerto o pin invalido
 */
int8_t HAL_GPIO_SetPullup(char port, uint8_t pin, uint8_t enable);

#endif /* GPIO_H_ */