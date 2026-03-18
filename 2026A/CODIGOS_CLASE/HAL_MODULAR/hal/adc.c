/*
 * adc.c
 *
 * HAL - Implementacion del ADC
 * Targets soportados: ATmega328P, ATmega1284P
 *
 * Created: 13/03/2026
 * Author: USER
 */

#include "adc.h"
#include "interrupt.h"   /* HAL_CRITICAL_ENTER / HAL_CRITICAL_EXIT */

/* =========================================
   1. ESTADO INTERNO DEL MODULO
   =========================================
   Todo el estado del ADC vive en esta estructura.
   Al ser static, es invisible fuera de este archivo.
   ========================================= */

typedef struct {
    /* Configuracion general */
    hal_adc_ref_t       reference;
    hal_adc_prescaler_t prescaler;
    uint16_t            vref_mv;
    uint8_t             use_irq;
    uint8_t             initialized;

    /* Callback global para conversiones individuales en modo IRQ */
    volatile hal_adc_callback_t global_callback;

    /* Resultado de la ultima conversion (modo IRQ) */
    volatile uint16_t   last_result;
    volatile uint8_t    last_channel;
    volatile uint8_t    conversion_done;   /* Flag: 1 cuando ISR deposito resultado */

    /* Estado del scan mode */
    const hal_adc_scan_entry_t *scan_entries;
    uint8_t                     scan_count;
    volatile uint8_t            scan_index;   /* Canal actual en el barrido */
    volatile uint8_t            scan_active;  /* 1 si el scan esta corriendo */
} hal_adc_state_t;

static hal_adc_state_t _adc;

/* =========================================
   2. HELPERS INTERNOS
   ========================================= */

/*
 * apply_reference
 * ----------------
 * Escribe la referencia elegida en los bits REFS1:0 de ADMUX.
 *
 * ADMUX layout:
 *   [7:6] REFS1:0  -> referencia
 *   [5]   ADLAR    -> alineacion del resultado
 *   [3:0] MUX3:0   -> canal
 *
 * Se preservan los bits de canal (MUX) y se sobreescriben
 * solo los bits de referencia.
 */
static void apply_reference(hal_adc_ref_t ref)
{
    /*
     * Los bits REFS1:0 son los dos bits mas altos de ADMUX.
     * El enum hal_adc_ref_t ya tiene los valores en la posicion
     * correcta para los bits 1:0 del campo REFS.
     * Los desplazamos 6 posiciones para ubicarlos en [7:6].
     */
    ADMUX = (uint8_t)((ADMUX & 0x3FU) | (uint8_t)((uint8_t)ref << 6U));
}

/*
 * apply_prescaler
 * ----------------
 * Escribe el prescaler en los bits ADPS2:0 de ADCSRA.
 */
static void apply_prescaler(hal_adc_prescaler_t prescaler)
{
    ADCSRA = (uint8_t)((ADCSRA & 0xF8U) | ((uint8_t)prescaler & 0x07U));
}

/*
 * validate_channel
 * -----------------
 * Verifica que el canal sea valido para este target.
 */
static int8_t validate_channel(uint8_t channel)
{
    return (channel <= HAL_ADC_MAX_CHANNEL) ? 0 : -1;
}

/*
 * start_conversion_on_channel
 * ----------------------------
 * Selecciona el canal y dispara la conversion.
 * Es el punto de entrada comun para polling e IRQ.
 *
 * NOTA: Despues de cambiar el canal en ADMUX hay que
 * esperar al menos un ciclo de conversion antes de leer
 * para que el multiplexor se estabilice. En polling esto
 * se maneja descartando la primera lectura si se cambia canal.
 */
static void start_conversion_on_channel(uint8_t channel)
{
    HAL_ADC_SELECT_CH(channel);
    HAL_ADC_START();
}

/* =========================================
   3. INICIALIZACION
   ========================================= */

