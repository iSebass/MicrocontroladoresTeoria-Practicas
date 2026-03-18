/*
 * timer.h
 *
 * HAL - Capa de abstraccion de Timers
 * Targets soportados: ATmega328P, ATmega1284P
 *
 * ----------------------------------------------------------------
 * QUE ES UN TIMER EN AVR?
 * ----------------------------------------------------------------
 * Un timer es un contador hardware que incrementa su valor
 * autonomamente, sin intervenir el CPU. Cuando llega a cierto
 * valor genera eventos (interrupciones, cambios en pines) que
 * el programa puede aprovechar.
 *
 * Usos tipicos:
 *   - Generar senales PWM para controlar motores, LEDs, servos
 *   - Medir tiempo transcurrido (timestamps, timeouts)
 *   - Generar interrupciones periodicas (tareas cada N ms)
 *   - Contar pulsos externos (encoders, frecuencimetros)
 *
 * TIMERS DISPONIBLES:
 *
 *   Timer0  8 bits  (0..255)   ambos targets  pines OC0A/OC0B
 *   Timer1  16 bits (0..65535) ambos targets  pines OC1A/OC1B
 *   Timer2  8 bits  (0..255)   ambos targets  pines OC2A/OC2B
 *                              puede usar reloj externo 32.768kHz (RTC)
 *   Timer3  16 bits (0..65535) solo 1284P     pines OC3A/OC3B
 *
 * REGISTROS PRINCIPALES (ejemplo Timer1):
 *
 *   TCCRnA / TCCRnB  -> Timer Control Registers
 *                       Configuran modo, prescaler y comportamiento
 *                       de los pines OC.
 *
 *   TCNTn            -> Timer Counter
 *                       Valor actual del contador. Lectura/escritura.
 *
 *   OCRnA / OCRnB    -> Output Compare Registers
 *                       Valor de comparacion. Cuando TCNTn == OCRn
 *                       ocurre un evento (IRQ, cambio en pin OC).
 *
 *   ICRn             -> Input Capture Register (solo timers 16 bits)
 *                       Captura el valor de TCNTn en un evento externo.
 *                       Tambien se usa como TOP en modo Fast PWM.
 *
 *   TIMSKn           -> Timer Interrupt Mask
 *                       Habilita interrupciones: overflow, compare A/B,
 *                       input capture.
 *
 *   TIFRn            -> Timer Interrupt Flag Register
 *                       Flags de eventos pendientes.
 * ----------------------------------------------------------------
 *
 * MODOS DE OPERACION:
 *
 *   NORMAL / OVERFLOW
 *     El contador cuenta de 0 hasta MAX (255 u 65535) y desborda
 *     de vuelta a 0. Al desbordar genera una IRQ de overflow.
 *     Util para: tareas periodicas, medir tiempo.
 *
 *   CTC (Clear Timer on Compare Match)
 *     El contador cuenta de 0 hasta OCRnA (valor configurable)
 *     y vuelve a 0. Genera IRQ al llegar al tope.
 *     Util para: frecuencias exactas, delays precisos.
 *     Ventaja sobre NORMAL: el periodo es configurable sin calculos
 *     complejos de precarga del contador.
 *
 *   FAST PWM
 *     El contador cuenta de 0 hasta TOP (255, 511, 1023 o ICR).
 *     El pin OC se pone en 1 al llegar a 0 y en 0 al llegar a OCR.
 *     Frecuencia fija, duty cycle variable.
 *     Util para: control de velocidad de motores DC, brillo de LEDs.
 *     Ventaja: maxima frecuencia de PWM para un prescaler dado.
 *
 *   PHASE CORRECT PWM
 *     El contador cuenta de 0 a TOP y luego de TOP a 0 (zig-zag).
 *     El pin OC cambia al pasar por OCR en ambas direcciones.
 *     Frecuencia mitad que Fast PWM, pero simetrica respecto al centro.
 *     Util para: control de servos, inversores, donde la simetria importa.
 * ----------------------------------------------------------------
 *
 * Created: 13/03/2026
 * Author: USER
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stddef.h>

/* =========================================
   1. DETECCION DE TARGET
   ========================================= */

#if defined(__AVR_ATmega328P__)
    #define HAL_TARGET_328P
#elif defined(__AVR_ATmega1284P__)
    #define HAL_TARGET_1284P
#else
    #error "timer.h: Target no soportado. Usa -mmcu=atmega328p o -mmcu=atmega1284p"
#endif

#ifndef F_CPU
    #warning "F_CPU no definido. Usando 16000000UL por defecto."
    #define F_CPU 16000000UL
#endif

/* =========================================
   2. TIPOS
   ========================================= */

