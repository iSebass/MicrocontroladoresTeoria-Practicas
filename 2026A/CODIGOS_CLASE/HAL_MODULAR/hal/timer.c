/*
 * timer.c
 *
 * HAL - Implementacion de la capa de Timers
 * Targets soportados: ATmega328P, ATmega1284P
 *
 * Created: 13/03/2026
 * Author: USER
 */

#include "timer.h"
#include "interrupt.h"   /* HAL_CRITICAL_ENTER / HAL_CRITICAL_EXIT */

/* =========================================
   1. ESTADO INTERNO
   =========================================
   Cada timer guarda su propio estado: modo actual, prescaler
   aplicado, valor TOP y los tres callbacks posibles.
   ========================================= */

typedef struct {
    hal_timer_mode_t      mode;
    hal_timer_prescaler_t prescaler;
    uint16_t              top;
    uint8_t               initialized;

    volatile hal_timer_callback_t cb_overflow;
    volatile hal_timer_callback_t cb_compare_a;
    volatile hal_timer_callback_t cb_compare_b;
} hal_timer_state_t;

static hal_timer_state_t _timer_state[HAL_TIMER_COUNT];

/* =========================================
   2. TABLAS DE BITS DE PRESCALER
   =========================================
   Timer0, Timer1 y Timer3 usan la misma tabla de CS bits.
   Timer2 tiene una tabla distinta (incluye /32 y /128).

   Los valores corresponden a los bits CSn2:CSn0 del registro TCCRnB.
   ========================================= */

/*
 * Tabla para Timer0, Timer1, Timer3
 * CS = 0 significa timer detenido (lo manejamos por separado).
 */
static const uint8_t _prescaler_bits_std[8] = {
    0U,   /* indice 0 — no usado                     */
    1U,   /* HAL_TIMER_PRESCALER_1    -> CS=001 (/1)  */
    2U,   /* HAL_TIMER_PRESCALER_8    -> CS=010 (/8)  */
    0U,   /* HAL_TIMER_PRESCALER_32   -> no existe     */
    3U,   /* HAL_TIMER_PRESCALER_64   -> CS=011 (/64) */
    0U,   /* HAL_TIMER_PRESCALER_128  -> no existe     */
    4U,   /* HAL_TIMER_PRESCALER_256  -> CS=100 (/256)*/
    5U    /* HAL_TIMER_PRESCALER_1024 -> CS=101 (/1024)*/
};

/*
 * Tabla para Timer2 (prescalers especiales)
 *   /32  -> CS=011
 *   /128 -> CS=101
 */
static const uint8_t _prescaler_bits_t2[8] = {
    0U,   /* indice 0 — no usado                      */
    1U,   /* HAL_TIMER_PRESCALER_1    -> CS=001 (/1)   */
    2U,   /* HAL_TIMER_PRESCALER_8    -> CS=010 (/8)   */
    3U,   /* HAL_TIMER_PRESCALER_32   -> CS=011 (/32)  */
    4U,   /* HAL_TIMER_PRESCALER_64   -> CS=100 (/64)  */
    5U,   /* HAL_TIMER_PRESCALER_128  -> CS=101 (/128) */
    6U,   /* HAL_TIMER_PRESCALER_256  -> CS=110 (/256) */
    7U    /* HAL_TIMER_PRESCALER_1024 -> CS=111 (/1024)*/
};

/* Devuelve los bits CS correctos segun timer y prescaler */
static uint8_t get_cs_bits(hal_timer_id_t id, hal_timer_prescaler_t prescaler)
{
    uint8_t idx = (uint8_t)prescaler;
    if (idx >= 8U) return 0U;

    if (id == HAL_TIMER2) {
        return _prescaler_bits_t2[idx];
    }
    return _prescaler_bits_std[idx];
}

/* =========================================
   3. INICIALIZACION
   =========================================
   Cada timer tiene registros distintos.
   Configuramos TCCRnA y TCCRnB segun el modo elegido.

   BITS DE MODO (WGM — Waveform Generation Mode):
     Los bits WGM se reparten entre TCCRnA[1:0] y TCCRnB[4:3].

   BITS DE COMPARE OUTPUT (COM — Compare Output Mode):
     COMnA1:0 y COMnB1:0 en TCCRnA.
     Controlan que hace el pin OC cuando TCNTn == OCRn.
     00 = desconectado | 10 = no-inversor | 11 = inversor
     Los configuramos al habilitar PWM, no aqui.
   ========================================= */

