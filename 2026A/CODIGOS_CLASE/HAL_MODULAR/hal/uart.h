/*
 * uart.h
 *
 * HAL - Capa de abstraccion de UART (Universal Asynchronous Receiver-Transmitter)
 * Targets soportados: ATmega328P (1x UART), ATmega1284P (2x UART)
 *
 * ----------------------------------------------------------------
 * QUE ES UART?
 * ----------------------------------------------------------------
 * UART es el protocolo de comunicacion serie mas comun en sistemas
 * embebidos. Permite enviar y recibir datos bit a bit entre dos
 * dispositivos usando solo dos cables:
 *
 *   TX  -> Transmit: el micro envia datos por este pin
 *   RX  -> Receive:  el micro recibe datos por este pin
 *
 * La comunicacion es asincrona: no hay un reloj compartido entre
 * los dos dispositivos. En su lugar, ambos acuerdan de antemano
 * una velocidad llamada "baud rate" (bits por segundo).
 *
 * Velocidades tipicas: 9600, 19200, 38400, 57600, 115200 bps.
 *
 * En AVR, el modulo de hardware que implementa UART se llama USART
 * (Universal Synchronous/Asynchronous Receiver-Transmitter) porque
 * tambien soporta modo sincrono, aunque aqui solo usamos el asincrono.
 * ----------------------------------------------------------------
 *
 * REGISTROS PRINCIPALES DEL USART EN AVR:
 *
 *   UBRRxH/L -> USART Baud Rate Register
 *               Controla la velocidad de comunicacion.
 *               El valor se calcula a partir del F_CPU y el baud rate.
 *
 *   UCSRxA   -> USART Control and Status Register A
 *               Contiene flags de estado: RXC (dato recibido),
 *               UDRE (buffer TX listo para nuevo dato), TXC (TX completo)
 *
 *   UCSRxB   -> USART Control and Status Register B
 *               Habilita TX, RX y sus interrupciones: RXEN, TXEN,
 *               RXCIE (IRQ al recibir), UDRIE (IRQ buffer TX vacio)
 *
 *   UCSRxC   -> USART Control and Status Register C
 *               Configura formato del frame: bits de datos (5..9),
 *               paridad (ninguna, par, impar), bits de stop (1 o 2)
 *
 *   UDRx     -> USART Data Register
 *               Escribir aqui envia un byte. Leer aqui recibe un byte.
 * ----------------------------------------------------------------
 *
 * ARQUITECTURA DE ESTE MODULO:
 *
 *   Modo polling    -> TX y RX esperan activamente al hardware.
 *                      Simple, pero bloquea el CPU mientras espera.
 *                      Util para debug o sistemas simples.
 *
 *   Modo IRQ + buffer circular ->
 *                      RX: cada byte recibido dispara una ISR que
 *                          lo guarda en un buffer circular. El codigo
 *                          principal lee del buffer cuando quiera.
 *                      TX: el codigo principal escribe en el buffer.
 *                          La ISR vacia el buffer automaticamente.
 *                      El CPU queda libre para hacer otras cosas.
 *
 * Created: 13/03/2026
 * Author: iSebas
 */

#ifndef UART_H_
#define UART_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>          /* FILE, fdev_setup_stream — para printf */

/* =========================================
   1. DETECCION DE TARGET
   ========================================= */

#if defined(__AVR_ATmega328P__)
    #define HAL_TARGET_328P
#elif defined(__AVR_ATmega1284P__)
    #define HAL_TARGET_1284P
#else
    #error "uart.h: Target no soportado. Usa -mmcu=atmega328p o -mmcu=atmega1284p"
#endif

/* =========================================
   2. CONFIGURACION DEL BUFFER CIRCULAR
   =========================================
   El buffer circular es un arreglo de tamano fijo que se usa
   como cola FIFO (primero en entrar, primero en salir).

   Funciona con dos indices: head (escritura) y tail (lectura).
   Cuando un indice llega al final del arreglo, vuelve al inicio
   — de ahi el nombre "circular".

        [ ][ ][D][D][D][ ][ ]
                ^        ^
               tail      head
        tail = donde leer el proximo byte
        head = donde escribir el proximo byte

   El tamano DEBE ser potencia de 2 (16, 32, 64, 128...).
   Esto permite usar una mascara de bits en lugar de modulo (%)
   para el wrap-around, lo que es mas rapido en AVR:
       indice = (indice + 1) & (TAMANIO - 1)   <- rapido (AND)
       indice = (indice + 1) % TAMANIO          <- lento  (DIV)
   ========================================= */