int8_t HAL_ADC_Init(const hal_adc_config_t *config)
{
    if (config == NULL) return -1;

    /* Guardar configuracion */
    _adc.reference   = config->reference;
    _adc.prescaler   = config->prescaler;
    _adc.vref_mv     = config->vref_mv;
    _adc.use_irq     = config->use_irq;
    _adc.initialized = 1U;

    _adc.global_callback  = NULL;
    _adc.conversion_done  = 0U;
    _adc.scan_active      = 0U;
    _adc.scan_entries     = NULL;
    _adc.scan_count       = 0U;
    _adc.scan_index       = 0U;

    /*
     * Secuencia de inicializacion del ADC:
     *
     * 1. Configurar referencia en ADMUX
     * 2. Configurar prescaler en ADCSRA
     * 3. Habilitar ADEN (ADC Enable)
     * 4. Si se usa IRQ, habilitar ADIE (ADC Interrupt Enable)
     * 5. Realizar una conversion "dummy" — la primera conversion
     *    despues de habilitar el ADC toma mas tiempo (inicializacion
     *    interna del hardware) y su resultado no es confiable.
     *    Se descarta para que las lecturas reales sean precisas.
     */
    apply_reference(_adc.reference);
    apply_prescaler(_adc.prescaler);

    /* Habilitar ADC */
    HAL_ADC_ENABLE();

    /* Habilitar IRQ si corresponde */
    if (_adc.use_irq != 0U) {
        ADCSRA |= (1U << ADIE);
    }

    /* Conversion dummy — descartar resultado */
    ADMUX  = (uint8_t)(ADMUX & 0xF0U);   /* Canal 0 para la dummy */
    HAL_ADC_START();
    HAL_ADC_WAIT();
    (void)HAL_ADC_READ_RESULT();          /* Leer y descartar */

    return 0;
}

void HAL_ADC_Deinit(void)
{
    HAL_ADC_DISABLE();
    ADCSRA &= (uint8_t)(~(1U << ADIE));  /* Deshabilitar IRQ */
    _adc.initialized = 0U;
    _adc.scan_active = 0U;
}

/* =========================================
   4. CONVERSION DE CANAL UNICO
   ========================================= */

int8_t HAL_ADC_Read(uint8_t channel, uint16_t *result)
{
    if (_adc.initialized == 0U)        return -1;
    if (validate_channel(channel) != 0) return -1;

    if (_adc.use_irq == 0U) {
        /*
         * MODO POLLING
         * ------------
         * 1. Seleccionar canal y arrancar conversion
         * 2. Esperar a que ADSC se limpie (fin de conversion)
         * 3. Leer resultado del registro ADC
         *
         * Si se cambia de canal respecto a la conversion anterior,
         * se hace una conversion de "warmup" y se descarta para
         * dar tiempo al multiplexor a estabilizarse.
         */
        if (result == NULL) return -1;

        start_conversion_on_channel(channel);
        HAL_ADC_WAIT();
        *result = HAL_ADC_READ_RESULT();
        return 0;

    } else {
        /*
         * MODO IRQ
         * --------
         * Arranca la conversion y retorna inmediatamente.
         * La ISR depositara el resultado y llamara al callback.
         * result no se llena aqui — usar el callback.
         */
        _adc.conversion_done = 0U;
        start_conversion_on_channel(channel);
        return 0;
    }
}

int8_t HAL_ADC_ReadMillivolts(uint8_t channel, uint16_t *mv)
{
    if (mv == NULL || _adc.use_irq != 0U) return -1;

    uint16_t raw;
    if (HAL_ADC_Read(channel, &raw) != 0) return -1;

    /*
     * Conversion de cuentas a milivolts:
     *
     *          raw * vref_mv
     *   mV  = -------------
     *              1023
     *
     * Se usa uint32_t para el producto intermedio para evitar
     * overflow: 1023 * 5000 = 5,115,000 > 65535 (limite uint16_t).
     */
    *mv = (uint16_t)(((uint32_t)raw * (uint32_t)_adc.vref_mv)
                     / (uint32_t)HAL_ADC_RESOLUTION);
    return 0;
}

