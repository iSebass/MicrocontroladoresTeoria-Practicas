/*
 * uart.c
 *
 * HAL - Implementacion de la capa UART
 * Targets soportados: ATmega328P, ATmega1284P
 *
 * Created: 13/03/2026
 * Author: USER
 */

#include "uart.h"
#include "interrupt.h"   /* HAL_CRITICAL_ENTER / HAL_CRITICAL_EXIT */
#include <stdio.h>
#include <string.h>      /* strlen */
#include <stdlib.h>      /* itoa   */

/* =========================================
   1. BUFFER CIRCULAR
   =========================================
   Un buffer circular (ring buffer) es la estructura de datos
   central de este modulo. Permite que la ISR deposite bytes
   recibidos sin bloquear al codigo principal.

   Estructura:
     data[]  -> arreglo donde viven los bytes
     head    -> indice de escritura (ISR escribe aqui)
     tail    -> indice de lectura  (codigo principal lee aqui)
     mask    -> tamanio - 1, usado para el wrap-around eficiente

   Cuando head == tail: el buffer esta VACIO.
   Cuando (head + 1) & mask == tail: el buffer esta LLENO.
   Se sacrifica una posicion para distinguir vacio de lleno.

   Ejemplo con buffer de 8 bytes (mask = 7 = 0b0111):
     Escribir: data[head] = byte; head = (head + 1) & mask;
     Leer:     byte = data[tail]; tail = (tail + 1) & mask;
   ========================================= */

typedef struct {
    uint8_t  data[UART_RX_BUFFER_SIZE];
    volatile uint8_t head;
    volatile uint8_t tail;
    uint8_t  mask;
} hal_uart_rx_buf_t;

typedef struct {
    uint8_t  data[UART_TX_BUFFER_SIZE];
    volatile uint8_t head;
    volatile uint8_t tail;
    uint8_t  mask;
} hal_uart_tx_buf_t;

/* =========================================
   2. ESTADO INTERNO POR INSTANCIA UART
   =========================================
   Cada instancia UART tiene su propio estado: buffers,
   modo de operacion, y si fue inicializada.
   ========================================= */

typedef struct {
    hal_uart_rx_buf_t rx_buf;
    hal_uart_tx_buf_t tx_buf;
    uint8_t           use_irq;      /* 0 = polling, 1 = IRQ  */
    uint8_t           initialized;  /* 0 = no init, 1 = listo */
} hal_uart_state_t;

static hal_uart_state_t _uart_state[HAL_UART_COUNT];

/* =========================================
   3. ACCESO A REGISTROS SEGUN INSTANCIA
   =========================================
   ATmega328P tiene USART0 (registros: UBRR0, UCSR0x, UDR0).
   ATmega1284P agrega USART1 (registros: UBRR1, UCSR1x, UDR1).

   Esta estructura agrupa punteros a los registros de cada
   instancia para que el resto del codigo no necesite hacer
   if/else por cada acceso a registro.
   ========================================= */

typedef struct {
    volatile uint16_t *ubrr;
    volatile uint8_t  *ucsra;
    volatile uint8_t  *ucsrb;
    volatile uint8_t  *ucsrc;
    volatile uint8_t  *udr;
} hal_uart_regs_t;

static int8_t get_uart_regs(hal_uart_port_t port, hal_uart_regs_t *regs)
{
    switch (port) {
        case HAL_UART0:
            regs->ubrr  = &UBRR0;
            regs->ucsra = &UCSR0A;
            regs->ucsrb = &UCSR0B;
            regs->ucsrc = &UCSR0C;
            regs->udr   = &UDR0;
            return 0;

#if defined(HAL_TARGET_1284P)
        case HAL_UART1:
            regs->ubrr  = &UBRR1;
            regs->ucsra = &UCSR1A;
            regs->ucsrb = &UCSR1B;
            regs->ucsrc = &UCSR1C;
            regs->udr   = &UDR1;
            return 0;
#endif

        default:
            return -1;
    }
}

/* =========================================
   4. OPERACIONES SOBRE EL BUFFER CIRCULAR
   ========================================= */

/* Escribe un byte en el buffer TX. Retorna 0 si OK, -1 si lleno. */
static int8_t tx_buf_write(hal_uart_tx_buf_t *buf, uint8_t byte)
{
    uint8_t next_head = (uint8_t)((buf->head + 1U) & buf->mask);
    if (next_head == buf->tail) {
        return -1;  /* Buffer lleno */
    }
    buf->data[buf->head] = byte;
    buf->head = next_head;
    return 0;
}