static int8_t init_timer0(const hal_timer_config_t *cfg)
{
    /*
     * Timer0 — 8 bits
     * TCCRnA: [COM0A1:0][COM0B1:0][--][--][WGM01][WGM00]
     * TCCRnB: [FOC0A][FOC0B][--][--][WGM02][CS02][CS01][CS00]
     *
     * WGM bits para cada modo:
     *   NORMAL:    WGM02:0 = 000
     *   CTC:       WGM02:0 = 010  (TOP = OCR0A)
     *   FAST PWM:  WGM02:0 = 011  (TOP = 0xFF) o 111 (TOP = OCR0A)
     *   PHASE PWM: WGM02:0 = 001  (TOP = 0xFF) o 101 (TOP = OCR0A)
     */
    uint8_t tccra = 0U;
    uint8_t tccrb = 0U;

    switch (cfg->mode) {
        case HAL_TIMER_MODE_NORMAL:
            /* WGM = 000 — todo en 0, nada que setear */
            break;

        case HAL_TIMER_MODE_CTC:
            /* WGM = 010: WGM01=1 en TCCRnA */
            tccra |= (1U << WGM01);
            OCR0A  = (uint8_t)(cfg->top & 0xFFU);
            break;

        case HAL_TIMER_MODE_FAST_PWM:
            /* WGM = 011: WGM01=1, WGM00=1 en TCCRnA */
            tccra |= (1U << WGM01) | (1U << WGM00);
            break;

        case HAL_TIMER_MODE_PHASE_PWM:
            /* WGM = 001: WGM00=1 en TCCRnA */
            tccra |= (1U << WGM00);
            break;

        default:
            return -1;
    }

    TCCR0A = tccra;
    TCCR0B = tccrb;   /* Prescaler = 0 (timer detenido hasta HAL_Timer_Start) */
    TCNT0  = 0U;
    return 0;
}

static int8_t init_timer1(const hal_timer_config_t *cfg)
{
    /*
     * Timer1 — 16 bits
     * TCCRnA: [COM1A1:0][COM1B1:0][--][--][WGM11][WGM10]
     * TCCRnB: [ICNC1][ICES1][--][WGM13][WGM12][CS12][CS11][CS10]
     *
     * WGM para cada modo (16 bits tiene mas opciones):
     *   NORMAL:    WGM13:0 = 0000
     *   CTC:       WGM13:0 = 0100  (TOP = OCR1A)
     *   FAST PWM:  WGM13:0 = 1110  (TOP = ICR1)  <- mas flexible
     *   PHASE PWM: WGM13:0 = 1010  (TOP = ICR1)
     */
    uint8_t tccra = 0U;
    uint8_t tccrb = 0U;

    switch (cfg->mode) {
        case HAL_TIMER_MODE_NORMAL:
            break;

        case HAL_TIMER_MODE_CTC:
            /* WGM12=1 en TCCRnB */
            tccrb  |= (1U << WGM12);
            OCR1A   = cfg->top;
            break;

        case HAL_TIMER_MODE_FAST_PWM:
            /*
             * WGM13=1, WGM12=1 en TCCRnB; WGM11=1 en TCCRnA
             * TOP = ICR1 — permite frecuencia de PWM arbitraria
             */
            tccra |= (1U << WGM11);
            tccrb |= (1U << WGM13) | (1U << WGM12);
            ICR1   = cfg->top;
            break;

        case HAL_TIMER_MODE_PHASE_PWM:
            /* WGM13=1 en TCCRnB; WGM11=1 en TCCRnA */
            tccra |= (1U << WGM11);
            tccrb |= (1U << WGM13);
            ICR1   = cfg->top;
            break;

        default:
            return -1;
    }

    TCCR1A = tccra;
    TCCR1B = tccrb;
    TCNT1  = 0U;
    return 0;
}