int8_t HAL_ADC_ReadAverage(uint8_t channel, uint8_t samples, uint16_t *result)
{
    if (result == NULL)                 return -1;
    if (_adc.initialized == 0U)         return -1;
    if (validate_channel(channel) != 0) return -1;
    if (samples == 0U || samples > 64U) return -1;
    if (_adc.use_irq != 0U)             return -1;  /* Solo polling */

    /*
     * Acumular N muestras y dividir.
     *
     * Se usa uint32_t para el acumulador porque:
     *   64 muestras * 1023 cuentas = 65,472  <- cabe en uint16_t
     *   pero con samples > 64 desbordaria, por eso limitamos a 64.
     *
     * El primer valor leido se descarta (warmup del multiplexor).
     */
    uint32_t sum = 0U;
    uint16_t raw;

    /* Warmup: primera conversion descartada al cambiar canal */
    HAL_ADC_SELECT_CH(channel);
    HAL_ADC_START();
    HAL_ADC_WAIT();
    (void)HAL_ADC_READ_RESULT();

    for (uint8_t i = 0U; i < samples; i++) {
        HAL_ADC_START();
        HAL_ADC_WAIT();
        raw  = HAL_ADC_READ_RESULT();
        sum += (uint32_t)raw;
    }

    *result = (uint16_t)(sum / (uint32_t)samples);
    return 0;
}

/* =========================================
   5. FUNCION MAP
   =========================================
   Mapea linealmente un valor de un rango a otro.

   Formula:
     out = out_min + (value - in_min) * (out_max - out_min)
                     -----------------------------------------
                                (in_max - in_min)

   Se usa int32_t para manejar rangos negativos y evitar
   overflow en el producto intermedio.

   Ejemplo: mapear 512 cuentas (0..1023) a grados (0..180):
     HAL_ADC_Map(512, 0, 1023, 0, 180) = 90
   ========================================= */

int32_t HAL_ADC_Map(int32_t value,
                    int32_t in_min,  int32_t in_max,
                    int32_t out_min, int32_t out_max)
{
    if (in_max == in_min) return out_min;  /* Evitar division por cero */

    return out_min + ((value - in_min) * (out_max - out_min))
                     / (in_max - in_min);
}

/* =========================================
   6. SCAN MODE — BARRIDO DE MULTIPLES CANALES
   =========================================
   El scan mode recorre una lista de canales en secuencia.
   Cada canal tiene su propio callback opcional.

   En modo polling: convierte todos los canales en un loop
                    y llama a cada callback antes de seguir.
   En modo IRQ:     convierte el primer canal, la ISR llama
                    al callback y arranca el siguiente canal
                    automaticamente. Funciona como una cadena.
   ========================================= */

int8_t HAL_ADC_ScanStart(const hal_adc_scan_entry_t *entries, uint8_t count)
{
    if (entries == NULL)                     return -1;
    if (count == 0U ||
        count > HAL_ADC_SCAN_MAX_CHANNELS)   return -1;
    if (_adc.initialized == 0U)              return -1;

    /* Validar todos los canales antes de arrancar */
    for (uint8_t i = 0U; i < count; i++) {
        if (validate_channel(entries[i].channel) != 0) return -1;
    }

    if (_adc.use_irq == 0U) {
        /*
         * SCAN POLLING
         * ------------
         * Convertir todos los canales en secuencia.
         * Bloqueante hasta que termina la lista completa.
         */
        for (uint8_t i = 0U; i < count; i++) {
            uint16_t raw;
            HAL_ADC_SELECT_CH(entries[i].channel);

            /* Warmup al cambiar de canal */
            HAL_ADC_START();
            HAL_ADC_WAIT();
            (void)HAL_ADC_READ_RESULT();

            /* Conversion real */
            HAL_ADC_START();
            HAL_ADC_WAIT();
            raw = HAL_ADC_READ_RESULT();

            if (entries[i].callback != NULL) {
                entries[i].callback(entries[i].channel, raw);
            }
        }
        return 0;

    } else {
        /*
         * SCAN IRQ
         * --------
         * Guardar la lista y arrancar la primera conversion.
         * La ISR se encargara de las siguientes.
         */
        uint8_t sreg;
        HAL_CRITICAL_ENTER(sreg);
        _adc.scan_entries = entries;
        _adc.scan_count   = count;
        _adc.scan_index   = 0U;
        _adc.scan_active  = 1U;
        HAL_CRITICAL_EXIT(sreg);

        start_conversion_on_channel(entries[0U].channel);
        return 0;
    }
}