/* Lee un byte del buffer TX. Retorna 0 si OK, -1 si vacio. */
static int8_t tx_buf_read(hal_uart_tx_buf_t *buf, uint8_t *byte)
{
    if (buf->head == buf->tail) {
        return -1;  /* Buffer vacio */
    }
    *byte = buf->data[buf->tail];
    buf->tail = (uint8_t)((buf->tail + 1U) & buf->mask);
    return 0;
}

/* Escribe un byte en el buffer RX. Llamada desde la ISR. */
static int8_t rx_buf_write(hal_uart_rx_buf_t *buf, uint8_t byte)
{
    uint8_t next_head = (uint8_t)((buf->head + 1U) & buf->mask);
    if (next_head == buf->tail) {
        return -1;  /* Buffer lleno — byte perdido */
    }
    buf->data[buf->head] = byte;
    buf->head = next_head;
    return 0;
}

/* Lee un byte del buffer RX. Llamada desde el codigo principal. */
static int8_t rx_buf_read(hal_uart_rx_buf_t *buf, uint8_t *byte)
{
    if (buf->head == buf->tail) {
        return -1;  /* Buffer vacio */
    }
    *byte = buf->data[buf->tail];
    buf->tail = (uint8_t)((buf->tail + 1U) & buf->mask);
    return 0;
}

/* Retorna cuantos bytes hay en el buffer RX */
static uint8_t rx_buf_available(const hal_uart_rx_buf_t *buf)
{
    return (uint8_t)((buf->head - buf->tail) & buf->mask);
}

/* =========================================
   5. INICIALIZACION
   ========================================= */

int8_t HAL_UART_Init(hal_uart_port_t port, const hal_uart_config_t *config)
{
    if (port >= HAL_UART_COUNT || config == NULL) {
        return -1;
    }

    hal_uart_regs_t regs;
    if (get_uart_regs(port, &regs) != 0) {
        return -1;
    }

    hal_uart_state_t *st = &_uart_state[port];

    /* Inicializar buffers */
    st->rx_buf.head = 0U;
    st->rx_buf.tail = 0U;
    st->rx_buf.mask = (uint8_t)(UART_RX_BUFFER_SIZE - 1U);

    st->tx_buf.head = 0U;
    st->tx_buf.tail = 0U;
    st->tx_buf.mask = (uint8_t)(UART_TX_BUFFER_SIZE - 1U);

    st->use_irq     = config->use_irq;
    st->initialized = 1U;

    /*
     * Configurar Baud Rate
     *
     * UBRR = (F_CPU / (16 * baud)) - 1
     * Se escribe primero el byte alto (H) y luego el bajo (L).
     * En algunos AVR, escribir UBRRL dispara la actualizacion.
     */
    *regs.ubrr = HAL_UART_UBRR(config->baud_rate);

    /*
     * Configurar UCSRxC — formato del frame
     *
     * Bits de datos: UCSZ1:0 en UCSRxC (y UCSZ2 en UCSRxB para 9 bits)
     *   5 bits -> 000 | 6 bits -> 001 | 7 bits -> 010 | 8 bits -> 011
     *
     * Paridad: UPM1:0
     *   Sin paridad -> 00 | Par -> 10 | Impar -> 11
     *
     * Bits de stop: USBS
     *   1 stop bit -> 0 | 2 stop bits -> 1
     */
    uint8_t ucsrc = (1U << URSEL0);  /* URSEL: selecciona UCSRxC vs UBRRH */

    /* Bits de datos */
    uint8_t data_bits = (config->data_bits >= 8U) ? 3U :
                        (config->data_bits == 7U) ? 2U :
                        (config->data_bits == 6U) ? 1U : 0U;
    ucsrc |= (uint8_t)(data_bits << UCSZ00);

    /* Paridad */
    if (config->parity == HAL_UART_PARITY_EVEN) {
        ucsrc |= (1U << UPM01);
    } else if (config->parity == HAL_UART_PARITY_ODD) {
        ucsrc |= (1U << UPM01) | (1U << UPM00);
    }

    /* Bits de stop */
    if (config->stop_bits == 2U) {
        ucsrc |= (1U << USBS0);
    }

    *regs.ucsrc = ucsrc;

    /*
     * Configurar UCSRxB — habilitar TX y RX
     *
     * RXEN: habilita el receptor
     * TXEN: habilita el transmisor
     * RXCIE: habilita IRQ al recibir un byte (solo modo IRQ)
     * UDRIE: habilita IRQ cuando el buffer TX queda vacio
     *        (se activa solo cuando hay datos para enviar)
     */
    uint8_t ucsrb = (1U << RXEN0) | (1U << TXEN0);
    if (config->use_irq != 0U) {
        ucsrb |= (1U << RXCIE0);  /* IRQ de RX siempre activa en modo IRQ */
        /* UDRIE se activa solo cuando hay datos en el buffer TX */
    }
    *regs.ucsrb = ucsrb;

    return 0;
}