static int8_t init_timer2(const hal_timer_config_t *cfg)
{
    /*
     * Timer2 — 8 bits, identico a Timer0 en estructura de registros.
     * La diferencia principal es el prescaler (ver tabla _prescaler_bits_t2)
     * y que puede usar un cristal externo de 32.768kHz (modo asincrono).
     */
    uint8_t tccra = 0U;
    uint8_t tccrb = 0U;

    switch (cfg->mode) {
        case HAL_TIMER_MODE_NORMAL:
            break;

        case HAL_TIMER_MODE_CTC:
            tccra |= (1U << WGM21);
            OCR2A  = (uint8_t)(cfg->top & 0xFFU);
            break;

        case HAL_TIMER_MODE_FAST_PWM:
            tccra |= (1U << WGM21) | (1U << WGM20);
            break;

        case HAL_TIMER_MODE_PHASE_PWM:
            tccra |= (1U << WGM20);
            break;

        default:
            return -1;
    }

    TCCR2A = tccra;
    TCCR2B = tccrb;
    TCNT2  = 0U;
    return 0;
}

#if defined(HAL_TARGET_1284P)
static int8_t init_timer3(const hal_timer_config_t *cfg)
{
    /*
     * Timer3 — 16 bits, identico en estructura a Timer1.
     * Solo disponible en ATmega1284P.
     */
    uint8_t tccra = 0U;
    uint8_t tccrb = 0U;

    switch (cfg->mode) {
        case HAL_TIMER_MODE_NORMAL:
            break;
        case HAL_TIMER_MODE_CTC:
            tccrb |= (1U << WGM32);
            OCR3A  = cfg->top;
            break;
        case HAL_TIMER_MODE_FAST_PWM:
            tccra |= (1U << WGM31);
            tccrb |= (1U << WGM33) | (1U << WGM32);
            ICR3   = cfg->top;
            break;
        case HAL_TIMER_MODE_PHASE_PWM:
            tccra |= (1U << WGM31);
            tccrb |= (1U << WGM33);
            ICR3   = cfg->top;
            break;
        default:
            return -1;
    }

    TCCR3A = tccra;
    TCCR3B = tccrb;
    TCNT3  = 0U;
    return 0;
}
#endif

int8_t HAL_Timer_Init(hal_timer_id_t id, const hal_timer_config_t *config)
{
    if (id >= HAL_TIMER_COUNT || config == NULL) return -1;

    hal_timer_state_t *st = &_timer_state[id];
    st->mode        = config->mode;
    st->prescaler   = config->prescaler;
    st->top         = config->top;
    st->initialized = 1U;

    st->cb_overflow  = NULL;
    st->cb_compare_a = NULL;
    st->cb_compare_b = NULL;

    switch (id) {
        case HAL_TIMER0: return init_timer0(config);
        case HAL_TIMER1: return init_timer1(config);
        case HAL_TIMER2: return init_timer2(config);
#if defined(HAL_TARGET_1284P)
        case HAL_TIMER3: return init_timer3(config);
#endif
        default: return -1;
    }
}

/* =========================================
   4. START / STOP / RESET
   =========================================
   Arrancar el timer = escribir los bits CS en TCCRnB.
   Detenerlo = poner CS en 000.
   ========================================= */

int8_t HAL_Timer_Start(hal_timer_id_t id)
{
    if (id >= HAL_TIMER_COUNT) return -1;
    if (_timer_state[id].initialized == 0U) return -1;

    uint8_t cs = get_cs_bits(id, _timer_state[id].prescaler);
    if (cs == 0U) return -1;   /* Prescaler invalido para este timer */

    switch (id) {
        case HAL_TIMER0: TCCR0B = (uint8_t)((TCCR0B & 0xF8U) | cs); break;
        case HAL_TIMER1: TCCR1B = (uint8_t)((TCCR1B & 0xF8U) | cs); break;
        case HAL_TIMER2: TCCR2B = (uint8_t)((TCCR2B & 0xF8U) | cs); break;
#if defined(HAL_TARGET_1284P)
        case HAL_TIMER3: TCCR3B = (uint8_t)((TCCR3B & 0xF8U) | cs); break;
#endif
        default: return -1;
    }
    return 0;
}

void HAL_Timer_Stop(hal_timer_id_t id)
{
    if (id >= HAL_TIMER_COUNT) return;
    /* CS = 000 detiene el timer */
    switch (id) {
        case HAL_TIMER0: TCCR0B &= (uint8_t)(~0x07U); break;
        case HAL_TIMER1: TCCR1B &= (uint8_t)(~0x07U); break;
        case HAL_TIMER2: TCCR2B &= (uint8_t)(~0x07U); break;
#if defined(HAL_TARGET_1284P)
        case HAL_TIMER3: TCCR3B &= (uint8_t)(~0x07U); break;
#endif
        default: break;
    }
}

