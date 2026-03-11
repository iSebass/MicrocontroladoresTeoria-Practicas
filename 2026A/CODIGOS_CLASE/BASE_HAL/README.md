# Mini HAL GPIO para AVR

Implementación de una **Mini HAL (Hardware Abstraction Layer)** para el manejo de pines GPIO en microcontroladores AVR.

Esta librería busca **simplificar la manipulación de registros** sin ocultar el funcionamiento real del hardware, permitiendo que los estudiantes comprendan cómo interactuar directamente con los periféricos del microcontrolador.

---

# Características

- Configuración de pines como **entrada o salida**
- Escritura de niveles lógicos **HIGH / LOW**
- Lectura del estado de un pin
- Manipulación de **nibbles altos y bajos**
- Acceso directo a registros
- Código simple y didáctico para aprendizaje de sistemas embebidos

---

# Archivo principal

```
gpio.h
```

Este archivo contiene las macros necesarias para manipular los pines GPIO.

---

# Definiciones básicas

```c
#define HIGH    1U
#define LOW     0U

#define INPUT   0U
#define OUTPUT  1U
```

Estas constantes permiten escribir código más legible.

### Ejemplo

```c
GPIO_PIN_MODE(DDRB, PB0, OUTPUT);
```

En lugar de escribir:

```c
DDRB |= (1 << PB0);
```

---

# Configuración del modo del pin

Permite configurar un pin como **entrada o salida**.

```c
#define GPIO_PIN_MODE(ddr, pin, mode) \
    do { \
        if ((mode) == OUTPUT) \
            (ddr) |= (1U << (pin)); \
        else \
            (ddr) &= ~(1U << (pin)); \
    } while(0)
```

### Ejemplo

```c
GPIO_PIN_MODE(DDRB, PB0, OUTPUT);
GPIO_PIN_MODE(DDRB, PB1, INPUT);
```

---

# Escritura de nivel lógico

Permite colocar un pin en estado **HIGH o LOW**.

```c
#define GPIO_PIN_WRITE(port, pin, valuee) \
    do { \
        if ((valuee) == HIGH) \
            (port) |= (1U << (pin)); \
        else \
            (port) &= ~(1U << (pin)); \
    } while(0)
```

### Ejemplo

```c
GPIO_PIN_WRITE(PORTB, PB0, HIGH);
GPIO_PIN_WRITE(PORTB, PB0, LOW);
```

---

# Lectura del estado de un pin

Permite verificar el estado de un pin configurado como entrada.

```c
#define GPIO_PIN_READ(pinreg, pin) \
    ((pinreg) & (1U << (pin)))
```

### Ejemplo

```c
if(GPIO_PIN_READ(PINB, PB1))
{
    // botón presionado
}
```

---

# Escritura del nibble bajo

Permite modificar únicamente los **4 bits menos significativos** de un puerto.

```c
#define GPIO_WRITE_LOW_NIBBLE(port, valuee) \
    (port) = ((port & 0xF0) | (valuee & 0x0F))
```

### Ejemplo

```c
GPIO_WRITE_LOW_NIBBLE(PORTD, 0x05);
```

Resultado esperado:

```
PORTD = XXXX0101
```

---

# Escritura del nibble alto

Permite modificar únicamente los **4 bits más significativos** de un puerto.

```c
#define GPIO_WRITE_HIGH_NIBBLE(port, valuee) \
    (port) = ((port & 0x0F) | ((valuee & 0x0F) << 4))
```

### Ejemplo

```c
GPIO_WRITE_HIGH_NIBBLE(PORTD, 0x0A);
```

Resultado esperado:

```
PORTD = 1010XXXX
```

---

# Lectura del nibble bajo

```c
#define GPIO_READ_LOW_NIBBLE(pinreg) \
    ((pinreg) & 0x0F)
```

### Ejemplo

```c
uint8_t data = GPIO_READ_LOW_NIBBLE(PINC);
```

---

# Lectura del nibble alto

```c
#define GPIO_READ_HIGH_NIBBLE(pinreg) \
    (((pinreg) >> 4) & 0x0F)
```

### Ejemplo

```c
uint8_t data = GPIO_READ_HIGH_NIBBLE(PINC);
```

---

# Ejemplo completo

Encender un LED utilizando un botón.

```c
#include "gpio.h"

int main(void)
{
    GPIO_PIN_MODE(DDRB, PB0, OUTPUT);
    GPIO_PIN_MODE(DDRB, PB1, INPUT);

    while(1)
    {
        if(GPIO_PIN_READ(PINB, PB1))
        {
            GPIO_PIN_WRITE(PORTB, PB0, HIGH);
        }
        else
        {
            GPIO_PIN_WRITE(PORTB, PB0, LOW);
        }
    }
}
```

---

# ¿Por qué usar `do { } while(0)` en macros?

Muchas macros utilizan la estructura:

```c
do {
   ...
} while(0)
```

Esto permite que la macro se comporte como **una única instrucción**, evitando problemas cuando se usa dentro de estructuras como `if`.

Ejemplo correcto:

```c
if(x)
    GPIO_PIN_WRITE(PORTB, PB0, HIGH);
else
    GPIO_PIN_WRITE(PORTB, PB0, LOW);
```

Sin esta estructura la macro podría romper la lógica del programa.

Es una técnica estándar en C para escribir **macros seguras**.

---

# Ventajas pedagógicas

Esta mini HAL permite:

- Comprender la manipulación de **registros en microcontroladores**
- Escribir código **más limpio y legible**
- Facilitar el aprendizaje de **operaciones a nivel de bit**
- Preparar a los estudiantes para **HALs más complejas**

---

# Autor

Juan Sebastian Correa  
Ingeniero Electrónico  
Curso: Sistemas Embebidos