void HAL_UART_Deinit(hal_uart_port_t port)
{
    if (port >= HAL_UART_COUNT) return;

    hal_uart_regs_t regs;
    if (get_uart_regs(port, &regs) != 0) return;

    /* Deshabilitar TX, RX e interrupciones */
    *regs.ucsrb = 0U;

    _uart_state[port].initialized = 0U;
}

/* =========================================
   6. TX — TRANSMISION
   ========================================= */

/*
 * Envia un byte en modo polling.
 * Espera activamente hasta que el registro UDR este libre.
 * "Activamente" significa que el CPU no hace nada mas mientras espera.
 */
static int8_t uart_send_byte_polling(const hal_uart_regs_t *regs, uint8_t byte)
{
    /* Esperar hasta que UDRE (Data Register Empty) este en 1 */
    while (!HAL_UART_TX_READY(*regs->ucsra)) {
        /* CPU bloqueado esperando hardware */
    }
    *regs->udr = byte;
    return 0;
}

/*
 * Agrega un byte al buffer TX en modo IRQ.
 * La ISR USART_UDRE se encargara de enviarlo.
 */
static int8_t uart_send_byte_irq(hal_uart_port_t    port,
                                  const hal_uart_regs_t *regs,
                                  uint8_t            byte)
{
    hal_uart_state_t *st = &_uart_state[port];

    uint8_t sreg;
    HAL_CRITICAL_ENTER(sreg);
    int8_t res = tx_buf_write(&st->tx_buf, byte);
    /*
     * Activar UDRIE: habilita la IRQ "buffer TX vacio".
     * Cuando el hardware termine de enviar el byte actual,
     * disparara la ISR que lee el siguiente byte del buffer.
     */
    if (res == 0) {
        *regs->ucsrb |= (1U << UDRIE0);
    }
    HAL_CRITICAL_EXIT(sreg);

    return res;
}

int8_t HAL_UART_SendByte(hal_uart_port_t port, uint8_t byte)
{
    if (port >= HAL_UART_COUNT) return -1;

    hal_uart_state_t *st = &_uart_state[port];
    if (st->initialized == 0U) return -1;

    hal_uart_regs_t regs;
    if (get_uart_regs(port, &regs) != 0) return -1;

    if (st->use_irq != 0U) {
        return uart_send_byte_irq(port, &regs, byte);
    } else {
        return uart_send_byte_polling(&regs, byte);
    }
}

int8_t HAL_UART_SendString(hal_uart_port_t port, const char *str)
{
    if (str == NULL) return -1;

    int8_t count = 0;
    while (*str != '\0') {
        if (HAL_UART_SendByte(port, (uint8_t)*str) != 0) {
            return -1;
        }
        str++;
        count++;
    }
    return count;
}

int8_t HAL_UART_SendInt(hal_uart_port_t port, int32_t value)
{
    /*
     * itoa convierte un entero a string en la base indicada.
     * El buffer necesita 12 caracteres para el peor caso de int32_t:
     * "-2147483648\0" = 12 caracteres.
     */
    char buf[12];
    itoa((int)value, buf, 10);
    return HAL_UART_SendString(port, buf);
}

int8_t HAL_UART_SendHex(hal_uart_port_t port, uint8_t value)
{
    /*
     * Formato: "0x" + dos digitos hex.
     * El nibble alto es value >> 4, el bajo es value & 0x0F.
     * Si el digito es >= 10, le sumamos 'A' - 10 para obtener A..F.
     */
    const char hex_chars[] = "0123456789ABCDEF";
    char buf[5];  /* "0x" + 2 chars + '\0' */

    buf[0] = '0';
    buf[1] = 'x';
    buf[2] = hex_chars[(value >> 4U) & 0x0FU];
    buf[3] = hex_chars[value & 0x0FU];
    buf[4] = '\0';

    return HAL_UART_SendString(port, buf);
}

/* =========================================
   7. RX — RECEPCION
   ========================================= */

int8_t HAL_UART_ReceiveByte(hal_uart_port_t port, uint8_t *byte)
{
    if (port >= HAL_UART_COUNT || byte == NULL) return -1;

    hal_uart_state_t *st = &_uart_state[port];
    if (st->initialized == 0U) return -1;

    if (st->use_irq != 0U) {
        /* Modo IRQ: leer del buffer circular */
        return rx_buf_read(&st->rx_buf, byte);
    } else {
        /* Modo polling: esperar hasta que llegue un byte */
        hal_uart_regs_t regs;
        if (get_uart_regs(port, &regs) != 0) return -1;

        while (!HAL_UART_RX_READY(*regs.ucsra)) {
            /* Esperar dato */
        }
        *byte = *regs.udr;
        return 0;
    }
}