void HAL_Timer_Reset(hal_timer_id_t id)
{
    switch (id) {
        case HAL_TIMER0: TCNT0 = 0U; break;
        case HAL_TIMER1: TCNT1 = 0U; break;
        case HAL_TIMER2: TCNT2 = 0U; break;
#if defined(HAL_TARGET_1284P)
        case HAL_TIMER3: TCNT3 = 0U; break;
#endif
        default: break;
    }
}

/* =========================================
   5. INTERRUPCIONES Y CALLBACKS
   ========================================= */

int8_t HAL_Timer_EnableIRQ(hal_timer_id_t id, hal_timer_irq_src_t src)
{
    if (id >= HAL_TIMER_COUNT) return -1;

    switch (id) {
        case HAL_TIMER0:
            if (src == HAL_TIMER_IRQ_OVERFLOW)  TIMSK0 |= (1U << TOIE0);
            if (src == HAL_TIMER_IRQ_COMPARE_A) TIMSK0 |= (1U << OCIE0A);
            if (src == HAL_TIMER_IRQ_COMPARE_B) TIMSK0 |= (1U << OCIE0B);
            break;
        case HAL_TIMER1:
            if (src == HAL_TIMER_IRQ_OVERFLOW)  TIMSK1 |= (1U << TOIE1);
            if (src == HAL_TIMER_IRQ_COMPARE_A) TIMSK1 |= (1U << OCIE1A);
            if (src == HAL_TIMER_IRQ_COMPARE_B) TIMSK1 |= (1U << OCIE1B);
            break;
        case HAL_TIMER2:
            if (src == HAL_TIMER_IRQ_OVERFLOW)  TIMSK2 |= (1U << TOIE2);
            if (src == HAL_TIMER_IRQ_COMPARE_A) TIMSK2 |= (1U << OCIE2A);
            if (src == HAL_TIMER_IRQ_COMPARE_B) TIMSK2 |= (1U << OCIE2B);
            break;
#if defined(HAL_TARGET_1284P)
        case HAL_TIMER3:
            if (src == HAL_TIMER_IRQ_OVERFLOW)  TIMSK3 |= (1U << TOIE3);
            if (src == HAL_TIMER_IRQ_COMPARE_A) TIMSK3 |= (1U << OCIE3A);
            if (src == HAL_TIMER_IRQ_COMPARE_B) TIMSK3 |= (1U << OCIE3B);
            break;
#endif
        default: return -1;
    }
    return 0;
}

void HAL_Timer_DisableIRQ(hal_timer_id_t id, hal_timer_irq_src_t src)
{
    switch (id) {
        case HAL_TIMER0:
            if (src == HAL_TIMER_IRQ_OVERFLOW)  TIMSK0 &= (uint8_t)(~(1U << TOIE0));
            if (src == HAL_TIMER_IRQ_COMPARE_A) TIMSK0 &= (uint8_t)(~(1U << OCIE0A));
            if (src == HAL_TIMER_IRQ_COMPARE_B) TIMSK0 &= (uint8_t)(~(1U << OCIE0B));
            break;
        case HAL_TIMER1:
            if (src == HAL_TIMER_IRQ_OVERFLOW)  TIMSK1 &= (uint8_t)(~(1U << TOIE1));
            if (src == HAL_TIMER_IRQ_COMPARE_A) TIMSK1 &= (uint8_t)(~(1U << OCIE1A));
            if (src == HAL_TIMER_IRQ_COMPARE_B) TIMSK1 &= (uint8_t)(~(1U << OCIE1B));
            break;
        case HAL_TIMER2:
            if (src == HAL_TIMER_IRQ_OVERFLOW)  TIMSK2 &= (uint8_t)(~(1U << TOIE2));
            if (src == HAL_TIMER_IRQ_COMPARE_A) TIMSK2 &= (uint8_t)(~(1U << OCIE2A));
            if (src == HAL_TIMER_IRQ_COMPARE_B) TIMSK2 &= (uint8_t)(~(1U << OCIE2B));
            break;
#if defined(HAL_TARGET_1284P)
        case HAL_TIMER3:
            if (src == HAL_TIMER_IRQ_OVERFLOW)  TIMSK3 &= (uint8_t)(~(1U << TOIE3));
            if (src == HAL_TIMER_IRQ_COMPARE_A) TIMSK3 &= (uint8_t)(~(1U << OCIE3A));
            if (src == HAL_TIMER_IRQ_COMPARE_B) TIMSK3 &= (uint8_t)(~(1U << OCIE3B));
            break;
#endif
        default: break;
    }
}

