#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>

/* --- Peripheral Base Addresses (STM32F103xB Memory Map) --- */
#define PERIPH_BASE       (0x40000000UL)
#define APB2PERIPH_BASE   (PERIPH_BASE + 0x00010000UL)
#define AHBPERIPH_BASE    (PERIPH_BASE + 0x00020000UL)

/* RCC Base and Register Struct */
#define RCC_BASE          (AHBPERIPH_BASE + 0x1000UL)

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} RCC_RegDef_t;

#define RCC               ((RCC_RegDef_t *)RCC_BASE)

/* GPIO Port Base Addresses */
#define GPIOA_BASE        (APB2PERIPH_BASE + 0x0800UL)
#define GPIOB_BASE        (APB2PERIPH_BASE + 0x0C00UL)
#define GPIOC_BASE        (APB2PERIPH_BASE + 0x1000UL)

typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_RegDef_t;

#define GPIOA             ((GPIO_RegDef_t *)GPIOA_BASE)
#define GPIOB             ((GPIO_RegDef_t *)GPIOB_BASE)
#define GPIOC             ((GPIO_RegDef_t *)GPIOC_BASE)

/* Pin Definitions */
#define GPIO_PIN_0        0
#define GPIO_PIN_1        1
#define GPIO_PIN_2        2
#define GPIO_PIN_3        3
#define GPIO_PIN_4        4
#define GPIO_PIN_5        5
#define GPIO_PIN_6        6
#define GPIO_PIN_7        7
#define GPIO_PIN_8        8
#define GPIO_PIN_9        9
#define GPIO_PIN_10       10
#define GPIO_PIN_11       11
#define GPIO_PIN_12       12
#define GPIO_PIN_13       13
#define GPIO_PIN_14       14
#define GPIO_PIN_15       15

/* Pin Logic State */
typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET   = 1
} GPIO_PinState_t;

/* Pin Modes (Combined MODE[1:0] and CNF[1:0] bitmask per pin) */
typedef enum {
    GPIO_MODE_INPUT_ANALOG      = 0x0, // MODE=00, CNF=00
    GPIO_MODE_INPUT_FLOATING    = 0x4, // MODE=00, CNF=01
    GPIO_MODE_INPUT_PULL        = 0x8, // MODE=00, CNF=10
    GPIO_MODE_OUTPUT_PP_2MHZ    = 0x2, // MODE=10, CNF=00
    GPIO_MODE_OUTPUT_PP_10MHZ   = 0x1, // MODE=01, CNF=00
    GPIO_MODE_OUTPUT_PP_50MHZ   = 0x3, // MODE=11, CNF=00
    GPIO_MODE_OUTPUT_OD_2MHZ    = 0x6, // MODE=10, CNF=01
    GPIO_MODE_OUTPUT_OD_10MHZ   = 0x5, // MODE=01, CNF=01
    GPIO_MODE_OUTPUT_OD_50MHZ   = 0x7  // MODE=11, CNF=01
} GPIO_PinMode_t;

/* --- GPIO Driver APIs --- */
void gpio_clock_enable(GPIO_RegDef_t *pGPIOx);
void gpio_init(GPIO_RegDef_t *pGPIOx, uint8_t pin, GPIO_PinMode_t mode);
void gpio_write_pin(GPIO_RegDef_t *pGPIOx, uint8_t pin, GPIO_PinState_t state);
void gpio_toggle_pin(GPIO_RegDef_t *pGPIOx, uint8_t pin);
GPIO_PinState_t gpio_read_pin(GPIO_RegDef_t *pGPIOx, uint8_t pin);

#endif /* GPIO_H_ */