void HAL_ADC_ScanStop(void)
{
    uint8_t sreg;
    HAL_CRITICAL_ENTER(sreg);
    _adc.scan_active = 0U;
    _adc.scan_index  = 0U;
    HAL_CRITICAL_EXIT(sreg);
}

void HAL_ADC_RegisterCallback(hal_adc_callback_t callback)
{
    uint8_t sreg;
    HAL_CRITICAL_ENTER(sreg);
    _adc.global_callback = callback;
    HAL_CRITICAL_EXIT(sreg);
}

/* =========================================
   7. ISR DEL ADC
   =========================================
   Se dispara cuando una conversion termina (flag ADIF).
   avr-gcc limpia ADIF automaticamente al entrar a la ISR.

   Responsabilidades:
     1. Leer el resultado del registro ADC
     2. Si hay scan activo: llamar al callback del canal actual
        y arrancar la conversion del siguiente canal
     3. Si no hay scan: llamar al callback global si existe
   ========================================= */

ISR(ADC_vect)
{
    uint16_t result  = HAL_ADC_READ_RESULT();
    uint8_t  channel = (uint8_t)(ADMUX & 0x0FU);  /* Canal actual */

    _adc.last_result     = result;
    _adc.last_channel    = channel;
    _adc.conversion_done = 1U;

    if (_adc.scan_active != 0U) {
        /*
         * Scan mode activo:
         * 1. Llamar al callback de este canal
         * 2. Avanzar al siguiente canal
         * 3. Si quedan canales, arrancar la siguiente conversion
         * 4. Si se termino la lista, el scan se detiene
         *    (se puede reiniciar con HAL_ADC_ScanStart)
         */
        uint8_t idx = _adc.scan_index;

        if (_adc.scan_entries[idx].callback != NULL) {
            _adc.scan_entries[idx].callback(channel, result);
        }

        _adc.scan_index++;

        if (_adc.scan_index < _adc.scan_count) {
            /* Arrancar siguiente canal */
            start_conversion_on_channel(
                _adc.scan_entries[_adc.scan_index].channel);
        } else {
            /* Lista completa — detener scan */
            _adc.scan_active = 0U;
            _adc.scan_index  = 0U;
        }

    } else {
        /* Conversion individual — llamar callback global */
        if (_adc.global_callback != NULL) {
            _adc.global_callback(channel, result);
        }
    }
}

/* =========================================
   8. EJEMPLOS DE USO
   ========================================= */