int8_t HAL_Timer_RegisterCallback(hal_timer_id_t       id,
                                   hal_timer_irq_src_t  src,
                                   hal_timer_callback_t callback)
{
    if (id >= HAL_TIMER_COUNT) return -1;

    hal_timer_state_t *st = &_timer_state[id];
    uint8_t sreg;
    HAL_CRITICAL_ENTER(sreg);

    switch (src) {
        case HAL_TIMER_IRQ_OVERFLOW:  st->cb_overflow  = callback; break;
        case HAL_TIMER_IRQ_COMPARE_A: st->cb_compare_a = callback; break;
        case HAL_TIMER_IRQ_COMPARE_B: st->cb_compare_b = callback; break;
        default:
            HAL_CRITICAL_EXIT(sreg);
            return -1;
    }

    HAL_CRITICAL_EXIT(sreg);
    return 0;
}

/* =========================================
   6. PWM
   =========================================
   Habilitar PWM = conectar el pin OC al timer.
   Esto se hace poniendo COMnX1:0 en 10 (modo no-inversor).

   No-inversor:
     pin = HIGH cuando TCNT < OCR
     pin = LOW  cuando TCNT >= OCR
     duty cycle = OCR / TOP * 100%

   Para usar el pin como PWM tambien debe estar configurado
   como OUTPUT en el registro DDR (usar gpio.h).
   ========================================= */

int8_t HAL_Timer_PWM_Enable(hal_timer_id_t id, hal_timer_channel_t channel)
{
    if (id >= HAL_TIMER_COUNT) return -1;

    /*
     * COMnA1 o COMnB1 = 1, COMnA0 o COMnB0 = 0
     * -> modo no-inversor, pin OC conectado al timer
     */
    switch (id) {
        case HAL_TIMER0:
            if (channel == HAL_TIMER_CH_A) TCCR0A |= (1U << COM0A1);
            else                            TCCR0A |= (1U << COM0B1);
            break;
        case HAL_TIMER1:
            if (channel == HAL_TIMER_CH_A) TCCR1A |= (1U << COM1A1);
            else                            TCCR1A |= (1U << COM1B1);
            break;
        case HAL_TIMER2:
            if (channel == HAL_TIMER_CH_A) TCCR2A |= (1U << COM2A1);
            else                            TCCR2A |= (1U << COM2B1);
            break;
#if defined(HAL_TARGET_1284P)
        case HAL_TIMER3:
            if (channel == HAL_TIMER_CH_A) TCCR3A |= (1U << COM3A1);
            else                            TCCR3A |= (1U << COM3B1);
            break;
#endif
        default: return -1;
    }
    return 0;
}

void HAL_Timer_PWM_Disable(hal_timer_id_t id, hal_timer_channel_t channel)
{
    /* COM = 00: desconectar pin OC del timer */
    switch (id) {
        case HAL_TIMER0:
            if (channel == HAL_TIMER_CH_A)
                TCCR0A &= (uint8_t)(~((1U << COM0A1) | (1U << COM0A0)));
            else
                TCCR0A &= (uint8_t)(~((1U << COM0B1) | (1U << COM0B0)));
            break;
        case HAL_TIMER1:
            if (channel == HAL_TIMER_CH_A)
                TCCR1A &= (uint8_t)(~((1U << COM1A1) | (1U << COM1A0)));
            else
                TCCR1A &= (uint8_t)(~((1U << COM1B1) | (1U << COM1B0)));
            break;
        case HAL_TIMER2:
            if (channel == HAL_TIMER_CH_A)
                TCCR2A &= (uint8_t)(~((1U << COM2A1) | (1U << COM2A0)));
            else
                TCCR2A &= (uint8_t)(~((1U << COM2B1) | (1U << COM2B0)));
            break;
#if defined(HAL_TARGET_1284P)
        case HAL_TIMER3:
            if (channel == HAL_TIMER_CH_A)
                TCCR3A &= (uint8_t)(~((1U << COM3A1) | (1U << COM3A0)));
            else
                TCCR3A &= (uint8_t)(~((1U << COM3B1) | (1U << COM3B0)));
            break;
#endif
        default: break;
    }
}