/** Tamano del buffer RX en bytes. Debe ser potencia de 2. */
#ifndef UART_RX_BUFFER_SIZE
    #define UART_RX_BUFFER_SIZE   64U
#endif

/** Tamano del buffer TX en bytes. Debe ser potencia de 2. */
#ifndef UART_TX_BUFFER_SIZE
    #define UART_TX_BUFFER_SIZE   64U
#endif

/* Verificacion en tiempo de compilacion: potencia de 2 */
#if (UART_RX_BUFFER_SIZE & (UART_RX_BUFFER_SIZE - 1U)) != 0U
    #error "UART_RX_BUFFER_SIZE debe ser potencia de 2 (16, 32, 64, 128...)"
#endif
#if (UART_TX_BUFFER_SIZE & (UART_TX_BUFFER_SIZE - 1U)) != 0U
    #error "UART_TX_BUFFER_SIZE debe ser potencia de 2 (16, 32, 64, 128...)"
#endif

/* =========================================
   3. TIPOS
   ========================================= */

/*
 * hal_uart_port_t — Instancia de UART a usar
 *
 * El ATmega328P tiene un solo UART: UART0.
 * El ATmega1284P tiene dos: UART0 y UART1.
 *
 * Todas las funciones reciben este parametro para saber
 * sobre que periferico operar.
 */
typedef enum {
    HAL_UART0 = 0U,
#if defined(HAL_TARGET_1284P)
    HAL_UART1 = 1U,
#endif
    HAL_UART_COUNT  /* Sentinel — NO usar como puerto */
} hal_uart_port_t;

/*
 * hal_uart_config_t — Configuracion completa de una instancia UART
 *
 * Se pasa a HAL_UART_Init() para configurar el periferico.
 * Agrupa todos los parametros en una sola estructura para que
 * la inicializacion sea clara y facil de modificar.
 */
typedef struct {
    uint32_t baud_rate;     /* Velocidad en bps: 9600, 115200, etc.  */
    uint8_t  data_bits;     /* Bits de datos: 5, 6, 7 u 8 (lo normal es 8) */
    uint8_t  stop_bits;     /* Bits de stop: 1 o 2                   */
    uint8_t  parity;        /* 0 = ninguna, 1 = par, 2 = impar       */
    uint8_t  use_irq;       /* 0 = polling, 1 = modo IRQ + buffer    */
} hal_uart_config_t;

/*
 * Valores predefinidos para paridad — hacen el codigo mas legible
 */
#define HAL_UART_PARITY_NONE   0U
#define HAL_UART_PARITY_EVEN   1U
#define HAL_UART_PARITY_ODD    2U

/*
 * Modos de operacion
 */
#define HAL_UART_MODE_POLLING  0U
#define HAL_UART_MODE_IRQ      1U

/* =========================================
   4. MACROS DE BAJO NIVEL
   =========================================
   Abstraen el acceso directo a registros del USART.
   Utiles para entender que hace el hardware por debajo.
   ========================================= */

/*
 * Calculo del valor UBRR (Baud Rate Register)
 *
 * Formula del datasheet AVR para modo asincrono normal:
 *   UBRR = (F_CPU / (16 * baud)) - 1
 *
 * F_CPU debe estar definido en el proyecto (Makefile o Atmel Studio).
 * Si no esta definido, usamos 16MHz como valor por defecto con aviso.
 */
#ifndef F_CPU
    #warning "F_CPU no definido. Usando 16000000UL por defecto."
    #define F_CPU 16000000UL
#endif

#define HAL_UART_UBRR(baud)     ((uint16_t)((F_CPU / (16UL * (baud))) - 1UL))

/** Verifica si el buffer de transmision esta listo para un nuevo byte */
#define HAL_UART_TX_READY(ucsra)    (((ucsra) & (1U << UDRE0)) != 0U)

/** Verifica si hay un byte disponible en el registro de recepcion */
#define HAL_UART_RX_READY(ucsra)    (((ucsra) & (1U << RXC0)) != 0U)

/* =========================================
   5. DECLARACIONES DE FUNCIONES
   ========================================= */

/* --- Inicializacion --- */

