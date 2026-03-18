# AVR-HAL — Hardware Abstraction Layer para ATmega328P y ATmega1284P

> Proyecto académico y técnico desarrollado en el marco de los programas de formación en sistemas embebidos de la **UCEVA** y **UNIVALLE sede Tuluá**.  
> Autor: **Juan Sebastián Correa Fernández** — Gestor Centro STEAM+ | Docente VIPS UCEVA | Profesor hora cátedra UNIVALLE sede Tuluá

---

## ¿Qué es este proyecto?

**AVR-HAL** es una capa de abstracción de hardware (*Hardware Abstraction Layer*) escrita en **C estándar** para los microcontroladores **ATmega328P** y **ATmega1284P** de Microchip (familia AVR de 8 bits).

Cada módulo ofrece **dos niveles de acceso** complementarios:

| Nivel | Forma | Cuándo usarlo |
|:---|:---|:---|
| **Macros directas** | `GPIO_WRITE_PORTB(5, HIGH)` | ISR, loops de tiempo crítico — sin overhead |
| **Funciones HAL** | `HAL_GPIO_WritePin('B', 5, HIGH)` | Inicialización y lógica de aplicación — con validación |

El target se detecta **automáticamente** a partir del flag `-mmcu` del compilador. No existe ningún archivo de configuración que editar manualmente.

---

## Microcontroladores soportados

| Microcontrolador | Flash | SRAM | UART | Puertos GPIO | Timers | Canales ADC |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|
| ATmega328P | 32 KB | 2 KB | 1 (UART0) | B, C, D | 0(8b), 1(16b), 2(8b) | 8 |
| ATmega1284P | 128 KB | 16 KB | 2 (UART0, UART1) | **A**, B, C, D | 0(8b), 1(16b), 2(8b), **3(16b)** | 8 |

> Los elementos en **negrita** son exclusivos del ATmega1284P y se habilitan automáticamente al compilar con `-mmcu=atmega1284p`.

---

## Módulos implementados

| Módulo | Archivos | Características principales |
|:---|:---|:---|
| **GPIO** | `gpio.h` / `gpio.c` | Macros por puerto y pin; funciones con validación; puerto completo; pull-up |
| **Interrupciones** | `interrupt.h` / `interrupt.c` | Callbacks para EXT_INT y PCINT; secciones críticas atómicas |
| **UART** | `uart.h` / `uart.c` | Polling e IRQ; buffer circular TX/RX; redirección de `printf`; dos instancias en 1284P |
| **ADC** | `adc.h` / `adc.c` | Polling e IRQ; lectura en mV; promedio de muestras; scan multichannel; mapeo de valores |
| **Timer** | `timer.h` / `timer.c` | Modos CTC, Fast PWM, Phase Correct PWM; timestamps millis/micros; callbacks por evento |

---

## Estructura del proyecto

```
avr-hal/
│
├── gpio.h / gpio.c           # Módulo GPIO
├── interrupt.h / interrupt.c # Módulo de interrupciones
├── uart.h / uart.c           # Módulo UART
├── adc.h / adc.c             # Módulo ADC
├── timer.h / timer.c         # Módulo de timers y PWM
│
├── examples/                 # Ejemplos de uso por módulo
│   ├── example_gpio.c
│   ├── example_interrupt.c
│   ├── example_uart.c
│   ├── example_adc.c
│   └── example_timer.c
│
├── docs/
│   ├── guia_tecnica.md       # Arquitectura, registros AVR, API completa
│   └── guia_didactica.md     # Conceptos, ejercicios, ruta de aprendizaje
│
├── main.c
└── README.md
```

---

## Detección automática del target

La portabilidad funciona sin intervención del usuario. `avr-gcc` define un símbolo según el flag `-mmcu`:

```
-mmcu=atmega328p   →  define __AVR_ATmega328P__
-mmcu=atmega1284p  →  define __AVR_ATmega1284P__
```

Cada módulo de la HAL usa ese símbolo para activar o desactivar código:

```c
#if defined(__AVR_ATmega1284P__)
    case 'A':                  // Puerto A — solo 1284P
        *ddr = &DDRA; ...
#endif
```

El mismo `main.c` compila sin modificaciones para ambos microcontroladores.

---

