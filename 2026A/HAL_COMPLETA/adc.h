/*
 * adc.h
 *
 * HAL - Capa de abstraccion del ADC (Analog-to-Digital Converter)
 * Targets soportados: ATmega328P, ATmega1284P
 *
 * ----------------------------------------------------------------
 * QUE ES EL ADC?
 * ----------------------------------------------------------------
 * El ADC convierte una tension analogica (continua) en un numero
 * digital que el microcontrolador puede procesar.
 *
 * En el ATmega328P y 1284P el ADC tiene estas caracteristicas:
 *   - Resolucion: 10 bits  -> valores de 0 a 1023
 *   - Canales:    8 (328P) / 8 (1284P, dos puertos ADC)
 *   - Referencia seleccionable: AVCC, interna 1.1V, o AREF externo
 *
 * Formula de conversion:
 *
 *          Vin * 1023
 *   ADC = -----------
 *           Vref
 *
 * Donde Vin es la tension en el pin y Vref es la tension de
 * referencia. Despejando Vin:
 *
 *          ADC * Vref
 *   Vin = ------------   (en mV si Vref esta en mV)
 *              1023
 *
 * Ejemplo con AVCC = 5000mV:
 *   ADC = 512  ->  Vin = (512 * 5000) / 1023 = 2501 mV ~ 2.5V
 * ----------------------------------------------------------------
 *
 * REGISTROS PRINCIPALES DEL ADC EN AVR:
 *
 *   ADMUX  -> ADC Multiplexer Selection Register
 *             Selecciona el canal de entrada (MUX3:0) y la
 *             referencia de tension (REFS1:0).
 *             ADLAR: alinea el resultado a la izquierda (8 bits).
 *
 *   ADCSRA -> ADC Control and Status Register A
 *             ADEN:  habilita el ADC
 *             ADSC:  inicia una conversion (Start Conversion)
 *             ADATE: habilita auto-trigger
 *             ADIF:  flag de conversion completa
 *             ADIE:  habilita IRQ al completar conversion
 *             ADPS2:0: prescaler del reloj del ADC
 *
 *   ADCL/H -> ADC Data Register Low/High
 *             Resultado de la conversion (10 bits en dos registros).
 *             SIEMPRE leer ADCL primero, luego ADCH.
 *             En avr-gcc se accede como ADC (uint16_t) directamente.
 *
 * PRESCALER DEL ADC:
 *   El ADC necesita un reloj entre 50kHz y 200kHz para maxima
 *   precision. El prescaler divide F_CPU para obtener ese rango.
 *
 *   Para F_CPU = 16MHz:
 *     /128 -> 125kHz  (recomendado para 10 bits de precision)
 *     /64  -> 250kHz  (aceptable, ligera perdida de precision)
 * ----------------------------------------------------------------
 *
 * Created: 13/03/2026
 * Author: USER
 */

#ifndef ADC_H_
#define ADC_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stddef.h>

/* =========================================
   1. DETECCION DE TARGET
   ========================================= */

#if defined(__AVR_ATmega328P__)
    #define HAL_TARGET_328P
    #define HAL_ADC_MAX_CHANNEL   7U   /* ADC0..ADC7 */
#elif defined(__AVR_ATmega1284P__)
    #define HAL_TARGET_1284P
    #define HAL_ADC_MAX_CHANNEL   7U   /* ADC0..ADC7 */
#else
    #error "adc.h: Target no soportado. Usa -mmcu=atmega328p o -mmcu=atmega1284p"
#endif

/* Resolucion del ADC en cuentas */
#define HAL_ADC_RESOLUTION      1023U

/* =========================================
   2. TIPOS
   ========================================= */

/*
 * hal_adc_ref_t — Tension de referencia del ADC
 *
 * La referencia determina el valor maximo que el ADC puede medir.
 * Un voltaje igual a Vref produce el resultado maximo (1023).
 *
 *   AVCC      -> Usa la tension de alimentacion del micro (VCC).
 *                Es la opcion mas comun. Si VCC = 5V, mide 0..5V.
 *
 *   INTERNAL  -> Referencia interna de 1.1V generada dentro del chip.
 *                Util para medir tensiones pequeñas con mas precision.
 *                No requiere componentes externos.
 *
 *   AREF      -> Referencia externa en el pin AREF del micro.
 *                Maxima precision si se usa una referencia de voltaje
 *                dedicada (ej: LM4040, REF3030).
 */