int8_t HAL_UART_Available(hal_uart_port_t port)
{
    if (port >= HAL_UART_COUNT) return -1;
    if (_uart_state[port].initialized == 0U) return -1;
    return (int8_t)rx_buf_available(&_uart_state[port].rx_buf);
}

/* =========================================
   8. ISR — RUTINAS DE INTERRUPCION
   =========================================
   Estas ISR se activan automaticamente cuando el hardware
   UART genera un evento. Son muy cortas por diseno: solo
   mueven bytes entre los registros del hardware y los buffers.

   REGLA DE ORO de las ISR: hacerlas lo mas cortas posible.
   Nunca esperar, nunca bloquear, nunca usar printf dentro de una ISR.
   ========================================= */

/*
 * ISR de recepcion UART0 (USART_RX)
 *
 * Se dispara cuando el hardware recibio un byte completo.
 * Lee UDR0 (obligatorio — si no se lee, se pierden datos)
 * y lo deposita en el buffer circular RX.
 */
ISR(USART_RX_vect)
{
    uint8_t byte = UDR0;  /* Leer ANTES de cualquier otra cosa */

    if (_uart_state[HAL_UART0].initialized != 0U &&
        _uart_state[HAL_UART0].use_irq     != 0U) {
        rx_buf_write(&_uart_state[HAL_UART0].rx_buf, byte);
    }
}

/*
 * ISR de buffer TX vacio UART0 (USART_UDRE)
 *
 * Se dispara cuando el registro UDR0 quedo vacio y esta
 * listo para recibir el siguiente byte a transmitir.
 * Lee el buffer TX y envía el siguiente byte.
 * Si el buffer quedo vacio, deshabilita esta IRQ para no
 * seguir disparandose innecesariamente.
 */
ISR(USART_UDRE_vect)
{
    uint8_t byte;
    hal_uart_state_t *st = &_uart_state[HAL_UART0];

    if (tx_buf_read(&st->tx_buf, &byte) == 0) {
        UDR0 = byte;
    } else {
        /* Buffer TX vacio: deshabilitar esta IRQ */
        UCSR0B &= (uint8_t)(~(1U << UDRIE0));
    }
}

#if defined(HAL_TARGET_1284P)
/*
 * ISR de recepcion UART1 — identica a UART0 pero para la segunda instancia
 */
ISR(USART1_RX_vect)
{
    uint8_t byte = UDR1;

    if (_uart_state[HAL_UART1].initialized != 0U &&
        _uart_state[HAL_UART1].use_irq     != 0U) {
        rx_buf_write(&_uart_state[HAL_UART1].rx_buf, byte);
    }
}

ISR(USART1_UDRE_vect)
{
    uint8_t byte;
    hal_uart_state_t *st = &_uart_state[HAL_UART1];

    if (tx_buf_read(&st->tx_buf, &byte) == 0) {
        UDR1 = byte;
    } else {
        UCSR1B &= (uint8_t)(~(1U << UDRIE1));
    }
}
#endif

/* =========================================
   9. REDIRECCION DE printf A UART
   =========================================
   avr-libc permite redirigir stdout a un dispositivo propio
   implementando una funcion de escritura de un solo caracter
   y asociandola a un FILE stream con fdev_setup_stream().

   Despues de llamar HAL_UART_SetStdout(), cualquier llamada
   a printf(), puts() o putchar() envia los datos por UART.

   Nota: printf en AVR es costoso en flash y RAM. Para uso
   intensivo considera usar HAL_UART_SendString / SendInt
   directamente. printf es comodo para debug.
   ========================================= */

static hal_uart_port_t _stdout_port = HAL_UART0;

/*
 * uart_putchar — funcion de escritura para avr-libc
 *
 * Esta es la "pieza de conexion" entre printf y el UART.
 * avr-libc la llama cada vez que printf quiere enviar un caracter.
 * El parametro stream permite saber a que FILE stream pertenece,
 * pero aqui no lo usamos porque solo tenemos un stdout a la vez.
 */
static int uart_putchar(char c, FILE *stream)
{
    (void)stream;  /* No se usa — evitar warning de compilador */

    /* Convencion de terminales: \n -> \r\n para nueva linea correcta */
    if (c == '\n') {
        HAL_UART_SendByte(_stdout_port, '\r');
    }
    return (HAL_UART_SendByte(_stdout_port, (uint8_t)c) == 0) ? 0 : -1;
}