/**
 * HAL_UART_Init
 * --------------
 * Inicializa una instancia UART con la configuracion indicada.
 * Configura baud rate, formato del frame, y habilita TX/RX.
 * Si use_irq = 1, tambien habilita las interrupciones de RX y TX.
 *
 * Debe llamarse antes de cualquier otra funcion UART.
 * Requiere que las interrupciones globales esten habilitadas
 * (HAL_IRQ_ENABLE()) si se usa modo IRQ.
 *
 * @param port    HAL_UART0 [o HAL_UART1 en 1284P]
 * @param config  Puntero a estructura de configuracion
 * @return        0 si OK, -1 si parametro invalido
 */
int8_t HAL_UART_Init(hal_uart_port_t port, const hal_uart_config_t *config);

/**
 * HAL_UART_Deinit
 * ----------------
 * Deshabilita el UART y libera los buffers internos.
 *
 * @param port  HAL_UART0 [o HAL_UART1 en 1284P]
 */
void HAL_UART_Deinit(hal_uart_port_t port);

/* --- TX: Transmision --- */

/**
 * HAL_UART_SendByte
 * ------------------
 * Envia un byte por UART.
 *
 * Modo polling: espera a que el hardware este listo y envia.
 * Modo IRQ:     agrega el byte al buffer TX y retorna inmediatamente.
 *
 * @param port  HAL_UART0 [o HAL_UART1 en 1284P]
 * @param byte  Byte a enviar
 * @return      0 si OK, -1 si puerto invalido o buffer lleno (modo IRQ)
 */
int8_t HAL_UART_SendByte(hal_uart_port_t port, uint8_t byte);

/**
 * HAL_UART_SendString
 * --------------------
 * Envia una cadena de texto terminada en '\0'.
 *
 * @param port  HAL_UART0 [o HAL_UART1 en 1284P]
 * @param str   Puntero a la cadena (null-terminated)
 * @return      Numero de bytes enviados, o -1 si error
 */
int8_t HAL_UART_SendString(hal_uart_port_t port, const char *str);

/**
 * HAL_UART_SendInt
 * -----------------
 * Envia un entero con signo como texto decimal.
 * Ejemplo: HAL_UART_SendInt(HAL_UART0, -42) envia "-42\0"
 *
 * @param port   HAL_UART0 [o HAL_UART1 en 1284P]
 * @param value  Valor a enviar
 * @return       0 si OK, -1 si error
 */
int8_t HAL_UART_SendInt(hal_uart_port_t port, int32_t value);

/**
 * HAL_UART_SendHex
 * -----------------
 * Envia un byte como texto hexadecimal con prefijo "0x".
 * Ejemplo: HAL_UART_SendHex(HAL_UART0, 0xAB) envia "0xAB"
 *
 * @param port   HAL_UART0 [o HAL_UART1 en 1284P]
 * @param value  Byte a enviar en hex
 * @return       0 si OK, -1 si error
 */
int8_t HAL_UART_SendHex(hal_uart_port_t port, uint8_t value);

/* --- RX: Recepcion --- */

/**
 * HAL_UART_ReceiveByte
 * ---------------------
 * Lee un byte del UART.
 *
 * Modo polling: espera hasta que llegue un byte (bloqueante).
 * Modo IRQ:     lee del buffer circular. Si no hay datos, retorna -1.
 *
 * @param port    HAL_UART0 [o HAL_UART1 en 1284P]
 * @param byte    Puntero donde guardar el byte recibido
 * @return        0 si OK, -1 si no hay datos (IRQ) o error
 */
int8_t HAL_UART_ReceiveByte(hal_uart_port_t port, uint8_t *byte);

/**
 * HAL_UART_Available
 * -------------------
 * Retorna cuantos bytes hay disponibles en el buffer RX.
 * Solo util en modo IRQ. En modo polling siempre retorna 0.
 *
 * @param port  HAL_UART0 [o HAL_UART1 en 1284P]
 * @return      Numero de bytes en el buffer, o -1 si error
 */
int8_t HAL_UART_Available(hal_uart_port_t port);

/* --- printf --- */

/**
 * HAL_UART_SetStdout
 * -------------------
 * Redirige printf() y puts() al UART indicado.
 * Despues de llamar esta funcion, printf("Hola\n") envia
 * "Hola\n" por el UART en modo polling.
 *
 * Solo un UART puede ser stdout a la vez.
 *
 * @param port  HAL_UART0 [o HAL_UART1 en 1284P]
 * @return      0 si OK, -1 si puerto invalido
 */
int8_t HAL_UART_SetStdout(hal_uart_port_t port);

#endif /* UART_H_ */