typedef enum {
    HAL_ADC_REF_AREF     = 0x00U,   /* REFS1:0 = 00 — externo en AREF */
    HAL_ADC_REF_AVCC     = 0x01U,   /* REFS1:0 = 01 — VCC con cap en AREF */
    HAL_ADC_REF_INTERNAL = 0x03U    /* REFS1:0 = 11 — 1.1V interno */
} hal_adc_ref_t;

/*
 * hal_adc_prescaler_t — Divisor del reloj para el ADC
 *
 * El ADC requiere un reloj entre 50kHz y 200kHz.
 * Elegir el prescaler correcto segun F_CPU:
 *
 *   F_CPU = 16MHz:  usar DIV128 (125kHz) — maxima precision
 *   F_CPU =  8MHz:  usar DIV64  (125kHz)
 *   F_CPU =  1MHz:  usar DIV8   (125kHz)
 */
typedef enum {
    HAL_ADC_PRESCALER_2   = 0x01U,
    HAL_ADC_PRESCALER_4   = 0x02U,
    HAL_ADC_PRESCALER_8   = 0x03U,
    HAL_ADC_PRESCALER_16  = 0x04U,
    HAL_ADC_PRESCALER_32  = 0x05U,
    HAL_ADC_PRESCALER_64  = 0x06U,
    HAL_ADC_PRESCALER_128 = 0x07U   /* Recomendado para 16MHz */
} hal_adc_prescaler_t;

/*
 * hal_adc_config_t — Configuracion global del ADC
 *
 * Se pasa a HAL_ADC_Init() una sola vez al inicio.
 */
typedef struct {
    hal_adc_ref_t       reference;    /* Tension de referencia         */
    hal_adc_prescaler_t prescaler;    /* Divisor de reloj              */
    uint16_t            vref_mv;      /* Valor de Vref en milivolts    */
                                      /* (para conversion a mV)        */
                                      /* Ej: 5000 para AVCC=5V         */
    uint8_t             use_irq;      /* 0=polling, 1=IRQ              */
} hal_adc_config_t;

/*
 * hal_adc_callback_t — Callback para modo IRQ
 *
 * Se llama al completar cada conversion.
 * Recibe el canal convertido y el resultado en cuentas (0..1023).
 */
typedef void (*hal_adc_callback_t)(uint8_t channel, uint16_t result);

/*
 * hal_adc_scan_entry_t — Entrada para el modo scan
 *
 * El scan mode recorre una lista de canales automaticamente.
 * Cada entrada asocia un canal con su callback opcional.
 */
typedef struct {
    uint8_t             channel;     /* Canal ADC a medir (0..7)      */
    hal_adc_callback_t  callback;    /* Callback al completar, o NULL */
} hal_adc_scan_entry_t;

/* Modos de operacion */
#define HAL_ADC_MODE_POLLING   0U
#define HAL_ADC_MODE_IRQ       1U

/* Numero maximo de canales en el scan */
#ifndef HAL_ADC_SCAN_MAX_CHANNELS
    #define HAL_ADC_SCAN_MAX_CHANNELS   8U
#endif

/* =========================================
   3. MACROS DE BAJO NIVEL
   =========================================
   Utiles para entender el hardware y para casos donde
   se necesita control directo sobre los registros.
   ========================================= */

/** Habilita el ADC (bit ADEN en ADCSRA) */
#define HAL_ADC_ENABLE()        do { ADCSRA |=  (1U << ADEN); } while (0)

/** Deshabilita el ADC — ahorra energia cuando no se usa */
#define HAL_ADC_DISABLE()       do { ADCSRA &= (uint8_t)(~(1U << ADEN)); } while (0)

/** Inicia una conversion (bit ADSC en ADCSRA) */
#define HAL_ADC_START()         do { ADCSRA |=  (1U << ADSC); } while (0)

/**
 * Espera a que la conversion termine (polling sobre ADSC).
 * ADSC se limpia automaticamente cuando la conversion termina.
 */
#define HAL_ADC_WAIT()          do { while ((ADCSRA & (1U << ADSC)) != 0U) {} } while (0)

/** Selecciona el canal en ADMUX (bits MUX3:0) */
#define HAL_ADC_SELECT_CH(ch)                                           \
    do {                                                                \
        ADMUX = (uint8_t)((ADMUX & 0xF0U) | ((ch) & 0x0FU));           \
    } while (0)

/** Lee el resultado de la conversion (registro ADC de 16 bits) */
#define HAL_ADC_READ_RESULT()   ((uint16_t)(ADC))

/* =========================================
   4. DECLARACIONES DE FUNCIONES
   ========================================= */

/* --- Inicializacion --- */

