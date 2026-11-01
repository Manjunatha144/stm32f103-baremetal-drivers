#include "usart.h"
#include <stdint.h>

#define RCC_BASE      0x40021000UL
#define GPIOC_BASE    0x40011000UL

#define RCC_APB2ENR   (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define GPIOC_CRH     (*(volatile uint32_t *)(GPIOC_BASE + 0x04))
#define GPIOC_ODR     (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))

void delay_ms(volatile uint32_t count) {
    for (volatile uint32_t i = 0; i < count * 800; i++) {
        __asm__("nop");
    }
}

int main(void) {
    // 1. Enable GPIOC clock (bit 4) for PC13 onboard LED
    RCC_APB2ENR |= (1UL << 4);

    // 2. Configure PC13 as Output Push-Pull (2MHz)
    GPIOC_CRH &= ~(0xFUL << 20);
    GPIOC_CRH |=  (0x2UL << 20);

    // 3. Initialize USART1 from usart.c
    usart1_init();

    while (1) {
        // Toggle PC13 LED
        GPIOC_ODR ^= (1UL << 13);

        // Stream serial message using usart.c
        usart1_print("MODULAR TEST 123!\r\n");

        delay_ms(500);
    }
}