int8_t HAL_Timer_PWM_SetDuty(hal_timer_id_t      id,
                               hal_timer_channel_t channel,
                               uint8_t             duty)
{
    if (id >= HAL_TIMER_COUNT) return -1;
    if (duty > 100U) return -1;

    uint16_t top = _timer_state[id].top;
    /* Para timers de 8 bits en Fast/Phase PWM, TOP = 255 */
    if (id == HAL_TIMER0 || id == HAL_TIMER2) {
        top = 255U;
    }

    uint16_t ocr = HAL_TIMER_DUTY(top, duty);
    return HAL_Timer_PWM_SetRaw(id, channel, ocr);
}

int8_t HAL_Timer_PWM_SetRaw(hal_timer_id_t      id,
                              hal_timer_channel_t channel,
                              uint16_t            value)
{
    if (id >= HAL_TIMER_COUNT) return -1;

    switch (id) {
        case HAL_TIMER0:
            if (channel == HAL_TIMER_CH_A) OCR0A = (uint8_t)(value & 0xFFU);
            else                            OCR0B = (uint8_t)(value & 0xFFU);
            break;
        case HAL_TIMER1:
            if (channel == HAL_TIMER_CH_A) OCR1A = value;
            else                            OCR1B = value;
            break;
        case HAL_TIMER2:
            if (channel == HAL_TIMER_CH_A) OCR2A = (uint8_t)(value & 0xFFU);
            else                            OCR2B = (uint8_t)(value & 0xFFU);
            break;
#if defined(HAL_TARGET_1284P)
        case HAL_TIMER3:
            if (channel == HAL_TIMER_CH_A) OCR3A = value;
            else                            OCR3B = value;
            break;
#endif
        default: return -1;
    }
    return 0;
}

/* =========================================
   7. SISTEMA DE TIMESTAMP
   =========================================
   Usa Timer1 en modo CTC para generar una IRQ cada 1ms.
   La ISR incrementa un contador de 32 bits (_millis_count).

   Para 1ms con F_CPU=16MHz y prescaler=64:
     TOP = (16000000 / (2 * 64 * 1000)) - 1 = 124
   ========================================= */

static volatile uint32_t _millis_count = 0U;
static volatile uint32_t _micros_count = 0U;

/* Ticks de Timer1 por microsegundo con prescaler 64 */
#define _T1_PRESCALER_VAL   64UL
#define _T1_TICKS_PER_MS    HAL_TIMER_CTC_TOP(1000UL, _T1_PRESCALER_VAL)

int8_t HAL_Timer_Timestamp_Init(void)
{
    hal_timer_config_t cfg = {
        .mode      = HAL_TIMER_MODE_CTC,
        .prescaler = HAL_TIMER_PRESCALER_64,
        .top       = _T1_TICKS_PER_MS
    };

    if (HAL_Timer_Init(HAL_TIMER1, &cfg) != 0) return -1;

    HAL_Timer_RegisterCallback(HAL_TIMER1,
                                HAL_TIMER_IRQ_COMPARE_A,
                                NULL);   /* La ISR maneja _millis directamente */

    HAL_Timer_EnableIRQ(HAL_TIMER1, HAL_TIMER_IRQ_COMPARE_A);
    HAL_Timer_Start(HAL_TIMER1);
    return 0;
}

uint32_t HAL_Timer_Millis(void)
{
    uint32_t ms;
    uint8_t sreg;
    /*
     * Lectura atomica del contador de 32 bits.
     * Sin seccion critica, la ISR podria modificar _millis_count
     * a mitad de una lectura de 4 bytes, dando un valor corrupto.
     */
    HAL_CRITICAL_ENTER(sreg);
    ms = _millis_count;
    HAL_CRITICAL_EXIT(sreg);
    return ms;
}

uint32_t HAL_Timer_Micros(void)
{
    uint32_t ms;
    uint16_t tcnt;
    uint8_t sreg;

    HAL_CRITICAL_ENTER(sreg);
    ms   = _millis_count;
    tcnt = TCNT1;   /* Valor actual del contador dentro del ms actual */
    HAL_CRITICAL_EXIT(sreg);

    /*
     * us = ms * 1000 + (tcnt * prescaler / (F_CPU/1000000))
     * Simplificado para F_CPU=16MHz, prescaler=64:
     *   cada tick = 4us
     */
    return (ms * 1000UL) +
           ((uint32_t)tcnt * _T1_PRESCALER_VAL) / (F_CPU / 1000000UL);
}

