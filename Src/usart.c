#include "usart.h"

void usart1_init(void) {
    // 1. Enable GPIOA clock (bit 2) and USART1 clock (bit 14) on APB2
    RCC_APB2ENR |= (1UL << 2) | (1UL << 14);

    // 2. Configure PA9 (TX) as Alternate Function Push-Pull (50MHz) -> 0xB
    GPIOA_CRH &= ~(0xFUL << 4);
    GPIOA_CRH |=  (0xBUL << 4);

    // 3. Configure PA10 (RX) as Input Floating -> 0x4
    GPIOA_CRH &= ~(0xFUL << 8);
    GPIOA_CRH |=  (0x4UL << 8);

    // 4. Baud rate 115200 (at default 8 MHz HSI clock) -> BRR = 0x45
    USART1_BRR = 0x0045;

    // 5. Enable TE (bit 3), RE (bit 2), and UE (bit 13)
    USART1_CR1 |= (1UL << 3) | (1UL << 2) | (1UL << 13);
}

void usart1_write(char ch) {
    // Wait until TXE (bit 7) is set
    while (!(USART1_SR & (1UL << 7)));
    USART1_DR = (ch & 0xFF);
}

void usart1_print(const char *str) {
    while (*str) {
        usart1_write(*str++);
    }
}