/*
 * hal_timer_id_t — Identificador del timer a usar
 *
 * Cada timer tiene caracteristicas distintas:
 *   TIMER0 y TIMER2 son de 8 bits  -> contador 0..255
 *   TIMER1 y TIMER3 son de 16 bits -> contador 0..65535
 *
 * TIMER3 solo existe en el ATmega1284P.
 */
typedef enum {
    HAL_TIMER0 = 0U,
    HAL_TIMER1 = 1U,
    HAL_TIMER2 = 2U,
#if defined(HAL_TARGET_1284P)
    HAL_TIMER3 = 3U,
#endif
    HAL_TIMER_COUNT   /* Sentinel — NO usar como ID */
} hal_timer_id_t;

/*
 * hal_timer_mode_t — Modo de operacion del timer
 *
 * Ver descripcion completa en el encabezado del archivo.
 */
typedef enum {
    HAL_TIMER_MODE_NORMAL    = 0U,   /* Overflow libre                  */
    HAL_TIMER_MODE_CTC       = 1U,   /* Clear on Compare Match          */
    HAL_TIMER_MODE_FAST_PWM  = 2U,   /* Fast PWM                        */
    HAL_TIMER_MODE_PHASE_PWM = 3U    /* Phase Correct PWM               */
} hal_timer_mode_t;

/*
 * hal_timer_prescaler_t — Divisor del reloj del timer
 *
 * El prescaler divide F_CPU para obtener la frecuencia del timer.
 * A mayor prescaler, el timer cuenta mas lento pero puede medir
 * periodos mas largos.
 *
 * Ejemplo con F_CPU = 16MHz:
 *   /1    -> tick cada  62.5 ns  (muy rapido, periodo corto)
 *   /8    -> tick cada 500 ns
 *   /64   -> tick cada   4 us    (el mas comun para PWM)
 *   /256  -> tick cada  16 us
 *   /1024 -> tick cada  64 us    (util para periodos largos)
 *
 * NOTA: Timer2 tiene prescalers diferentes a Timer0/1/3.
 *       /32 y /128 son exclusivos de Timer2.
 *       Esta HAL usa los mismos valores enum para todos;
 *       timer.c aplica los bits correctos segun el timer.
 */
typedef enum {
    HAL_TIMER_PRESCALER_1    = 1U,
    HAL_TIMER_PRESCALER_8    = 2U,
    HAL_TIMER_PRESCALER_32   = 3U,   /* Solo Timer2 */
    HAL_TIMER_PRESCALER_64   = 4U,
    HAL_TIMER_PRESCALER_128  = 5U,   /* Solo Timer2 */
    HAL_TIMER_PRESCALER_256  = 6U,
    HAL_TIMER_PRESCALER_1024 = 7U
} hal_timer_prescaler_t;

/*
 * hal_timer_channel_t — Canal de comparacion (OC pin)
 *
 * Cada timer tiene dos canales de comparacion: A y B.
 * Cada canal puede controlar un pin de salida OC independiente.
 * En modo PWM, cada canal tiene su propio duty cycle (OCRnA/OCRnB).
 */
typedef enum {
    HAL_TIMER_CH_A = 0U,
    HAL_TIMER_CH_B = 1U
} hal_timer_channel_t;

/*
 * hal_timer_irq_src_t — Fuente de interrupcion del timer
 *
 * Cada timer puede generar hasta 3 tipos de IRQ:
 *   OVERFLOW  -> el contador desborda (llega al MAX y vuelve a 0)
 *   COMPARE_A -> TCNTn == OCRnA
 *   COMPARE_B -> TCNTn == OCRnB
 */
typedef enum {
    HAL_TIMER_IRQ_OVERFLOW  = 0U,
    HAL_TIMER_IRQ_COMPARE_A = 1U,
    HAL_TIMER_IRQ_COMPARE_B = 2U
} hal_timer_irq_src_t;

/*
 * hal_timer_callback_t — Tipo de callback para eventos del timer
 *
 * Funcion void(void) que se llama desde la ISR correspondiente.
 * IMPORTANTE: mantener el callback corto — estamos en una ISR.
 */
typedef void (*hal_timer_callback_t)(void);

/*
 * hal_timer_config_t — Configuracion completa de un timer
 *
 * Se pasa a HAL_Timer_Init() para configurar el periferico.
 */
typedef struct {
    hal_timer_mode_t       mode;        /* Modo de operacion              */
    hal_timer_prescaler_t  prescaler;   /* Divisor de reloj               */
    uint16_t               top;         /* Valor TOP para CTC y PWM       */
                                        /* En NORMAL se ignora            */
                                        /* En CTC: periodo = top+1 ticks  */
                                        /* En PWM 8 bits: ignorado (=255) */
                                        /* En PWM 16 bits: valor ICR1     */
} hal_timer_config_t;

/* =========================================
   3. MACROS DE UTILIDAD
   =========================================
   Formulas para calcular configuraciones del timer
   sin necesidad de leer el datasheet cada vez.
   ========================================= */