/* FILE stream estatico que usa uart_putchar como funcion de salida */
static FILE _uart_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

int8_t HAL_UART_SetStdout(hal_uart_port_t port)
{
    if (port >= HAL_UART_COUNT) return -1;
    if (_uart_state[port].initialized == 0U) return -1;

    _stdout_port = port;

    /*
     * stdout es una variable global de avr-libc que apunta al
     * FILE stream de salida estandar. Al redirigirla a nuestro
     * stream, printf usara uart_putchar automaticamente.
     */
    stdout = &_uart_stdout;

    return 0;
}

/* =========================================
   10. EJEMPLOS DE USO
   ========================================= */

/*
 * -----------------------------------------------------------
 * EJEMPLO 1: Debug por UART con printf (modo polling)
 * -----------------------------------------------------------
 *
 * #include "uart.h"
 * #include "interrupt.h"
 *
 * int main(void) {
 *     hal_uart_config_t cfg = {
 *         .baud_rate = 115200U,
 *         .data_bits = 8U,
 *         .stop_bits = 1U,
 *         .parity    = HAL_UART_PARITY_NONE,
 *         .use_irq   = HAL_UART_MODE_POLLING
 *     };
 *
 *     HAL_UART_Init(HAL_UART0, &cfg);
 *     HAL_UART_SetStdout(HAL_UART0);
 *
 *     printf("Sistema iniciado\n");
 *
 *     uint16_t contador = 0U;
 *     while (1) {
 *         printf("Contador: %u\n", contador++);
 *     }
 * }
 * -----------------------------------------------------------
 *
 * EJEMPLO 2: Recepcion por interrupciones con buffer
 * -----------------------------------------------------------
 * El CPU no se bloquea esperando datos. Puede hacer otras
 * cosas y revisar el buffer cuando quiera.
 *
 * #include "uart.h"
 * #include "interrupt.h"
 *
 * int main(void) {
 *     hal_uart_config_t cfg = {
 *         .baud_rate = 9600U,
 *         .data_bits = 8U,
 *         .stop_bits = 1U,
 *         .parity    = HAL_UART_PARITY_NONE,
 *         .use_irq   = HAL_UART_MODE_IRQ
 *     };
 *
 *     HAL_UART_Init(HAL_UART0, &cfg);
 *     HAL_IRQ_ENABLE();
 *
 *     while (1) {
 *         if (HAL_UART_Available(HAL_UART0) > 0) {
 *             uint8_t byte;
 *             HAL_UART_ReceiveByte(HAL_UART0, &byte);
 *             HAL_UART_SendByte(HAL_UART0, byte);  // eco
 *         }
 *         // ... otras tareas del sistema ...
 *     }
 * }
 * -----------------------------------------------------------
 *
 * EJEMPLO 3: Enviar datos numericos sin printf
 * -----------------------------------------------------------
 * Mas liviano que printf — no jala toda la libreria de formato.
 *
 * HAL_UART_SendString(HAL_UART0, "Temperatura: ");
 * HAL_UART_SendInt(HAL_UART0, 23);
 * HAL_UART_SendString(HAL_UART0, " C\n");
 *
 * HAL_UART_SendString(HAL_UART0, "Registro: ");
 * HAL_UART_SendHex(HAL_UART0, 0xB4);
 * HAL_UART_SendString(HAL_UART0, "\n");
 * -----------------------------------------------------------
 *
 * EJEMPLO 4: Dos UARTs en ATmega1284P
 * -----------------------------------------------------------
 * UART0 para debug (printf), UART1 para comunicar con otro
 * dispositivo (sensor, modulo BT, etc.)
 *
 * hal_uart_config_t debug_cfg = { .baud_rate=115200U, ... };
 * hal_uart_config_t sensor_cfg = { .baud_rate=9600U, .use_irq=HAL_UART_MODE_IRQ, ... };
 *
 * HAL_UART_Init(HAL_UART0, &debug_cfg);
 * HAL_UART_Init(HAL_UART1, &sensor_cfg);
 * HAL_UART_SetStdout(HAL_UART0);
 * HAL_IRQ_ENABLE();
 *
 * printf("Esperando datos del sensor...\n");
 *
 * while (1) {
 *     if (HAL_UART_Available(HAL_UART1) > 0) {
 *         uint8_t dato;
 *         HAL_UART_ReceiveByte(HAL_UART1, &dato);
 *         printf("Sensor: 0x%02X\n", dato);
 *     }
 * }
 * -----------------------------------------------------------
 */