/**
 * HAL_ADC_Init
 * -------------
 * Configura el ADC: referencia, prescaler y modo de operacion.
 * Debe llamarse una sola vez al inicio, antes de cualquier lectura.
 *
 * @param config  Puntero a estructura de configuracion
 * @return        0 si OK, -1 si parametro invalido
 */
int8_t HAL_ADC_Init(const hal_adc_config_t *config);

/**
 * HAL_ADC_Deinit
 * ---------------
 * Deshabilita el ADC. Util para reducir consumo en sleep mode.
 */
void HAL_ADC_Deinit(void);

/* --- Conversion de canal unico --- */

/**
 * HAL_ADC_Read
 * -------------
 * Realiza una conversion en el canal indicado.
 *
 * Modo polling: bloquea hasta que la conversion termine.
 * Modo IRQ:     inicia la conversion y retorna inmediatamente.
 *               El resultado llega por callback.
 *
 * @param channel   Canal ADC: 0..7
 * @param result    Puntero donde guardar el resultado (0..1023).
 *                  Solo se llena en modo polling.
 * @return          0 si OK, -1 si canal invalido
 */
int8_t HAL_ADC_Read(uint8_t channel, uint16_t *result);

/**
 * HAL_ADC_ReadMillivolts
 * -----------------------
 * Convierte la lectura del ADC directamente a milivolts.
 * Usa el vref_mv configurado en HAL_ADC_Init.
 *
 * Solo disponible en modo polling.
 *
 * @param channel   Canal ADC: 0..7
 * @param mv        Puntero donde guardar el resultado en mV
 * @return          0 si OK, -1 si error
 */
int8_t HAL_ADC_ReadMillivolts(uint8_t channel, uint16_t *mv);

/**
 * HAL_ADC_ReadAverage
 * --------------------
 * Realiza N conversiones y retorna el promedio.
 * Util para filtrar ruido en señales analogicas ruidosas.
 *
 * Solo disponible en modo polling.
 *
 * @param channel   Canal ADC: 0..7
 * @param samples   Numero de muestras: 1..64 (recomendado: 8 o 16)
 * @param result    Puntero donde guardar el promedio (0..1023)
 * @return          0 si OK, -1 si error
 */
int8_t HAL_ADC_ReadAverage(uint8_t channel, uint8_t samples, uint16_t *result);

/**
 * HAL_ADC_Map
 * ------------
 * Mapea un valor ADC de su rango original a un rango destino.
 * Equivalente al map() de Arduino pero para valores ADC.
 *
 * Util para convertir cuentas a unidades fisicas:
 *   - Temperatura de un sensor NTC
 *   - Posicion de un potenciometro en grados
 *   - Nivel de un deposito en porcentaje
 *
 * Ejemplo: potenciometro 0..1023 -> angulo 0..180 grados
 *   HAL_ADC_Map(lectura, 0, 1023, 0, 180)
 *
 * @param value     Valor a mapear
 * @param in_min    Minimo del rango de entrada
 * @param in_max    Maximo del rango de entrada
 * @param out_min   Minimo del rango de salida
 * @param out_max   Maximo del rango de salida
 * @return          Valor mapeado al rango de salida
 */
int32_t HAL_ADC_Map(int32_t value,
                    int32_t in_min,  int32_t in_max,
                    int32_t out_min, int32_t out_max);

/* --- Scan mode (multiples canales) --- */

/**
 * HAL_ADC_ScanStart
 * ------------------
 * Inicia el barrido automatico de una lista de canales.
 * En cada canal completado se llama al callback asociado.
 *
 * En modo polling: convierte todos los canales de la lista
 *                  en secuencia y retorna al terminar.
 * En modo IRQ:     convierte de forma no bloqueante.
 *                  Llama al callback de cada canal al terminar.
 *
 * @param entries   Arreglo de canales a barrer
 * @param count     Numero de entradas (max HAL_ADC_SCAN_MAX_CHANNELS)
 * @return          0 si OK, -1 si parametros invalidos
 */
int8_t HAL_ADC_ScanStart(const hal_adc_scan_entry_t *entries, uint8_t count);

/**
 * HAL_ADC_ScanStop
 * -----------------
 * Detiene el scan mode en curso (solo modo IRQ).
 */
void HAL_ADC_ScanStop(void);

/**
 * HAL_ADC_RegisterCallback
 * -------------------------
 * Registra un callback global que se llama al completar
 * cualquier conversion en modo IRQ (fuera del scan mode).
 *
 * @param callback  Funcion void(uint8_t channel, uint16_t result)
 */
void HAL_ADC_RegisterCallback(hal_adc_callback_t callback);

#endif /* ADC_H_ */