/*
 * HAL_TIMER_CTC_TOP(freq_hz, prescaler_val)
 * ------------------------------------------
 * Calcula el valor OCR para generar una frecuencia dada en modo CTC.
 *
 * Formula: TOP = (F_CPU / (2 * prescaler * freq_hz)) - 1
 *
 * Ejemplo: generar 1kHz con prescaler 64 y F_CPU 16MHz:
 *   TOP = (16000000 / (2 * 64 * 1000)) - 1 = 124
 *
 * Uso:
 *   cfg.top = HAL_TIMER_CTC_TOP(1000UL, 64UL);
 */
#define HAL_TIMER_CTC_TOP(freq_hz, prescaler_val) \
    ((uint16_t)((F_CPU / (2UL * (prescaler_val) * (freq_hz))) - 1UL))

/*
 * HAL_TIMER_PWM_TOP(freq_hz, prescaler_val)
 * ------------------------------------------
 * Calcula el valor ICR (TOP) para Fast PWM en timers de 16 bits
 * con una frecuencia de PWM deseada.
 *
 * Formula: TOP = (F_CPU / (prescaler * freq_hz)) - 1
 *
 * Ejemplo: PWM a 50Hz (servo) con prescaler 8 y F_CPU 16MHz:
 *   TOP = (16000000 / (8 * 50)) - 1 = 39999
 */
#define HAL_TIMER_PWM_TOP(freq_hz, prescaler_val) \
    ((uint16_t)((F_CPU / ((prescaler_val) * (freq_hz))) - 1UL))

/*
 * HAL_TIMER_DUTY(top, percent)
 * -----------------------------
 * Calcula el valor OCR para un duty cycle en porcentaje (0..100).
 *
 * Ejemplo: 75% de duty cycle con TOP=39999:
 *   OCR = HAL_TIMER_DUTY(39999, 75) = 29999
 */
#define HAL_TIMER_DUTY(top, percent) \
    ((uint16_t)(((uint32_t)(top) * (uint32_t)(percent)) / 100UL))

/*
 * HAL_TIMER_US_TO_TICKS(us, prescaler_val)
 * -----------------------------------------
 * Convierte microsegundos a ticks del timer.
 *
 * Util para cargar el contador con un valor inicial en modo NORMAL
 * y generar un periodo especifico.
 */
#define HAL_TIMER_US_TO_TICKS(us, prescaler_val) \
    ((uint16_t)(((uint32_t)(us) * (F_CPU / 1000000UL)) / (prescaler_val)))

/* =========================================
   4. DECLARACIONES DE FUNCIONES
   ========================================= */

/* --- Inicializacion y control --- */

/**
 * HAL_Timer_Init
 * ---------------
 * Configura un timer con el modo y prescaler indicados.
 * No lo habilita ni activa interrupciones — eso es separado.
 *
 * @param id      Timer a configurar: HAL_TIMER0..HAL_TIMER3
 * @param config  Puntero a estructura de configuracion
 * @return        0 si OK, -1 si parametro invalido
 */
int8_t HAL_Timer_Init(hal_timer_id_t id, const hal_timer_config_t *config);

/**
 * HAL_Timer_Start
 * ----------------
 * Inicia el timer aplicando el prescaler (el contador comienza a correr).
 * Separado de Init para poder configurar callbacks antes de arrancar.
 *
 * @param id  Timer a iniciar
 * @return    0 si OK, -1 si error
 */
int8_t HAL_Timer_Start(hal_timer_id_t id);

/**
 * HAL_Timer_Stop
 * ---------------
 * Detiene el timer (prescaler = 0, contador congela).
 * No borra la configuracion ni los callbacks.
 *
 * @param id  Timer a detener
 */
void HAL_Timer_Stop(hal_timer_id_t id);

/**
 * HAL_Timer_Reset
 * ----------------
 * Pone el contador TCNTn en 0.
 *
 * @param id  Timer a resetear
 */
void HAL_Timer_Reset(hal_timer_id_t id);

/* --- Interrupciones y callbacks --- */

/**
 * HAL_Timer_EnableIRQ
 * --------------------
 * Habilita una fuente de interrupcion del timer.
 *
 * @param id    Timer
 * @param src   HAL_TIMER_IRQ_OVERFLOW, COMPARE_A o COMPARE_B
 * @return      0 si OK, -1 si error
 */
int8_t HAL_Timer_EnableIRQ(hal_timer_id_t id, hal_timer_irq_src_t src);

/**
 * HAL_Timer_DisableIRQ
 * ---------------------
 * Deshabilita una fuente de interrupcion del timer.
 *
 * @param id    Timer
 * @param src   Fuente a deshabilitar
 */
void HAL_Timer_DisableIRQ(hal_timer_id_t id, hal_timer_irq_src_t src);

