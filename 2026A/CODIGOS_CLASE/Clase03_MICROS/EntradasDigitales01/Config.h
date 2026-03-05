#ifndef CONFIG_H_
#define CONFIG_H_

#define F_CPU 16000000UL
#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>

// ============================================================
// BIT MACROS (máscaras y operaciones)
// ============================================================

// En AVR existe _BV(n), pero dejo BIT(n) por legibilidad.
#ifndef BIT
#define BIT(n) (1u << (n))
#endif

#define SET_BIT(reg, mask)    do { (reg) |=  (uint8_t)(mask); } while (0)
#define CLR_BIT(reg, mask)    do { (reg) &= (uint8_t)~(mask); } while (0)
#define TGL_BIT(reg, mask)    do { (reg) ^=  (uint8_t)(mask); } while (0)

#define WRITE_BIT(reg, mask, value) \
do { if (value) SET_BIT((reg), (mask)); else CLR_BIT((reg), (mask)); } while (0)

#define READ_BIT(reg, mask)   (((reg) & (uint8_t)(mask)) ? 1u : 0u)

// ============================================================
// GPIO HELPERS (DDRx, PORTx, PINx)
// ============================================================

// Configuración de dirección
#define PIN_OUTPUT(ddr, bit)  SET_BIT((ddr), BIT(bit))
#define PIN_INPUT(ddr, bit)   CLR_BIT((ddr), BIT(bit))

// Pull-up (solo útil si está como entrada)
#define PIN_PULLUP_ON(port, bit)   SET_BIT((port), BIT(bit))
#define PIN_PULLUP_OFF(port, bit)  CLR_BIT((port), BIT(bit))

// Escritura en salida
#define PIN_HIGH(port, bit)   SET_BIT((port), BIT(bit))
#define PIN_LOW(port, bit)    CLR_BIT((port), BIT(bit))
#define PIN_TOGGLE(port, bit) TGL_BIT((port), BIT(bit))

// Lectura (desde PINx)
#define PIN_READ(pinreg, bit) (((pinreg) & BIT(bit)) ? 1u : 0u)

#endif /* CONFIG_H_ */