## Ejemplos de uso por módulo

### GPIO — macros y funciones

```c
#include "gpio.h"

int main(void) {
    // Macros (sin overhead — ideales en loops e ISR)
    GPIO_PIN_MODE_PORTB(5, OUTPUT);      // LED como salida
    GPIO_PIN_MODE_PORTD(2, INPUT);       // botón como entrada
    GPIO_PULLUP_PORTD(2, 1U);            // activar pull-up interno

    while (1) {
        if (GPIO_READ_PORTD(2) == LOW) { // botón presionado
            GPIO_WRITE_PORTB(5, HIGH);
        } else {
            GPIO_WRITE_PORTB(5, LOW);
        }
    }
}
```

```c
// Funciones HAL (con validación — ideales en inicialización)
HAL_GPIO_ConfigPin('B', 5, OUTPUT);
HAL_GPIO_WritePin('B', 5, HIGH);
int8_t nivel = HAL_GPIO_ReadPin('D', 2);  // HIGH, LOW o -1 si error
HAL_GPIO_TogglePin('B', 5);
HAL_GPIO_SetPullup('D', 2, 1U);
```

### Interrupción externa con callback

```c
#include "interrupt.h"
#include "gpio.h"

static void on_boton(void) {
    GPIO_TOGGLE_PORTB(5);   // toggle LED — ejecutado desde la ISR
}

int main(void) {
    GPIO_PIN_MODE_PORTB(5, OUTPUT);
    GPIO_PIN_MODE_PORTD(2, INPUT);
    GPIO_PULLUP_PORTD(2, 1U);

    HAL_EXT_INT_RegisterCallback(HAL_EXT_INT0, HAL_IRQ_SENSE_FALL, on_boton);
    HAL_EXT_INT_CLEAR_FLAG(HAL_EXT_INT0);
    HAL_EXT_INT_ENABLE(HAL_EXT_INT0);
    HAL_IRQ_ENABLE();   // sei()

    while (1) { }
}
```

### UART — polling con printf

```c
#include "uart.h"

int main(void) {
    hal_uart_config_t cfg = {
        .baud_rate = 115200U,
        .data_bits = 8U,
        .stop_bits = 1U,
        .parity    = HAL_UART_PARITY_NONE,
        .use_irq   = HAL_UART_MODE_POLLING
    };
    HAL_UART_Init(HAL_UART0, &cfg);
    HAL_UART_SetStdout(HAL_UART0);   // redirige printf al UART

    printf("Sistema iniciado\n");

    while (1) {
        printf("Uptime: %lu ms\n", HAL_Timer_Millis());
    }
}
```

### UART — recepción por IRQ con buffer circular

```c
hal_uart_config_t cfg = { .baud_rate = 9600U, ..., .use_irq = HAL_UART_MODE_IRQ };
HAL_UART_Init(HAL_UART0, &cfg);
HAL_IRQ_ENABLE();

while (1) {
    if (HAL_UART_Available(HAL_UART0) > 0) {
        uint8_t byte;
        HAL_UART_ReceiveByte(HAL_UART0, &byte);
        HAL_UART_SendByte(HAL_UART0, byte);   // eco
    }
    // ... el CPU hace otras cosas mientras espera datos
}
```

### ADC — polling, mV y promedio

```c
#include "adc.h"

hal_adc_config_t cfg = {
    .reference = HAL_ADC_REF_AVCC,
    .prescaler = HAL_ADC_PRESCALER_128,   // 125 kHz con F_CPU=16MHz
    .vref_mv   = 5000U,
    .use_irq   = HAL_ADC_MODE_POLLING
};
HAL_ADC_Init(&cfg);

uint16_t raw, mv, promedio;
HAL_ADC_Read(0, &raw);                 // lectura cruda (0..1023)
HAL_ADC_ReadMillivolts(0, &mv);        // lectura en mV directamente
HAL_ADC_ReadAverage(0, 16U, &promedio); // promedio de 16 muestras

// Mapear ADC a rango físico (ej. potenciómetro -> ángulo 0..180°)
int32_t angulo = HAL_ADC_Map(raw, 0, 1023, 0, 180);
```

### ADC — scan multichannel con IRQ