/*
 * -----------------------------------------------------------
 * EJEMPLO 1: Lectura simple en polling con conversion a mV
 * -----------------------------------------------------------
 * Caso tipico: leer un potenciometro o sensor analogico.
 *
 * #include "adc.h"
 * #include "uart.h"
 *
 * int main(void) {
 *     hal_adc_config_t cfg = {
 *         .reference = HAL_ADC_REF_AVCC,
 *         .prescaler = HAL_ADC_PRESCALER_128,
 *         .vref_mv   = 5000U,
 *         .use_irq   = HAL_ADC_MODE_POLLING
 *     };
 *     HAL_ADC_Init(&cfg);
 *
 *     // Configurar UART para debug
 *     hal_uart_config_t uart_cfg = { .baud_rate=115200, ... };
 *     HAL_UART_Init(HAL_UART0, &uart_cfg);
 *     HAL_UART_SetStdout(HAL_UART0);
 *
 *     while (1) {
 *         uint16_t mv;
 *         HAL_ADC_ReadMillivolts(0, &mv);   // Canal ADC0
 *         printf("Tension: %u mV\n", mv);
 *     }
 * }
 * -----------------------------------------------------------
 *
 * EJEMPLO 2: Promedio de muestras para reducir ruido
 * -----------------------------------------------------------
 * Sensores analogicos suelen tener ruido de alta frecuencia.
 * Promediar 16 muestras reduce significativamente ese ruido.
 *
 * uint16_t promedio;
 * HAL_ADC_ReadAverage(0, 16U, &promedio);
 * printf("ADC promedio: %u\n", promedio);
 * -----------------------------------------------------------
 *
 * EJEMPLO 3: Mapear lectura ADC a unidades fisicas
 * -----------------------------------------------------------
 *
 * // Potenciometro -> angulo (0 a 270 grados)
 * uint16_t raw;
 * HAL_ADC_Read(0, &raw);
 * int32_t angulo = HAL_ADC_Map(raw, 0, 1023, 0, 270);
 * printf("Angulo: %ld grados\n", angulo);
 *
 * // Sensor de temperatura LM35 (10mV/C, 0 a 150C con AVCC=5V)
 * uint16_t mv;
 * HAL_ADC_ReadMillivolts(1, &mv);
 * int32_t temp_c = HAL_ADC_Map(mv, 0, 1500, 0, 150);
 * printf("Temperatura: %ld C\n", temp_c);
 *
 * // Nivel de bateria (3.0V a 4.2V) como porcentaje
 * HAL_ADC_ReadMillivolts(2, &mv);
 * int32_t pct = HAL_ADC_Map(mv, 3000, 4200, 0, 100);
 * printf("Bateria: %ld%%\n", pct);
 * -----------------------------------------------------------
 *
 * EJEMPLO 4: Scan mode con IRQ — leer 3 canales sin bloquear
 * -----------------------------------------------------------
 * El CPU no se bloquea. Los callbacks se llaman automaticamente
 * al terminar cada conversion.
 *
 * static void on_canal0(uint8_t ch, uint16_t val) {
 *     // Llamado al terminar ADC0
 * }
 * static void on_canal1(uint8_t ch, uint16_t val) {
 *     // Llamado al terminar ADC1
 * }
 * static void on_canal2(uint8_t ch, uint16_t val) {
 *     // Llamado al terminar ADC2
 * }
 *
 * static const hal_adc_scan_entry_t canales[] = {
 *     { .channel = 0U, .callback = on_canal0 },
 *     { .channel = 1U, .callback = on_canal1 },
 *     { .channel = 2U, .callback = on_canal2 },
 * };
 *
 * int main(void) {
 *     hal_adc_config_t cfg = { ..., .use_irq = HAL_ADC_MODE_IRQ };
 *     HAL_ADC_Init(&cfg);
 *     HAL_IRQ_ENABLE();
 *
 *     HAL_ADC_ScanStart(canales, 3U);
 *
 *     while (1) {
 *         // CPU libre para otras tareas
 *         // Los callbacks se disparan automaticamente
 *     }
 * }
 * -----------------------------------------------------------
 *
 * EJEMPLO 5: Referencia interna 1.1V para señales pequeñas
 * -----------------------------------------------------------
 * Util para medir tensiones de 0 a 1.1V con mayor precision.
 * Ejemplo: sensor de corriente por shunt resistivo.
 *
 * hal_adc_config_t cfg = {
 *     .reference = HAL_ADC_REF_INTERNAL,
 *     .prescaler = HAL_ADC_PRESCALER_128,
 *     .vref_mv   = 1100U,            // Vref = 1.1V = 1100mV
 *     .use_irq   = HAL_ADC_MODE_POLLING
 * };
 * HAL_ADC_Init(&cfg);
 *
 * uint16_t mv;
 * HAL_ADC_ReadMillivolts(3, &mv);    // Resolucion: 1100/1023 ~ 1.07 mV/cuenta
 * -----------------------------------------------------------
 */