/**
 * HAL_Timer_RegisterCallback
 * ---------------------------
 * Asocia una funcion a un evento del timer.
 * La funcion se llama desde la ISR correspondiente.
 *
 * IMPORTANTE: los callbacks de timer se ejecutan en contexto de ISR.
 * Deben ser cortos, sin llamadas bloqueantes ni printf.
 *
 * @param id        Timer
 * @param src       Evento: overflow, compare A o compare B
 * @param callback  Funcion void(void), o NULL para desregistrar
 * @return          0 si OK, -1 si parametro invalido
 */
int8_t HAL_Timer_RegisterCallback(hal_timer_id_t      id,
                                   hal_timer_irq_src_t src,
                                   hal_timer_callback_t callback);

/* --- PWM --- */

/**
 * HAL_Timer_PWM_Enable
 * ---------------------
 * Habilita la salida PWM en el pin OC del canal indicado.
 * Conecta el pin OC al timer (modo no-inversor por defecto).
 *
 * Para que el pin funcione como PWM debe estar configurado
 * como OUTPUT en gpio.h antes de llamar esta funcion.
 *
 * @param id      Timer
 * @param channel HAL_TIMER_CH_A o HAL_TIMER_CH_B
 * @return        0 si OK, -1 si error
 */
int8_t HAL_Timer_PWM_Enable(hal_timer_id_t id, hal_timer_channel_t channel);

/**
 * HAL_Timer_PWM_Disable
 * ----------------------
 * Desconecta el pin OC del timer (el pin queda como GPIO normal).
 *
 * @param id      Timer
 * @param channel Canal a deshabilitar
 */
void HAL_Timer_PWM_Disable(hal_timer_id_t id, hal_timer_channel_t channel);

/**
 * HAL_Timer_PWM_SetDuty
 * ----------------------
 * Establece el duty cycle del canal PWM como porcentaje (0..100).
 *
 * Internamente calcula el valor OCR a partir del TOP configurado.
 * 0   = siempre LOW  (0% encendido)
 * 100 = siempre HIGH (100% encendido)
 *
 * @param id      Timer
 * @param channel Canal PWM
 * @param duty    Duty cycle 0..100
 * @return        0 si OK, -1 si error
 */
int8_t HAL_Timer_PWM_SetDuty(hal_timer_id_t      id,
                               hal_timer_channel_t channel,
                               uint8_t             duty);

/**
 * HAL_Timer_PWM_SetRaw
 * ---------------------
 * Establece el valor OCR directamente (sin conversion a porcentaje).
 * Util cuando se necesita maxima precision o se calcula el valor externamente.
 *
 * @param id      Timer
 * @param channel Canal PWM
 * @param value   Valor OCR (0..TOP)
 * @return        0 si OK, -1 si error
 */
int8_t HAL_Timer_PWM_SetRaw(hal_timer_id_t      id,
                              hal_timer_channel_t channel,
                              uint16_t            value);

/* --- Timestamp y medicion de tiempo --- */

/**
 * HAL_Timer_Timestamp_Init
 * -------------------------
 * Inicializa el sistema de timestamp usando Timer1 (16 bits).
 * Configura Timer1 en modo CTC con overflow cada 1ms.
 * Despues de llamar esta funcion, HAL_Timer_Millis() retorna
 * los milisegundos transcurridos desde el inicio.
 *
 * NOTA: esta funcion toma posesion de Timer1 para el timestamp.
 * No usar Timer1 para otras cosas si se usa el sistema de timestamp.
 *
 * @return  0 si OK, -1 si error
 */
int8_t HAL_Timer_Timestamp_Init(void);

/**
 * HAL_Timer_Millis
 * -----------------
 * Retorna los milisegundos transcurridos desde HAL_Timer_Timestamp_Init().
 * Similar al millis() de Arduino.
 *
 * Resolucion: 1ms.
 * Desbordamiento: cada ~49 dias (uint32_t).
 *
 * @return  Milisegundos transcurridos
 */
uint32_t HAL_Timer_Millis(void);

/**
 * HAL_Timer_Micros
 * -----------------
 * Retorna los microsegundos transcurridos.
 * Menor precision que Millis — resolucion depende del prescaler.
 *
 * @return  Microsegundos transcurridos
 */
uint32_t HAL_Timer_Micros(void);

/**
 * HAL_Timer_DelayMs
 * ------------------
 * Bloquea la ejecucion durante N milisegundos usando el timestamp.
 * Requiere haber llamado HAL_Timer_Timestamp_Init() antes.
 *
 * A diferencia de _delay_ms() de avr-libc, esta funcion permite
 * que las interrupciones sigan ejecutandose durante la espera.
 *
 * @param ms  Milisegundos a esperar
 */
void HAL_Timer_DelayMs(uint32_t ms);

#endif /* TIMER_H_ */