void HAL_Timer_DelayMs(uint32_t ms)
{
    uint32_t start = HAL_Timer_Millis();
    while ((HAL_Timer_Millis() - start) < ms) {
        /* Espera activa, pero las IRQ siguen funcionando */
    }
}

/* =========================================
   8. ISR DE TIMERS
   =========================================
   Cada ISR sigue el mismo patron:
     1. Llamar al callback registrado, si existe
     2. Actualizar contadores internos si aplica
   ========================================= */

/* Timer0 */
ISR(TIMER0_OVF_vect)
{
    if (_timer_state[HAL_TIMER0].cb_overflow != NULL) {
        _timer_state[HAL_TIMER0].cb_overflow();
    }
}

ISR(TIMER0_COMPA_vect)
{
    if (_timer_state[HAL_TIMER0].cb_compare_a != NULL) {
        _timer_state[HAL_TIMER0].cb_compare_a();
    }
}

ISR(TIMER0_COMPB_vect)
{
    if (_timer_state[HAL_TIMER0].cb_compare_b != NULL) {
        _timer_state[HAL_TIMER0].cb_compare_b();
    }
}

/* Timer1 */
ISR(TIMER1_OVF_vect)
{
    if (_timer_state[HAL_TIMER1].cb_overflow != NULL) {
        _timer_state[HAL_TIMER1].cb_overflow();
    }
}

ISR(TIMER1_COMPA_vect)
{
    /*
     * Si el timestamp esta activo, este vector incrementa _millis_count.
     * Si ademas hay un callback de usuario, lo llama despues.
     */
    _millis_count++;

    if (_timer_state[HAL_TIMER1].cb_compare_a != NULL) {
        _timer_state[HAL_TIMER1].cb_compare_a();
    }
}

ISR(TIMER1_COMPB_vect)
{
    if (_timer_state[HAL_TIMER1].cb_compare_b != NULL) {
        _timer_state[HAL_TIMER1].cb_compare_b();
    }
}

/* Timer2 */
ISR(TIMER2_OVF_vect)
{
    if (_timer_state[HAL_TIMER2].cb_overflow != NULL) {
        _timer_state[HAL_TIMER2].cb_overflow();
    }
}

ISR(TIMER2_COMPA_vect)
{
    if (_timer_state[HAL_TIMER2].cb_compare_a != NULL) {
        _timer_state[HAL_TIMER2].cb_compare_a();
    }
}

ISR(TIMER2_COMPB_vect)
{
    if (_timer_state[HAL_TIMER2].cb_compare_b != NULL) {
        _timer_state[HAL_TIMER2].cb_compare_b();
    }
}

/* Timer3 — solo ATmega1284P */
#if defined(HAL_TARGET_1284P)
ISR(TIMER3_OVF_vect)
{
    if (_timer_state[HAL_TIMER3].cb_overflow != NULL) {
        _timer_state[HAL_TIMER3].cb_overflow();
    }
}

ISR(TIMER3_COMPA_vect)
{
    if (_timer_state[HAL_TIMER3].cb_compare_a != NULL) {
        _timer_state[HAL_TIMER3].cb_compare_a();
    }
}

ISR(TIMER3_COMPB_vect)
{
    if (_timer_state[HAL_TIMER3].cb_compare_b != NULL) {
        _timer_state[HAL_TIMER3].cb_compare_b();
    }
}
#endif

/* =========================================
   9. EJEMPLOS DE USO
   ========================================= */

