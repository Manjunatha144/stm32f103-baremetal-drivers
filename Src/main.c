#include <stdint.h>

/* --- Hardware Register Definitions (Pure Bare-Metal) --- */
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
} RCC_TypeDef;

typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_TypeDef;

#define RCC   ((RCC_TypeDef *)0x40021000UL)
#define GPIOC ((GPIO_TypeDef *)0x40011000UL)

/* Simple software delay using inline assembly NOP */
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm volatile ("nop");
    }
}

int main(void) {
    /* 1. Enable peripheral clock for GPIOC (Bit 4 of RCC_APB2ENR) */
    RCC->APB2ENR |= (1UL << 4);

    /* 2. Configure PC13 as Output Push-Pull, 2 MHz
     *    CRH bits [23:20]: MODE13 = 10 (2MHz), CNF13 = 00 (Push-pull) -> 0x2
     */
    GPIOC->CRH &= ~(0xFUL << 20);
    GPIOC->CRH |=  (0x2UL << 20);

    /* 3. Fast Blink Loop */
    while (1) {
        // Active-low: Pull LOW to turn LED ON
        GPIOC->BRR = (1UL << 13);
        delay(150000);  // Fast blink interval

        // Pull HIGH to turn LED OFF
        GPIOC->BSRR = (1UL << 13);
        delay(150000);  // Fast blink interval
    }
}