```c
static void on_ch0(uint8_t ch, uint16_t val) { /* canal 0 listo */ }
static void on_ch1(uint8_t ch, uint16_t val) { /* canal 1 listo */ }
static void on_ch2(uint8_t ch, uint16_t val) { /* canal 2 listo */ }

static const hal_adc_scan_entry_t canales[] = {
    { .channel = 0U, .callback = on_ch0 },
    { .channel = 1U, .callback = on_ch1 },
    { .channel = 2U, .callback = on_ch2 },
};

HAL_ADC_Init(&cfg_irq);
HAL_IRQ_ENABLE();
HAL_ADC_ScanStart(canales, 3U);   // convierte los 3 canales sin bloquear
```

### Timer — timestamps y PWM

```c
#include "timer.h"

// Sistema de tiempo (millis/micros — usa Timer1)
HAL_Timer_Timestamp_Init();
sei();

uint32_t inicio = HAL_Timer_Millis();
while (HAL_Timer_Millis() - inicio < 1000UL) {
    // esperar 1 segundo sin bloquear interrupciones
}

// PWM en Timer1 (pin OC1A) — señal para servo (50 Hz, 1-2 ms)
hal_timer_config_t pwm_cfg = {
    .mode      = HAL_TIMER_MODE_FAST_PWM,
    .prescaler = HAL_TIMER_PRESCALER_8,
    .top       = HAL_TIMER_PWM_TOP(50UL, 8UL)   // TOP para 50 Hz
};
GPIO_PIN_MODE_PORTB(1, OUTPUT);   // OC1A debe ser salida
HAL_Timer_Init(HAL_TIMER1, &pwm_cfg);
HAL_Timer_PWM_Enable(HAL_TIMER1, HAL_TIMER_CH_A);
HAL_Timer_PWM_SetDuty(HAL_TIMER1, HAL_TIMER_CH_A, 75);   // 75% duty
HAL_Timer_Start(HAL_TIMER1);
```

---

## Secciones críticas

Protegen variables compartidas entre el código principal y las ISR:

```c
#include "interrupt.h"

volatile uint16_t contador = 0U;   // compartida con ISR

// En el código principal:
uint8_t  sreg;
uint16_t copia;

HAL_CRITICAL_ENTER(sreg);   // deshabilita IRQ, guarda SREG
copia = contador;            // lectura atómica segura
HAL_CRITICAL_EXIT(sreg);    // restaura SREG (re-habilita IRQ si estaban activas)
```

---

## Convenciones de nombres

| Elemento | Convención | Ejemplo |
|:---|:---|:---|
| Funciones públicas | `HAL_Módulo_Acción()` | `HAL_UART_SendString()` |
| Macros de pin | `GPIO_ACCION_PORTx(pin, ...)` | `GPIO_WRITE_PORTB(5, HIGH)` |
| Macros de puerto completo | `GPIO_PORT_ACCION_x(val)` | `GPIO_PORT_WRITE_D(0xFF)` |
| Tipos | `hal_módulo_tipo_t` | `hal_uart_config_t` |
| Enumeraciones | `HAL_MÓDULO_VALOR` | `HAL_ADC_REF_AVCC` |
| Archivos | `módulo.h` / `módulo.c` | `uart.h` / `uart.c` |
| Constantes de nivel | `HIGH` / `LOW` | — |
| Constantes de dirección | `INPUT` / `OUTPUT` | — |

---

## Entorno de desarrollo

Compatible con cualquier entorno que soporte `avr-gcc` / `avr-libc`:

- **Microchip Studio** (Windows) — recomendado para principiantes
- **VS Code + avr-gcc + avrdude** — Windows, Linux, macOS
- **Makefile** con toolchain AVR estándar

---

## Documentación

| Documento | Contenido |
|:---|:---|
| [`docs/guia_tecnica.md`](docs/guia_tecnica.md) | Arquitectura en capas, registros AVR involucrados, decisiones de diseño, API completa de los 5 módulos |
| [`docs/guia_didactica.md`](docs/guia_didactica.md) | Qué es una HAL, comparativa con acceso directo, ejercicios progresivos, buenas prácticas, ruta de aprendizaje de 8 semanas |

---

> *"Una buena capa de abstracción no esconde el hardware — te ayuda a entenderlo mejor."*