/*
 * -----------------------------------------------------------
 * EJEMPLO 1: Tarea periodica cada 500ms con CTC + callback
 * -----------------------------------------------------------
 * Timer0 en CTC genera una IRQ cada 500ms.
 * El callback togglea un LED sin bloquear el loop principal.
 *
 * #include "timer.h"
 * #include "gpio.h"
 * #include "interrupt.h"
 *
 * static void cada_500ms(void) {
 *     GPIO_TOGGLE_PORTB(5);   // LED en PB5
 * }
 *
 * int main(void) {
 *     GPIO_PIN_MODE_PORTB(5, OUTPUT);
 *
 *     hal_timer_config_t cfg = {
 *         .mode      = HAL_TIMER_MODE_CTC,
 *         .prescaler = HAL_TIMER_PRESCALER_1024,
 *         .top       = HAL_TIMER_CTC_TOP(2UL, 1024UL)  // 2 veces/seg = 500ms
 *     };
 *     HAL_Timer_Init(HAL_TIMER0, &cfg);
 *     HAL_Timer_RegisterCallback(HAL_TIMER0, HAL_TIMER_IRQ_COMPARE_A, cada_500ms);
 *     HAL_Timer_EnableIRQ(HAL_TIMER0, HAL_TIMER_IRQ_COMPARE_A);
 *     HAL_Timer_Start(HAL_TIMER0);
 *     HAL_IRQ_ENABLE();
 *
 *     while (1) { }  // el LED parpadea solo
 * }
 * -----------------------------------------------------------
 *
 * EJEMPLO 2: PWM en servo con Timer1 (50Hz, 1ms-2ms)
 * -----------------------------------------------------------
 * Un servo RC necesita PWM a 50Hz con duty entre 1ms y 2ms.
 * Con TOP=39999 y prescaler=8, cada tick = 0.5us.
 *   1ms  = 2000 ticks -> 0 grados
 *   1.5ms= 3000 ticks -> 90 grados
 *   2ms  = 4000 ticks -> 180 grados
 *
 * #include "timer.h"
 * #include "gpio.h"
 *
 * int main(void) {
 *     GPIO_PIN_MODE_PORTB(1, OUTPUT);   // OC1A = PB1
 *
 *     hal_timer_config_t cfg = {
 *         .mode      = HAL_TIMER_MODE_FAST_PWM,
 *         .prescaler = HAL_TIMER_PRESCALER_8,
 *         .top       = HAL_TIMER_PWM_TOP(50UL, 8UL)   // = 39999
 *     };
 *     HAL_Timer_Init(HAL_TIMER1, &cfg);
 *     HAL_Timer_PWM_Enable(HAL_TIMER1, HAL_TIMER_CH_A);
 *     HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_A, 3000U);  // 90 grados
 *     HAL_Timer_Start(HAL_TIMER1);
 *
 *     while (1) { }
 * }
 * -----------------------------------------------------------
 *
 * EJEMPLO 3: Timestamp + DelayMs (reemplazo de _delay_ms)
 * -----------------------------------------------------------
 * HAL_Timer_DelayMs permite que las IRQ sigan corriendo
 * durante la espera, a diferencia de _delay_ms de avr-libc.
 *
 * #include "timer.h"
 * #include "interrupt.h"
 *
 * int main(void) {
 *     HAL_Timer_Timestamp_Init();
 *     HAL_IRQ_ENABLE();
 *
 *     while (1) {
 *         uint32_t t0 = HAL_Timer_Millis();
 *
 *         HAL_Timer_DelayMs(1000);   // esperar 1 segundo
 *
 *         uint32_t elapsed = HAL_Timer_Millis() - t0;
 *         // elapsed ~ 1000ms
 *     }
 * }
 * -----------------------------------------------------------
 *
 * EJEMPLO 4: PWM de brillo en LED con duty variable
 * -----------------------------------------------------------
 * Timer2 en Fast PWM, duty aumenta de 0 a 100% gradualmente.
 *
 * #include "timer.h"
 * #include "gpio.h"
 *
 * int main(void) {
 *     GPIO_PIN_MODE_PORTB(3, OUTPUT);   // OC2A = PB3
 *
 *     hal_timer_config_t cfg = {
 *         .mode      = HAL_TIMER_MODE_FAST_PWM,
 *         .prescaler = HAL_TIMER_PRESCALER_64,
 *         .top       = 255U
 *     };
 *     HAL_Timer_Init(HAL_TIMER2, &cfg);
 *     HAL_Timer_PWM_Enable(HAL_TIMER2, HAL_TIMER_CH_A);
 *     HAL_Timer_Start(HAL_TIMER2);
 *     HAL_Timer_Timestamp_Init();
 *     HAL_IRQ_ENABLE();
 *
 *     uint8_t duty = 0U;
 *     while (1) {
 *         HAL_Timer_PWM_SetDuty(HAL_TIMER2, HAL_TIMER_CH_A, duty);
 *         duty = (duty >= 100U) ? 0U : duty + 1U;
 *         HAL_Timer_DelayMs(10);
 *     }
 * }
 * -----------------------------------------------------------
 */
