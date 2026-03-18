# AVR-HAL — Hardware Abstraction Layer para ATmega328P y ATmega1284P

> **Proyecto académico y técnico** — UCEVA · UNIVALLE sede Tuluá · Centro STEAM+  
> **Autor:** Juan Sebastián Correa Fernández — Gestor Centro STEAM+ | Docente VIPS UCEVA | Profesor hora cátedra UNIVALLE sede Tuluá

---

## 🗂️ Tabla de contenido

### Fundamentos
- [¿Qué es una HAL?](#-qué-es-una-hal)
- [¿Por qué usar una HAL?](#-por-qué-usar-una-hal)
- [Programar directo vs. usar una HAL](#-programar-directo-vs-usar-una-hal)
- [Los dos niveles de esta HAL](#-los-dos-niveles-de-esta-hal)
- [Portabilidad automática entre microcontroladores](#-portabilidad-automática-entre-microcontroladores)

### El proyecto
- [Microcontroladores soportados](#-microcontroladores-soportados)
- [Módulos implementados](#-módulos-implementados)
- [Estructura del proyecto](#-estructura-del-proyecto)
- [Entorno de desarrollo](#-entorno-de-desarrollo)

### Módulos — conceptos y uso
- [Módulo GPIO](#-módulo-gpio)
- [Módulo Interrupciones](#-módulo-interrupciones)
- [Módulo UART](#-módulo-uart)
- [Módulo ADC](#-módulo-adc)
- [Módulo Timer](#-módulo-timer)

### Referencia
- [Convenciones de nombres](#-convenciones-de-nombres)
- [Diferencias entre ATmega328P y ATmega1284P](#-diferencias-entre-atmega328p-y-atmega1284p)
- [Buenas prácticas](#-buenas-prácticas)
- [Ruta de aprendizaje sugerida](#-ruta-de-aprendizaje-sugerida)

---

## 🧠 ¿Qué es una HAL?

Una **HAL** (*Hardware Abstraction Layer* — Capa de Abstracción de Hardware) es una interfaz de software que separa el código de la aplicación de los detalles concretos del hardware.

Cuando se programa un microcontrolador directamente, el código accede a registros específicos de ese dispositivo: `DDRB`, `PORTB`, `UCSR0A`, `ADCSRA`... Eso funciona perfectamente, pero genera un problema: el código queda **atado** a ese microcontrolador específico. Si en el futuro se cambia de modelo, hay que reescribir todo.

Una HAL resuelve esto colocando una capa intermedia:

```
Sin HAL:   Tu código  →  Registros del AVR  →  Hardware
Con HAL:   Tu código  →  HAL  →  Registros del AVR  →  Hardware
```

La HAL sabe exactamente cómo hablarle al hardware. El código de la aplicación solo necesita saber **qué quiere hacer**, no **cómo hacerlo** a nivel de registros.

### Analogía: el volante del automóvil

Cuando se conduce, se gira el volante. No es necesario saber si el sistema de dirección es hidráulico, electrohidráulico o eléctrico. Ese mecanismo es la "HAL del auto". Si se cambia de vehículo, se sigue girando el volante igual, aunque internamente todo sea diferente.

---

## ✅ ¿Por qué usar una HAL?

| Ventaja | Sin HAL | Con HAL |
|:---|:---|:---|
| **Portabilidad** | Código atado al microcontrolador | Cambiar de MCU = cambiar el flag `-mmcu` |
| **Mantenimiento** | Cambiar hardware implica reescribir la app | Solo se actualiza la HAL |
| **Legibilidad** | Require conocer los registros del AVR | `OUTPUT`, `HIGH`, `LOW` son auto-explicativos |
| **Seguridad** | Sin validación de parámetros | Las funciones `HAL_*` validan y retornan errores |
| **Trabajo en equipo** | Todos deben conocer el hardware | El que usa la HAL no necesita conocer el hardware |

### ¿Qué costo tiene?

Nada es gratis. Usar una HAL tiene compromisos:

| Costo | Impacto en esta HAL |
|:---|:---|
| Memoria Flash adicional | Muy bajo — las macros no generan código extra |
| Velocidad de ejecución | Nulo para macros; mínimo para funciones |
| Más archivos en el proyecto | Sí, pero organizados y con propósito claro |
| Curva de aprendizaje inicial | Moderada — hay que entender la estructura |

Para el ATmega328P (32 KB Flash) y ATmega1284P (128 KB Flash), el costo es completamente negligible.

---

## ⚖️ Programar directo vs. usar una HAL

La misma tarea realizada de las dos formas:

**Tarea:** encender el LED en PB5, esperar 500 ms, apagarlo.

### Sin HAL — acceso directo a registros

```c
#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    DDRB  |= (1 << DDB5);     // configurar PB5 como salida escribiendo 1 en el bit 5 de DDRB
    PORTB |= (1 << PORTB5);   // encender: escribir 1 en el bit 5 de PORTB
    _delay_ms(500);
    PORTB &= ~(1 << PORTB5);  // apagar: escribir 0 en el bit 5 de PORTB
    while(1);
}
```

### Con HAL

```c
#include "gpio.h"

int main(void) {
    GPIO_PIN_MODE_PORTB(5, OUTPUT);   // configura DDR
    GPIO_WRITE_PORTB(5, HIGH);        // escribe en PORT
    _delay_ms(500);
    GPIO_WRITE_PORTB(5, LOW);
    while(1);
}
```

> **Punto clave:** la HAL **no oculta** los registros. El acceso sigue siendo exactamente el mismo. La diferencia es que está organizado y nombrado de forma que cualquiera pueda entenderlo. Dentro de `GPIO_WRITE_PORTB`, el compilador genera exactamente la misma instrucción que `PORTB |= (1 << 5)`.

---

## 🔲 Los dos niveles de esta HAL

Esta HAL ofrece dos formas de acceder al hardware en cada módulo:

### Nivel 1 — Macros (velocidad máxima, sin overhead)

```c
GPIO_WRITE_PORTB(5, HIGH);   // el compilador lo reemplaza por la instrucción AVR directamente
```

- Se resuelven **en tiempo de compilación**
- No generan llamada a función
- No validan parámetros
- Ideales dentro de **ISR** y **loops de tiempo crítico**

### Nivel 2 — Funciones `HAL_*` (con validación y retorno de error)

```c
int8_t resultado = HAL_GPIO_WritePin('B', 5, HIGH);
if (resultado != 0) {
    // el puerto o pin era inválido — el error fue capturado
}
```

- Validan el puerto y el número de pin antes de operar
- Retornan `0` si todo fue bien, `-1` si algo es inválido
- Más costosas (llamada a función), pero más seguras
- Ideales en **inicialización** y **lógica de aplicación**

### Regla práctica

```
Loops e ISR        →  usar macros        (GPIO_WRITE_PORTB, GPIO_READ_PORTD...)
Inicialización     →  usar funciones     (HAL_GPIO_ConfigPin, HAL_UART_Init...)
```

Se pueden mezclar en el mismo proyecto sin ningún problema.

---

## 🔄 Portabilidad automática entre microcontroladores

No existe ningún archivo de configuración que editar. La selección del microcontrolador ocurre al momento de compilar, a través del flag `-mmcu`:

```bash
avr-gcc -mmcu=atmega328p  ...   # compila para ATmega328P
avr-gcc -mmcu=atmega1284p ...   # compila para ATmega1284P
```

`avr-gcc` define automáticamente un símbolo según el MCU elegido:

```
-mmcu=atmega328p   →  define  __AVR_ATmega328P__
-mmcu=atmega1284p  →  define  __AVR_ATmega1284P__
```

Cada módulo de la HAL usa ese símbolo para activar o desactivar código:

```c
// En gpio.c — el Puerto A solo existe en el ATmega1284P
#if defined(__AVR_ATmega1284P__)
    case 'A':
        *ddr = &DDRA;  *port_reg = &PORTA;  *pin_reg = &PINA;
        return 0;
#endif

// En interrupt.h — INT2 solo existe en el ATmega1284P
typedef enum {
    HAL_EXT_INT0 = 0U,
    HAL_EXT_INT1 = 1U,
#if defined(__AVR_ATmega1284P__)
    HAL_EXT_INT2 = 2U,      // ← solo disponible en 1284P
#endif
    HAL_EXT_INT_COUNT
} hal_ext_int_t;
```

El mismo `main.c` compila para ambos targets sin ningún cambio. Los periféricos exclusivos del 1284P (Puerto A, UART1, Timer3, INT2, PCINT_BANK3) quedan disponibles automáticamente.

---

## 🎛️ Microcontroladores soportados

| Característica | ATmega328P | ATmega1284P |
|:---|:---:|:---:|
| Flash | 32 KB | 128 KB |
| SRAM | 2 KB | 16 KB |
| EEPROM | 1 KB | 4 KB |
| Puertos GPIO | B, C, D | **A**, B, C, D |
| Pines I/O totales | 23 | 32 |
| UART | 1 (UART0) | 2 (UART0 + **UART1**) |
| Timer 8 bits | Timer0, Timer2 | Timer0, Timer2 |
| Timer 16 bits | Timer1 | Timer1, **Timer3** |
| INT externas | INT0, INT1 | INT0, INT1, **INT2** |
| Bancos PCINT | BANK0, 1, 2 | BANK0, 1, 2, **3** |
| Canales ADC | 8 | 8 |

> Todo lo que aparece en **negrita** se habilita automáticamente al compilar con `-mmcu=atmega1284p`.

---

## 📦 Módulos implementados

| Módulo | Archivos | Descripción |
|:---|:---|:---|
| [GPIO](#-módulo-gpio) | `gpio.h` / `gpio.c` | Macros y funciones para pines, puertos y pull-ups |
| [Interrupciones](#-módulo-interrupciones) | `interrupt.h` / `interrupt.c` | Callbacks para EXT_INT y PCINT; secciones críticas |
| [UART](#-módulo-uart) | `uart.h` / `uart.c` | Polling e IRQ, buffer circular, printf, dos instancias |
| [ADC](#-módulo-adc) | `adc.h` / `adc.c` | Polling e IRQ, mV, promedio, scan multichannel, mapeo |
| [Timer](#-módulo-timer) | `timer.h` / `timer.c` | CTC, PWM, timestamps millis/micros, callbacks por evento |

---

## 🗃️ Estructura del proyecto

```
avr-hal/
│
├── gpio.h / gpio.c            # Módulo GPIO
├── interrupt.h / interrupt.c  # Módulo de interrupciones
├── uart.h / uart.c            # Módulo UART
├── adc.h / adc.c              # Módulo ADC
├── timer.h / timer.c          # Módulo de timers y PWM
│
├── examples/                  # Ejemplos de uso por módulo
│   ├── example_gpio.c
│   ├── example_interrupt.c
│   ├── example_uart.c
│   ├── example_adc.c
│   └── example_timer.c
│
├── main.c                     # Punto de entrada
└── README.md                  # Este archivo
```

---

## 🛠️ Entorno de desarrollo

Compatible con cualquier entorno que soporte `avr-gcc` / `avr-libc`:

- **Microchip Studio** (Windows) — recomendado para principiantes
- **VS Code + avr-gcc + avrdude** — Windows, Linux, macOS
- **Makefile** con toolchain AVR estándar

**Parámetros de compilación típicos:**

```bash
# ATmega328P a 16 MHz
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -Wall \
        -o main.elf main.c gpio.c interrupt.c uart.c adc.c timer.c

# ATmega1284P a 16 MHz
avr-gcc -mmcu=atmega1284p -DF_CPU=16000000UL -Os -Wall \
        -o main.elf main.c gpio.c interrupt.c uart.c adc.c timer.c
```

> Solo cambia `-mmcu`. El resto del comando es idéntico.

---

---

# MÓDULOS — Conceptos y uso

---

## 🔌 Módulo GPIO

[↑ Volver al inicio](#-tabla-de-contenido)

### ¿Qué es GPIO?

**GPIO** (*General Purpose Input/Output*) son los pines digitales de propósito general del microcontrolador. Cada pin puede configurarse como entrada o salida y leerse o escribirse individualmente o en grupos de 8 (puertos completos).

### Los tres registros del AVR

En AVR, cada puerto GPIO se controla con **tres registros de 8 bits**. Cada bit del registro corresponde a un pin del puerto:

| Registro | Nombre completo | Función |
|:---|:---|:---|
| `DDRx` | Data Direction Register | Define la **dirección** del pin: `0` = entrada, `1` = salida |
| `PORTx` | Port Output Register | Si el pin es salida: escribe nivel lógico (`0`/`1`). Si es entrada: activa el pull-up (`1`) o lo desactiva (`0`) |
| `PINx` | Port Input Register | **Solo lectura.** Refleja el nivel físico real del pin en ese momento |

**Ejemplo — controlar PB5 (LED del Arduino UNO):**

```c
// Acceso directo (sin HAL):
DDRB  |= (1 << 5);    // bit 5 de DDRB = 1 → PB5 como salida
PORTB |= (1 << 5);    // bit 5 de PORT = 1 → PB5 en HIGH (5V)
PORTB &= ~(1 << 5);   // bit 5 de PORT = 0 → PB5 en LOW  (0V)
uint8_t v = (PINB >> 5) & 1; // leer el nivel real de PB5

// Con HAL (equivalente exacto):
GPIO_PIN_MODE_PORTB(5, OUTPUT);
GPIO_WRITE_PORTB(5, HIGH);
GPIO_WRITE_PORTB(5, LOW);
uint8_t v = GPIO_READ_PORTB(5);
```

### La resistencia pull-up interna

Cuando un pin está configurado como **entrada** sin nada conectado, queda "flotante": puede leer `HIGH` o `LOW` de forma aleatoria. La solución es activar la **resistencia pull-up interna** del AVR (~50 kΩ conectada internamente a VCC):

```
VCC ──[~50kΩ]── Pin ── (entrada del micro)
                 │
               [Botón]
                 │
               GND
```

Con pull-up activo: pin en reposo = `HIGH`. Al presionar el botón (conectado a GND): pin = `LOW`.

```c
GPIO_PIN_MODE_PORTD(2, INPUT);
GPIO_PULLUP_PORTD(2, 1U);   // 1U = activar pull-up
// Ahora: sin presionar = HIGH, presionado = LOW
```

### Macros disponibles

#### Por pin individual

| Macro | Descripción |
|:---|:---|
| `GPIO_PIN_MODE(ddr, pin, mode)` | Configura dirección de un pin (general, recibe el registro) |
| `GPIO_WRITE(port, pin, value)` | Escribe `HIGH` o `LOW` en un pin |
| `GPIO_READ(pinreg, pin)` | Lee el nivel del pin → retorna `HIGH` o `LOW` |
| `GPIO_TOGGLE(port, pin)` | Invierte el estado actual del pin |
| `GPIO_PULLUP(port, pin, enable)` | Activa (`1U`) o desactiva (`0U`) pull-up interno |

#### Por puerto específico

```c
// Formato: GPIO_ACCION_PORTx(pin, ...)

// Puerto B — ambos targets
GPIO_PIN_MODE_PORTB(pin, mode)    GPIO_WRITE_PORTB(pin, value)
GPIO_READ_PORTB(pin)              GPIO_TOGGLE_PORTB(pin)
GPIO_PULLUP_PORTB(pin, enable)

// Puerto C — ambos targets
GPIO_PIN_MODE_PORTC(pin, mode)    GPIO_WRITE_PORTC(pin, value)
GPIO_READ_PORTC(pin)              GPIO_TOGGLE_PORTC(pin)
GPIO_PULLUP_PORTC(pin, enable)

// Puerto D — ambos targets
GPIO_PIN_MODE_PORTD(pin, mode)    GPIO_WRITE_PORTD(pin, value)
GPIO_READ_PORTD(pin)              GPIO_TOGGLE_PORTD(pin)
GPIO_PULLUP_PORTD(pin, enable)

// Puerto A — SOLO ATmega1284P
GPIO_PIN_MODE_PORTA(pin, mode)    GPIO_WRITE_PORTA(pin, value)
GPIO_READ_PORTA(pin)              GPIO_TOGGLE_PORTA(pin)
GPIO_PULLUP_PORTA(pin, enable)
```

#### Puerto completo (8 bits a la vez)

```c
GPIO_PORT_MODE(ddr, value)      // dirección de los 8 pines
GPIO_PORT_WRITE(port, value)    // escribe un byte completo
GPIO_PORT_READ(pinreg)          // lee el estado de los 8 pines
GPIO_PORT_TOGGLE(port)          // invierte los 8 pines

// Por nombre de puerto:
GPIO_PORT_MODE_B(value)   GPIO_PORT_WRITE_B(value)   GPIO_PORT_READ_B()
GPIO_PORT_MODE_C(value)   GPIO_PORT_WRITE_C(value)   GPIO_PORT_READ_C()
GPIO_PORT_MODE_D(value)   GPIO_PORT_WRITE_D(value)   GPIO_PORT_READ_D()
GPIO_PORT_MODE_A(value)   GPIO_PORT_WRITE_A(value)   GPIO_PORT_READ_A()  // solo 1284P
```

### Funciones HAL (con validación)

El parámetro `port` es una letra: `'A'`, `'B'`, `'C'`, `'D'` (mayúscula o minúscula).

| Función | Retorno | Descripción |
|:---|:---:|:---|
| `HAL_GPIO_ConfigPin(port, pin, mode)` | `0`/`-1` | Configura dirección del pin con validación |
| `HAL_GPIO_WritePin(port, pin, value)` | `0`/`-1` | Escribe nivel lógico |
| `HAL_GPIO_ReadPin(port, pin)` | `HIGH`/`LOW`/`-1` | Lee nivel del pin |
| `HAL_GPIO_TogglePin(port, pin)` | `0`/`-1` | Invierte el estado |
| `HAL_GPIO_SetPullup(port, pin, enable)` | `0`/`-1` | Activa/desactiva pull-up |

### Ejemplo completo — LED + botón

```c
#include "gpio.h"

int main(void) {
    // Configurar LED como salida y botón como entrada con pull-up
    GPIO_PIN_MODE_PORTB(5, OUTPUT);
    GPIO_PIN_MODE_PORTD(2, INPUT);
    GPIO_PULLUP_PORTD(2, 1U);

    while (1) {
        // Botón presionado → nivel LOW (pull-up invierte la lógica)
        if (GPIO_READ_PORTD(2) == LOW) {
            GPIO_WRITE_PORTB(5, HIGH);   // encender LED
        } else {
            GPIO_WRITE_PORTB(5, LOW);    // apagar LED
        }
    }
}
```

### Cuándo usar macros y cuándo usar funciones

```c
// En una ISR — siempre macros (rapidez, sin overhead)
ISR(INT0_vect) {
    GPIO_TOGGLE_PORTB(5);        // una sola instrucción AVR
}

// En la inicialización — funciones HAL (legibilidad y seguridad)
int8_t res = HAL_GPIO_ConfigPin('B', 5, OUTPUT);
if (res != 0) { /* error: puerto o pin inválido */ }
```

---

## ⚡ Módulo Interrupciones

[↑ Volver al inicio](#-tabla-de-contenido)

### ¿Qué es una interrupción?

Una interrupción es una señal que el hardware envía al CPU diciéndole: *"ocurrió algo importante, detente y atiéndelo"*. El CPU pausa lo que estaba haciendo, ejecuta la **ISR** (*Interrupt Service Routine* — Rutina de Servicio de Interrupción), y luego regresa exactamente donde estaba.

```
Código principal:  [A][A][A]──────────────[A][A][A]
                              ↑          ↑
                           IRQ dispara  ISR termina
                              └──[ISR]──┘
```

Sin interrupciones, el CPU tendría que revisar constantemente si ocurrió algo (polling). Con interrupciones, puede hacer otras cosas y el hardware avisa cuando es necesario.

### Tipos de interrupción en esta HAL

**EXT_INT — Interrupción externa en pin dedicado**

Pines específicos del MCU que pueden detectar eventos con control del tipo de flanco:

| Target | Fuente | Pin |
|:---|:---|:---|
| ATmega328P | INT0 | PD2 |
| ATmega328P | INT1 | PD3 |
| ATmega1284P | INT0 | PD2 |
| ATmega1284P | INT1 | PD3 |
| ATmega1284P | INT2 | PB2 (solo 1284P) |

**PCINT — Pin Change Interrupt**

Cualquier pin del microcontrolador puede generar una interrupción al cambiar de nivel. Los pines se agrupan en **bancos** de 8; todos los pines del mismo banco comparten un único vector de interrupción.

| Banco | Puerto | Disponible en |
|:---|:---|:---|
| BANK0 | B | Ambos |
| BANK1 | C | Ambos |
| BANK2 | D | Ambos |
| BANK3 | A | Solo 1284P |

### Modos de disparo (sense mode)

Solo aplican a EXT_INT (las PCINT siempre detectan cualquier cambio):

| Constante | Evento |
|:---|:---|
| `HAL_IRQ_SENSE_LOW` | IRQ mientras el pin esté en nivel bajo continuo |
| `HAL_IRQ_SENSE_CHANGE` | IRQ en cualquier cambio de nivel (subida o bajada) |
| `HAL_IRQ_SENSE_FALL` | IRQ solo al bajar de HIGH a LOW (flanco de bajada) |
| `HAL_IRQ_SENSE_RISE` | IRQ solo al subir de LOW a HIGH (flanco de subida) |

### Macros de control global

```c
HAL_IRQ_ENABLE()    // sei()  — habilita todas las IRQ globalmente
HAL_IRQ_DISABLE()   // cli()  — deshabilita todas las IRQ
```

### Macros de EXT_INT

```c
HAL_EXT_INT_ENABLE(int_num)      // habilita INTx en el registro EIMSK
HAL_EXT_INT_DISABLE(int_num)     // deshabilita INTx
HAL_EXT_INT_CLEAR_FLAG(int_num)  // limpia el flag pendiente en EIFR
                                  // ⚠️ En AVR, los flags se limpian escribiendo 1
```

### Macros de PCINT

```c
HAL_PCINT_BANK_ENABLE(bank)            // habilita el banco en PCICR
HAL_PCINT_BANK_DISABLE(bank)           // deshabilita el banco
HAL_PCINT_PIN_ENABLE(pcmsk, mask)      // habilita pines específicos en PCMSKx
HAL_PCINT_PIN_DISABLE(pcmsk, mask)     // deshabilita pines específicos
HAL_PCINT_CLEAR_FLAG(bank)             // limpia el flag del banco en PCIFR
```

### Funciones de registro de callbacks

En lugar de escribir la ISR directamente, se registra una función normal:

| Función | Descripción |
|:---|:---|
| `HAL_EXT_INT_RegisterCallback(source, sense, callback)` | Registra callback para INTx y configura el sense mode |
| `HAL_PCINT_RegisterCallback(bank, callback)` | Registra callback para un banco PCINT completo |

Estas funciones **no habilitan** la interrupción. Solo configuran el modo y guardan el puntero a la función. La habilitación es un paso explícito posterior.

### Secuencias completas de configuración

**Para EXT_INT:**

```c
static void mi_callback(void) {
    GPIO_TOGGLE_PORTB(5);
}

// 1. Configurar el pin como entrada con pull-up
GPIO_PIN_MODE_PORTD(2, INPUT);
GPIO_PULLUP_PORTD(2, 1U);

// 2. Registrar el callback y configurar flanco
HAL_EXT_INT_RegisterCallback(HAL_EXT_INT0, HAL_IRQ_SENSE_FALL, mi_callback);

// 3. Limpiar flancos previos (pueden haberse acumulado antes de configurar)
HAL_EXT_INT_CLEAR_FLAG(HAL_EXT_INT0);

// 4. Habilitar la interrupción específica
HAL_EXT_INT_ENABLE(HAL_EXT_INT0);

// 5. Habilitar interrupciones globales (¡siempre al final!)
HAL_IRQ_ENABLE();
```

**Para PCINT:**

```c
HAL_PCINT_RegisterCallback(HAL_PCINT_BANK0, mi_callback);
HAL_PCINT_PIN_ENABLE(PCMSK0, (1 << PCINT3) | (1 << PCINT5));  // pines PB3 y PB5
HAL_PCINT_BANK_ENABLE(HAL_PCINT_BANK0);
HAL_IRQ_ENABLE();
```

### La sección crítica — proteger datos compartidos

**El problema:** una variable de 16 bits (`uint16_t`) requiere **dos instrucciones** de lectura en AVR (un byte a la vez). Si la ISR modifica esa variable entre las dos lecturas del código principal, el valor leído es incorrecto.

```
Código principal:   lee byte bajo  ←──── ISR interrumpe aquí ────→  lee byte alto
                        0x00                  escribe 0x0100               0x01
                    Resultado leído: 0x0100  ← INCORRECTO
```

**La solución — sección crítica:**

```c
volatile uint16_t contador = 0U;   // compartida con ISR
                                   // 'volatile' = siempre leer de memoria real

uint8_t  sreg;
uint16_t copia;

HAL_CRITICAL_ENTER(sreg);    // guarda SREG, deshabilita IRQ
copia = contador;             // lectura atómica — sin riesgo de interrupción
HAL_CRITICAL_EXIT(sreg);     // restaura SREG (re-habilita IRQ si estaban activas)

procesar(copia);
```

**¿Por qué no usar `cli()/sei()` directamente?**

Si las IRQ ya estaban deshabilitadas antes de entrar, el `sei()` al salir las habilitaría incorrectamente. `HAL_CRITICAL_EXIT` restaura el estado exacto de `SREG` — si ya estaban deshabilitadas, siguen deshabilitadas.

### Regla de oro de las ISR

> **Las ISR deben ser lo más cortas posible. Solo mover datos, nunca procesar.**

```c
// ✅ CORRECTO: ISR corta
volatile uint8_t flag = 0U;

static void on_boton(void) {
    flag = 1U;           // activar bandera y salir
}

// En el bucle principal:
if (flag) {
    flag = 0U;
    hacer_el_trabajo();  // aquí sí se procesa
}

// ❌ INCORRECTO: lógica pesada dentro de la ISR
static void on_boton_mal(void) {
    calcular_promedio();         // puede tardar muchos ciclos
    HAL_UART_SendString(...);    // bloqueante — nunca aquí
    HAL_ADC_ReadAverage(...);    // también bloqueante
}
```

---

## 📡 Módulo UART

[↑ Volver al inicio](#-tabla-de-contenido)

### ¿Qué es UART?

**UART** (*Universal Asynchronous Receiver-Transmitter*) es el protocolo de comunicación serie más común en sistemas embebidos. Permite enviar y recibir datos bit a bit entre dos dispositivos usando dos cables:

- **TX** (Transmit): el microcontrolador envía datos por este pin
- **RX** (Receive): el microcontrolador recibe datos por este pin

La comunicación es **asíncrona**: no hay un reloj compartido entre los dos dispositivos. En su lugar, ambos acuerdan de antemano una velocidad llamada **baudrate** (bits por segundo). Si los baudrates no coinciden exactamente, los datos recibidos serán corruptos.

El módulo de hardware en AVR se llama **USART** (*Universal Synchronous/Asynchronous Receiver-Transmitter*) porque también soporta modo síncrono, aunque aquí solo se usa el asíncrono.

### Registros del USART en AVR

| Registro | Función |
|:---|:---|
| `UBRRnH/L` | **Baud Rate Register** (16 bits = parte alta + baja). Controla la velocidad |
| `UCSRnA` | Estado: `RXC` (dato recibido), `UDRE` (buffer TX vacío para nuevo dato) |
| `UCSRnB` | Control: `RXEN` (habilita receptor), `TXEN` (habilita transmisor), `RXCIE` (IRQ RX), `UDRIE` (IRQ TX vacío) |
| `UCSRnC` | Formato del frame: `UCSZn` (bits de datos), `UPMn` (paridad), `USBS` (bits de parada) |
| `UDRn` | **Registro de datos**: escribir = transmitir, leer = recibir |

### Cálculo del baudrate

```
UBRR = (F_CPU / (16 × baudrate)) − 1

Ejemplo con F_CPU = 16 MHz y baudrate = 115200:
UBRR = (16,000,000 / (16 × 115,200)) − 1 = 8.68... ≈ 8
Error de baudrate: |(115,200 − 16,000,000/(16×(8+1))) / 115,200| = 3.5%
```

La HAL calcula esto automáticamente: `HAL_UART_UBRR(baud)`.

### Modos de operación

**Modo polling** — simple, bloqueante:

```
HAL_UART_SendByte():  espera UDRE=1  →  escribe en UDR  →  retorna
HAL_UART_ReceiveByte(): espera RXC=1  →  lee UDR          →  retorna
```

Durante la espera, el CPU no hace nada más. Útil para debug y prototipos simples.

**Modo IRQ con buffer circular** — no bloqueante, para producción:

```
TX: código principal escribe en buffer TX
    → ISR (UDRE) vacía el buffer automáticamente byte a byte

RX: ISR (RXC) deposita cada byte recibido en buffer RX
    → código principal lee del buffer cuando quiere
```

### El buffer circular (ring buffer)

Es la estructura de datos central del modo IRQ. Permite que la ISR y el código principal intercambien datos sin bloquearse mutuamente.

```
Arreglo de 8 posiciones (mask = 7 = 0b0111):

[ ][ ][D][D][D][ ][ ][ ]
         ↑           ↑
        tail         head

tail = próxima posición a leer   (avanza al leer)
head = próxima posición a escribir (avanza al escribir)
```

- `head == tail` → buffer **vacío**
- `(head + 1) & mask == tail` → buffer **lleno** (se sacrifica una posición)
- Wrap-around eficiente: `head = (head + 1) & mask` en lugar de `% tamaño`
- El tamaño **debe ser potencia de 2** (16, 32, 64...) para que la máscara funcione

El tamaño por defecto es 64 bytes para TX y RX. Se puede cambiar antes de compilar:

```c
// Antes de incluir uart.h, o en las opciones del compilador:
#define UART_RX_BUFFER_SIZE  128U   // debe ser potencia de 2
#define UART_TX_BUFFER_SIZE  128U
```

### Estructura de configuración

```c
hal_uart_config_t cfg = {
    .baud_rate = 115200U,              // velocidad en bps
    .data_bits = 8U,                   // 5, 6, 7 u 8 bits (lo normal: 8)
    .stop_bits = 1U,                   // 1 o 2 bits de parada
    .parity    = HAL_UART_PARITY_NONE, // NONE, EVEN u ODD
    .use_irq   = HAL_UART_MODE_POLLING // POLLING o IRQ
};
HAL_UART_Init(HAL_UART0, &cfg);
```

### API pública

| Función | Descripción |
|:---|:---|
| `HAL_UART_Init(port, &config)` | Inicializa la instancia UART |
| `HAL_UART_Deinit(port)` | Deshabilita y limpia buffers |
| `HAL_UART_SendByte(port, byte)` | Envía un byte |
| `HAL_UART_SendString(port, str)` | Envía cadena terminada en `\0` |
| `HAL_UART_SendInt(port, value)` | Envía entero con signo como texto decimal |
| `HAL_UART_SendHex(port, value)` | Envía byte como `0xNN` |
| `HAL_UART_ReceiveByte(port, *byte)` | Recibe un byte |
| `HAL_UART_Available(port)` | Bytes disponibles en buffer RX (modo IRQ) |
| `HAL_UART_SetStdout(port)` | Redirige `printf()` al UART indicado |

### Redirección de printf

`HAL_UART_SetStdout()` conecta `printf` al UART seleccionado:

```c
HAL_UART_Init(HAL_UART0, &cfg);
HAL_UART_SetStdout(HAL_UART0);

// A partir de aquí, printf envía por UART0:
printf("Temperatura: %d C\n", temperatura);
printf("ADC: %u cuentas = %u mV\n", raw, mv);
```

Internamente, `printf` delega en `uart_putchar()` que convierte `\n` en `\r\n` automáticamente para compatibilidad con terminales.

> **Advertencia:** `printf` es cómodo pero costoso en Flash y RAM. Para sistemas con memoria limitada, usar `HAL_UART_SendString()` y `HAL_UART_SendInt()` directamente es más eficiente.

### ISR implementadas

| ISR | Evento | Acción |
|:---|:---|:---|
| `USART_RX_vect` | Byte recibido en UART0 | Lee `UDR0`, guarda en buffer RX |
| `USART_UDRE_vect` | Buffer TX de UART0 vacío | Lee buffer TX, escribe en `UDR0`; si vacío, deshabilita `UDRIE` |
| `USART1_RX_vect` | Byte recibido en UART1 (1284P) | Igual para `UDR1` |
| `USART1_UDRE_vect` | Buffer TX de UART1 vacío (1284P) | Igual para `UDR1` |

### Ejemplos de uso

**Polling con printf:**

```c
#include "uart.h"

int main(void) {
    hal_uart_config_t cfg = {
        .baud_rate = 115200U, .data_bits = 8U,
        .stop_bits = 1U, .parity = HAL_UART_PARITY_NONE,
        .use_irq   = HAL_UART_MODE_POLLING
    };
    HAL_UART_Init(HAL_UART0, &cfg);
    HAL_UART_SetStdout(HAL_UART0);
    printf("Sistema iniciado.\n");
    while (1) { }
}
```

**IRQ con eco — el CPU no se bloquea:**

```c
hal_uart_config_t cfg = { ..., .use_irq = HAL_UART_MODE_IRQ };
HAL_UART_Init(HAL_UART0, &cfg);
HAL_IRQ_ENABLE();

while (1) {
    if (HAL_UART_Available(HAL_UART0) > 0) {
        uint8_t byte;
        HAL_UART_ReceiveByte(HAL_UART0, &byte);
        HAL_UART_SendByte(HAL_UART0, byte);
    }
    // el CPU puede hacer otras cosas mientras espera datos
}
```

**Dos UARTs en ATmega1284P:**

```c
// UART0 para debug con printf
hal_uart_config_t debug = { .baud_rate = 115200U, ..., .use_irq = HAL_UART_MODE_POLLING };
HAL_UART_Init(HAL_UART0, &debug);
HAL_UART_SetStdout(HAL_UART0);

// UART1 para sensor o módulo externo con buffer
hal_uart_config_t sensor = { .baud_rate = 9600U, ..., .use_irq = HAL_UART_MODE_IRQ };
HAL_UART_Init(HAL_UART1, &sensor);
HAL_IRQ_ENABLE();

while (1) {
    if (HAL_UART_Available(HAL_UART1) > 0) {
        uint8_t dato;
        HAL_UART_ReceiveByte(HAL_UART1, &dato);
        printf("Sensor recibio: 0x%02X\n", dato);
    }
}
```

---

## 📊 Módulo ADC

[↑ Volver al inicio](#-tabla-de-contenido)

### ¿Qué hace el ADC?

El **ADC** (*Analog to Digital Converter*) convierte una tensión analógica continua (cualquier valor entre 0 V y Vref) en un número entero que el microcontrolador puede procesar.

El ADC del ATmega328P y ATmega1284P tiene **10 bits de resolución**, lo que produce valores entre **0 y 1023**:

```
Valor ADC = (Vin × 1023) / Vref

Despejar Vin:
Vin (mV) = (Valor ADC × Vref_mV) / 1023

Ejemplo con Vref = 5000 mV:
  ADC = 0    →  Vin = 0 mV    (0 V)
  ADC = 512  →  Vin = 2501 mV (≈ 2.5 V)
  ADC = 1023 →  Vin = 5000 mV (5 V)
  Resolución = 5000/1023 ≈ 4.89 mV por cuenta
```

### Registros del ADC en AVR

| Registro | Función |
|:---|:---|
| `ADMUX` | Bits `REFS1:0` = referencia de tensión; bits `MUX3:0` = canal analógico |
| `ADCSRA` | `ADEN` = habilita ADC; `ADSC` = inicia conversión; `ADIE` = IRQ al terminar; `ADPS2:0` = prescaler del reloj |
| `ADCL` / `ADCH` | Resultado de 10 bits en dos registros. **Siempre leer ADCL primero**, luego ADCH |
| `ADC` | En avr-gcc, combina ADCL y ADCH en un `uint16_t` directamente |

### Referencia de tensión

La referencia define el valor máximo que el ADC puede medir. Un voltaje igual a Vref produce el resultado máximo (1023):

| Constante | Fuente | Uso típico |
|:---|:---|:---|
| `HAL_ADC_REF_AREF` | Pin AREF externo | Máxima precisión con referencia dedicada (LM4040, etc.) |
| `HAL_ADC_REF_AVCC` | VCC del microcontrolador | Lo más común — mide de 0 a VCC |
| `HAL_ADC_REF_INTERNAL` | Referencia interna 1.1 V | Medir señales pequeñas con mayor resolución |

### El prescaler del ADC — por qué importa

El ADC requiere un reloj entre **50 kHz y 200 kHz** para máxima precisión (10 bits efectivos). Si el reloj es más rápido, el resultado tiene menos bits útiles.

```
Para F_CPU = 16 MHz:

Prescaler 128  →  16 MHz / 128 = 125 kHz  ✅  máxima precisión (recomendado)
Prescaler  64  →  16 MHz / 64  = 250 kHz  ⚠️  ligera pérdida de precisión
Prescaler  16  →  16 MHz / 16  = 1 MHz    ❌  resultado no confiable
```

### La conversión dummy de inicialización

La primera conversión después de habilitar el ADC **siempre se descarta**:

```c
// En HAL_ADC_Init() — se realiza automáticamente:
HAL_ADC_ENABLE();
HAL_ADC_START();
HAL_ADC_WAIT();
(void)HAL_ADC_READ_RESULT();   // leer y descartar — resultado no confiable
```

El datasheet del AVR especifica que la primera conversión toma más ciclos de reloj (inicialización interna del hardware) y no es confiable.

### Modos de operación

**Polling** — síncrono, bloqueante:

```
HAL_ADC_Read() → selecciona canal → espera ADSC=0 → lee ADC → retorna
```

**IRQ** — asíncrono, no bloqueante:

```
HAL_ADC_Read() → selecciona canal → dispara conversión → retorna inmediatamente
ISR(ADC_vect) → lee resultado → llama callback → listo
```

### Estructura de configuración

```c
hal_adc_config_t cfg = {
    .reference = HAL_ADC_REF_AVCC,        // tensión de referencia
    .prescaler = HAL_ADC_PRESCALER_128,   // para F_CPU = 16 MHz
    .vref_mv   = 5000U,                   // Vref en mV (para conversión a mV)
    .use_irq   = HAL_ADC_MODE_POLLING     // POLLING o IRQ
};
HAL_ADC_Init(&cfg);
```

### API pública

| Función | Descripción |
|:---|:---|
| `HAL_ADC_Init(&config)` | Inicializa el ADC con la configuración dada |
| `HAL_ADC_Deinit()` | Deshabilita el ADC (ahorra energía en sleep) |
| `HAL_ADC_Read(channel, *result)` | Lee el canal en cuentas (0–1023) |
| `HAL_ADC_ReadMillivolts(channel, *mv)` | Lee el canal directamente en mV |
| `HAL_ADC_ReadAverage(channel, samples, *result)` | Promedio de N muestras (1–64) |
| `HAL_ADC_Map(value, in_min, in_max, out_min, out_max)` | Mapea a otro rango |
| `HAL_ADC_ScanStart(&entries, count)` | Inicia scan multichannel |
| `HAL_ADC_ScanStop()` | Detiene el scan (solo modo IRQ) |
| `HAL_ADC_RegisterCallback(callback)` | Callback global para modo IRQ |

### Lectura con promedio — para qué sirve

Las señales analógicas reales tienen ruido. Un potenciómetro, un sensor NTC, una señal de audio — todos producen lecturas ligeramente distintas cada vez. Promediar varias muestras reduce ese ruido:

```
Sin promedio:    512, 509, 515, 508, 511  ← valores saltan
Con 16 muestras: 511, 511, 511, 511, 511  ← señal estable
```

```c
uint16_t promedio;
HAL_ADC_ReadAverage(0, 16U, &promedio);   // 16 muestras, descarta la primera (warmup)
```

El costo: tarda 16 veces más. Con prescaler 128, cada conversión toma ~104 µs → 16 muestras = ~1.7 ms.

### HAL_ADC_Map() — de cuentas a unidades físicas

```c
// Potenciómetro (0..1023) → ángulo (0..270 grados)
int32_t angulo = HAL_ADC_Map(raw, 0, 1023, 0, 270);

// Sensor LM35 (10 mV/°C, rango 0–150°C) con Vref = 5V
uint16_t mv;
HAL_ADC_ReadMillivolts(1, &mv);
int32_t temp_c = HAL_ADC_Map(mv, 0, 1500, 0, 150);

// Batería LiPo (3.0 V–4.2 V) como porcentaje
HAL_ADC_ReadMillivolts(2, &mv);
int32_t pct = HAL_ADC_Map(mv, 3000, 4200, 0, 100);
```

Usa aritmética entera de 32 bits internamente para evitar overflow.

### Scan multichannel con IRQ

El scan mode convierte una lista de canales de forma encadenada y no bloqueante:

```c
static void on_ch0(uint8_t ch, uint16_t val) { resultados[0] = val; }
static void on_ch1(uint8_t ch, uint16_t val) { resultados[1] = val; }
static void on_ch2(uint8_t ch, uint16_t val) { resultados[2] = val; }

static const hal_adc_scan_entry_t canales[] = {
    { .channel = 0U, .callback = on_ch0 },
    { .channel = 1U, .callback = on_ch1 },
    { .channel = 2U, .callback = on_ch2 },
};

// Configurar ADC en modo IRQ
hal_adc_config_t cfg = { ..., .use_irq = HAL_ADC_MODE_IRQ };
HAL_ADC_Init(&cfg);
HAL_IRQ_ENABLE();

// Iniciar scan — la ISR encadena las conversiones automáticamente
HAL_ADC_ScanStart(canales, 3U);

while (1) {
    // el CPU está libre — los callbacks se llaman solos
}
```

---

## ⏱️ Módulo Timer

[↑ Volver al inicio](#-tabla-de-contenido)

### ¿Qué es un timer en AVR?

Un **timer** es un contador de hardware que se incrementa automáticamente con cada pulso del reloj del sistema (o una fracción de él), **sin intervenir al CPU**. Cuando llega a cierto valor, puede generar eventos: interrupciones, cambios en pines de salida, o capturar el tiempo en que ocurrió un evento externo.

Los timers resuelven el problema más fundamental de los sistemas embebidos: **medir y controlar el tiempo sin bloquear el CPU**.

```
Con _delay_ms():   CPU bloqueado ══════════════════════ (no puede hacer nada)
Con timer:         CPU libre ─────────────────────────── timer cuenta en paralelo
                                                   ↑
                                            IRQ al llegar al valor
```

### Timers disponibles

| Timer | Bits | Rango | Targets | Pines PWM |
|:---|:---:|:---:|:---:|:---|
| Timer0 | 8 | 0..255 | Ambos | OC0A (PD6), OC0B (PD5) |
| Timer1 | 16 | 0..65535 | Ambos | OC1A (PB1), OC1B (PB2) |
| Timer2 | 8 | 0..255 | Ambos | OC2A (PB3), OC2B (PD3) |
| Timer3 | 16 | 0..65535 | **Solo 1284P** | OC3A, OC3B |

### Registros del timer en AVR

| Registro | Función |
|:---|:---|
| `TCCRnA` | Bits `WGMn1:0` (modo, parte baja); bits `COMnx1:0` (comportamiento del pin OC) |
| `TCCRnB` | Bits `WGMn2` (modo, parte alta); bits `CSn2:0` (prescaler) |
| `TCNTn` | **Contador actual.** Lectura/escritura. Incrementa con cada tick |
| `OCRnA / OCRnB` | **Registros de comparación.** Cuando `TCNTn == OCRn` → evento |
| `ICRn` | Input Capture Register (solo 16 bits). También se usa como TOP en Fast PWM |
| `TIMSKn` | Máscara de interrupciones: `TOIEn` (overflow), `OCIEnA/B` (compare) |
| `TIFRn` | Flags de eventos pendientes |

### Modos de operación

#### Normal / Overflow

El contador cuenta de `0` hasta el máximo (`255` u `65535`) y desborda de vuelta a `0`. Al desbordar genera una IRQ de overflow.

```
TCNTn: 0 ──────────────────────→ 255 → 0 ──→ 255 → 0 ...
                                   ↑               ↑
                              IRQ overflow    IRQ overflow
```

**Cuándo usarlo:** tareas periódicas simples, medir tiempo con resolución fija.

#### CTC (Clear Timer on Compare Match)

El contador cuenta de `0` hasta `OCRnA` (valor configurable) y vuelve a `0`. Genera IRQ en cada comparación.

```
TCNTn: 0 ──────────────→ OCRnA → 0 ──────────────→ OCRnA → 0 ...
                           ↑                          ↑
                        IRQ + reset               IRQ + reset
```

**Período exacto:** `T = (OCRnA + 1) × prescaler / F_CPU`

**Cuándo usarlo:** generar interrupciones a frecuencias exactas, base de tiempo de milisegundos.

**Macro de cálculo:**

```c
// Calcular OCR para generar una IRQ a 1 kHz con prescaler 64 y F_CPU = 16 MHz:
cfg.top = HAL_TIMER_CTC_TOP(1000UL, 64UL);
// HAL_TIMER_CTC_TOP(f, p) = (F_CPU / (2 × p × f)) − 1 = 124
```

#### Fast PWM

El contador cuenta de `0` hasta `TOP` y vuelve a `0`. El pin OC se pone en `1` al reiniciarse el contador y en `0` al llegar a `OCRn`.

```
TCNTn: 0 ──────→ TOP → 0 ──────→ TOP → 0 ...
Pin OC:  ▔▔▔▔▔▔|_____|▔▔▔▔▔▔▔▔|_____|
                ↑ OCR    ↑ OCR
                duty cycle = OCR/TOP × 100%
```

**Frecuencia de PWM:** `f_PWM = F_CPU / (prescaler × (TOP + 1))`

**Cuándo usarlo:** control de brillo de LEDs, velocidad de motores DC, frecuencia fija con duty variable.

#### Phase Correct PWM

El contador cuenta de `0` a `TOP` y luego de `TOP` a `0` (zig-zag). El pin OC cambia al pasar por `OCRn` en ambas direcciones.

```
TCNTn: 0 ──→ TOP ──→ 0 ──→ TOP ──→ 0
Pin OC: ▔▔▔|_______|▔▔▔▔▔▔▔|_______|
```

Frecuencia mitad que Fast PWM, pero la señal es **simétrica respecto al centro**.

**Cuándo usarlo:** control de servos, inversores y aplicaciones donde la simetría de la señal importa.

### El prescaler del timer

El prescaler divide `F_CPU` para que el timer cuente más lento y pueda medir períodos más largos:

```
Para F_CPU = 16 MHz:
Prescaler   1  → tick cada     62.5 ns  (período máx Timer0: 16 µs)
Prescaler   8  → tick cada    500 ns
Prescaler  64  → tick cada      4 µs    (el más común para PWM)
Prescaler 256  → tick cada     16 µs
Prescaler 1024 → tick cada     64 µs    (períodos de hasta ~16 ms en Timer0)
```

> **Nota:** Timer2 tiene prescalers adicionales: `/32` y `/128`, exclusivos de ese timer.

### Configuración

```c
hal_timer_config_t cfg = {
    .mode      = HAL_TIMER_MODE_CTC,         // modo de operación
    .prescaler = HAL_TIMER_PRESCALER_64,     // divisor del reloj
    .top       = HAL_TIMER_CTC_TOP(1000UL, 64UL)  // IRQ a 1 kHz
};
```

### API pública — control básico

| Función | Descripción |
|:---|:---|
| `HAL_Timer_Init(id, &config)` | Configura modo y TOP. **No inicia el timer** |
| `HAL_Timer_Start(id)` | Aplica el prescaler — el contador comienza a correr |
| `HAL_Timer_Stop(id)` | Prescaler = 0 — el contador se congela |
| `HAL_Timer_Reset(id)` | Pone `TCNTn = 0` |

**¿Por qué Init y Start son funciones separadas?**

Para poder registrar callbacks y habilitar IRQ antes de que el timer empiece a correr, evitando que una ISR se dispare antes de que el sistema esté listo.

```c
HAL_Timer_Init(HAL_TIMER0, &cfg);                                    // 1. configurar
HAL_Timer_RegisterCallback(HAL_TIMER0, HAL_TIMER_IRQ_COMPARE_A, f); // 2. registrar callback
HAL_Timer_EnableIRQ(HAL_TIMER0, HAL_TIMER_IRQ_COMPARE_A);           // 3. habilitar IRQ
HAL_IRQ_ENABLE();                                                     // 4. IRQ globales
HAL_Timer_Start(HAL_TIMER0);                                          // 5. ¡arrancar!
```

### API pública — interrupciones y callbacks

| Función | Descripción |
|:---|:---|
| `HAL_Timer_EnableIRQ(id, src)` | Habilita `OVERFLOW`, `COMPARE_A` o `COMPARE_B` |
| `HAL_Timer_DisableIRQ(id, src)` | Deshabilita la fuente de IRQ |
| `HAL_Timer_RegisterCallback(id, src, callback)` | Asocia `void callback(void)` al evento |

### API pública — PWM

| Función | Descripción |
|:---|:---|
| `HAL_Timer_PWM_Enable(id, channel)` | Conecta el pin OC al timer (modo no-inversor) |
| `HAL_Timer_PWM_Disable(id, channel)` | Desconecta el pin OC — queda como GPIO normal |
| `HAL_Timer_PWM_SetDuty(id, channel, duty)` | Duty cycle en porcentaje (0..100) |
| `HAL_Timer_PWM_SetRaw(id, channel, value)` | Valor OCR directo — máxima precisión |

> ⚠️ El pin OC debe configurarse como OUTPUT en GPIO **antes** de habilitar el PWM.

### API pública — timestamps (millis/micros)

| Función | Descripción |
|:---|:---|
| `HAL_Timer_Timestamp_Init()` | Configura Timer1 como base de tiempo de 1 ms. Toma posesión de Timer1 |
| `HAL_Timer_Millis()` | Retorna ms transcurridos. `uint32_t` — desborda cada ~49 días |
| `HAL_Timer_Micros()` | Retorna µs transcurridos |
| `HAL_Timer_DelayMs(ms)` | Espera N ms (las interrupciones siguen activas durante la espera) |

### Macros de cálculo

```c
// Valor OCR para CTC a una frecuencia dada
HAL_TIMER_CTC_TOP(freq_hz, prescaler_val)
// → (F_CPU / (2 × prescaler × freq)) − 1

// Valor ICR (TOP) para Fast PWM de 16 bits a una frecuencia dada
HAL_TIMER_PWM_TOP(freq_hz, prescaler_val)
// → (F_CPU / (prescaler × freq)) − 1

// Valor OCR para un duty cycle en % (0..100)
HAL_TIMER_DUTY(top, percent)
// → (top × percent) / 100

// Convertir microsegundos a ticks del timer
HAL_TIMER_US_TO_TICKS(us, prescaler_val)
```

### Ejemplos de uso

**Tiempo no bloqueante con millis:**

```c
#include "timer.h"

HAL_Timer_Timestamp_Init();
sei();

uint32_t t_led = 0, t_uart = 0;

while (1) {
    uint32_t ahora = HAL_Timer_Millis();

    if (ahora - t_led >= 300UL) {
        GPIO_TOGGLE_PORTB(5);        // LED parpadea cada 300 ms
        t_led = ahora;
    }
    if (ahora - t_uart >= 1000UL) {
        printf("Uptime: %lu s\n", ahora / 1000UL);   // UART cada 1 s
        t_uart = ahora;
    }
    // el CPU nunca se bloquea — todo coexiste
}
```

**PWM para servo (50 Hz, pulso 1–2 ms):**

```c
// Servo estándar: 50 Hz (período = 20 ms)
// Pulso 1 ms = 0°,  1.5 ms = 90°,  2 ms = 180°
hal_timer_config_t servo_cfg = {
    .mode      = HAL_TIMER_MODE_FAST_PWM,
    .prescaler = HAL_TIMER_PRESCALER_8,
    .top       = HAL_TIMER_PWM_TOP(50UL, 8UL)   // TOP = 39999 → 50 Hz
};

GPIO_PIN_MODE_PORTB(1, OUTPUT);   // OC1A = PB1 → debe ser OUTPUT
HAL_Timer_Init(HAL_TIMER1, &servo_cfg);
HAL_Timer_PWM_Enable(HAL_TIMER1, HAL_TIMER_CH_A);

// Con TOP=39999, F_CPU=16MHz, prescaler=8: 1 tick = 0.5 µs
// 1.5 ms = 3000 ticks = 3000/39999 ≈ 7.5% duty
HAL_Timer_PWM_SetRaw(HAL_TIMER1, HAL_TIMER_CH_A, 3000U);

HAL_Timer_Start(HAL_TIMER1);
```

**Tono variable con CTC (buzzer):**

```c
// Generar nota LA (440 Hz) en OC0A (PD6)
hal_timer_config_t tono_cfg = {
    .mode      = HAL_TIMER_MODE_CTC,
    .prescaler = HAL_TIMER_PRESCALER_64,
    .top       = HAL_TIMER_CTC_TOP(440UL, 64UL)   // TOP = 283
};

GPIO_PIN_MODE_PORTD(6, OUTPUT);
HAL_Timer_Init(HAL_TIMER0, &tono_cfg);
HAL_Timer_PWM_Enable(HAL_TIMER0, HAL_TIMER_CH_A);   // toggle en cada comparación
HAL_Timer_Start(HAL_TIMER0);
```

---

---

# Referencia

---

## 📋 Convenciones de nombres

[↑ Volver al inicio](#-tabla-de-contenido)

| Elemento | Convención | Ejemplo |
|:---|:---|:---|
| Funciones públicas | `HAL_Módulo_Acción()` | `HAL_UART_SendString()` |
| Macros de pin | `GPIO_ACCION_PORTx(pin, ...)` | `GPIO_WRITE_PORTB(5, HIGH)` |
| Macros de puerto completo | `GPIO_PORT_ACCION_x(val)` | `GPIO_PORT_WRITE_D(0xFF)` |
| Tipos (typedefs) | `hal_módulo_tipo_t` | `hal_uart_config_t` |
| Enumeraciones | `HAL_MÓDULO_VALOR` | `HAL_ADC_REF_AVCC` |
| Archivos cabecera | `módulo.h` | `uart.h` |
| Archivos implementación | `módulo.c` | `uart.c` |
| Constantes de nivel | `HIGH` / `LOW` | — |
| Constantes de dirección | `INPUT` / `OUTPUT` | — |
| Modos de operación | `HAL_MÓDULO_MODE_MODO` | `HAL_UART_MODE_IRQ` |
| Callbacks | `hal_módulo_callback_t` | `hal_timer_callback_t` |

---

## 🔍 Diferencias entre ATmega328P y ATmega1284P

[↑ Volver al inicio](#-tabla-de-contenido)

| Periférico / Característica | ATmega328P | ATmega1284P | Manejado por |
|:---|:---:|:---:|:---|
| Flash | 32 KB | 128 KB | — |
| SRAM | 2 KB | 16 KB | — |
| Puerto A (8 pines) | ❌ | ✅ | `#if HAL_TARGET_1284P` en `gpio.c` |
| UART1 | ❌ | ✅ | `#if HAL_TARGET_1284P` en `uart.h/c` |
| Timer3 (16 bits) | ❌ | ✅ | `#if HAL_TARGET_1284P` en `timer.h/c` |
| INT2 | ❌ | ✅ | `#if HAL_TARGET_1284P` en `interrupt.h/c` |
| PCINT Bank3 (Puerto A) | ❌ | ✅ | `#if HAL_TARGET_1284P` en `interrupt.h/c` |
| Nombres registros USART | Sin sufijo (`UDR`) | Con sufijo (`UDR0`) | Resuelto por avr-libc |

---

## ✅ Buenas prácticas

[↑ Volver al inicio](#-tabla-de-contenido)

### 1. Variables de tiempo — usar `uint32_t`, no `int`

```c
// ❌ MAL: int desborda y produce valores negativos
int t = HAL_Timer_Millis();

// ✅ BIEN: uint32_t cuenta hasta ~49 días sin desbordar
uint32_t t = HAL_Timer_Millis();
```

### 2. Variables compartidas con ISR — siempre `volatile`

```c
// Sin volatile, el compilador puede optimizarla y no leer el valor actualizado
volatile uint16_t contador_irq = 0U;
volatile uint8_t  flag_evento  = 0U;
```

### 3. ISR mínimas — activar bandera, procesar afuera

```c
volatile uint8_t flag = 0U;

static void on_evento(void) {
    flag = 1U;   // solo esto — corto y sale
}

// En el bucle principal:
if (flag) {
    flag = 0U;
    hacer_trabajo_real();   // la lógica va aquí
}
```

### 4. Inicializar periféricos antes de usarlos

```c
// ✅ Secuencia correcta
HAL_ADC_Init(&cfg_adc);
HAL_UART_Init(HAL_UART0, &cfg_uart);
HAL_Timer_Timestamp_Init();
HAL_IRQ_ENABLE();   // sei() siempre al final de la inicialización
```

### 5. Verificar el retorno de las funciones HAL

```c
int8_t res = HAL_GPIO_ConfigPin('Z', 5, OUTPUT);   // 'Z' no existe
if (res != 0) {
    // manejar el error — sin esto, el bug pasa silencioso
}
```

### 6. Evitar `float` — usar aritmética entera

```c
// ❌ LENTO: operaciones flotantes sin FPU hardware
float voltaje = (raw / 1023.0f) * 5.0f;

// ✅ RÁPIDO: enteros — trabajar en mV directamente
uint16_t mv = (uint16_t)(((uint32_t)raw * 5000UL) / 1023UL);
```

### 7. Comentar el "por qué", no el "qué"

```c
// ❌ MAL: describe lo obvio
TCCR0B = (1 << CS01) | (1 << CS00);   // escribe bits en TCCR0B

// ✅ BIEN: explica la razón
TCCR0B = (1 << CS01) | (1 << CS00);   // Prescaler = 64 → 16MHz/64 = 250kHz → tick = 4µs
```

### 8. Timer Timestamps — no mezclar con PWM en Timer1

```c
// HAL_Timer_Timestamp_Init() toma posesión de Timer1.
// Si necesitas PWM en OC1A/OC1B Y timestamps, debes elegir:
// - Usar Timer0 o Timer2 para el PWM
// - Implementar el timestamp en otro timer manualmente
```

---

## 🎓 Ruta de aprendizaje sugerida

[↑ Volver al inicio](#-tabla-de-contenido)

### Semana 1 — GPIO
- LED parpadeante con macros y `_delay_ms()`
- Lectura de botón con pull-up activo
- Comparar macros (`GPIO_WRITE_PORTB`) vs. funciones (`HAL_GPIO_WritePin`)

### Semana 2 — UART
- Inicializar UART y usar `printf` por monitor serial
- Eco de caracteres (enviar y recibir)
- Comparar modo polling vs. modo IRQ con buffer

### Semana 3 — ADC
- Leer potenciómetro en cuentas y en mV
- Promediar 8 y 16 muestras, comparar con 1 muestra
- Mapear lectura a unidades físicas (ángulo, temperatura, %)

### Semana 4 — Timer y tiempo no bloqueante
- `HAL_Timer_Timestamp_Init()` + `HAL_Timer_Millis()`
- Múltiples eventos con diferente frecuencia en el mismo `while(1)`
- Comparar experiencia de usuario con y sin `_delay_ms()` bloqueante

### Semana 5 — Interrupciones
- Botón con INTx y callback (sin polling)
- PCINT para detectar varios botones con un solo vector
- Sección crítica con variable compartida de 16 bits

### Semana 6 — PWM
- Control de brillo de LED (potenciómetro → ADC → duty cycle PWM)
- Control de posición de servo (potenciómetro → ángulo → pulso)
- Generación de tono musical con CTC en Timer0

### Semana 7 — Proyecto integrador

**Sistema de monitoreo ambiental con alertas:**

- Lee temperatura cada 500 ms (NTC en ADC con promedio de 8 muestras)
- Envía temperatura por UART cada segundo usando `printf`
- LED verde parpadea lento (sistema OK), LED rojo parpadea rápido (alerta)
- Botón con INT0 actualiza el umbral de alerta al valor actual
- Todo el tiempo con `HAL_Timer_Millis()` — cero `_delay_ms()`

### Semana 8 — Portabilidad

- Migrar el proyecto de la Semana 7 a ATmega1284P (solo cambiar `-mmcu`)
- Agregar UART1 para enviar datos a un segundo dispositivo en paralelo
- Explorar Puerto A y Timer3, exclusivos del 1284P

---

> *"Una buena capa de abstracción no esconde el hardware — te ayuda a entenderlo mejor."*

---

**Proyecto académico — UCEVA · UNIVALLE sede Tuluá · Centro STEAM+ UCEVA**
