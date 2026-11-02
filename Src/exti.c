#include "exti.h"
#include "systick.h"

#define RCC_APB2ENR       (*(volatile uint32_t *)0x40021018)
#define GPIOB_CRL         (*(volatile uint32_t *)0x40010C00)

/* Corrected Register: EXTICR1 is 0x08, EXTICR2 is 0x0C */
#define AFIO_EXTICR2      (*(volatile uint32_t *)0x4001000C)

#define EXTI_BASE         0x40010400
#define EXTI_IMR          (*(volatile uint32_t *)(EXTI_BASE + 0x00))
#define EXTI_RTSR         (*(volatile uint32_t *)(EXTI_BASE + 0x08))
#define EXTI_PR           (*(volatile uint32_t *)(EXTI_BASE + 0x14))

#define NVIC_ISER0        (*(volatile uint32_t *)0xE000E100)

void exti_pb5_init(void) {
    /* 1. Clocks: Enable GPIOB (bit 3) and AFIO (bit 0) */
    RCC_APB2ENR |= (1 << 3) | (1 << 0);

    /* 2. Configure PB5 as Input Floating (CNF = 01, MODE = 00 -> 0x4) */
    GPIOB_CRL &= ~(0xF << 20);
    GPIOB_CRL |=  (0x4 << 20);

    /* 3. Map EXTI5 to Port B (AFIO_EXTICR2 bits [7:4] = 0x1) */
    AFIO_EXTICR2 &= ~(0xF << 4);
    AFIO_EXTICR2 |=  (0x1 << 4);

    /* 4. Clear any stale pending flag on line 5 before enabling */
    EXTI_PR = (1 << 5);

    /* 5. Unmask EXTI5 and configure Rising Edge trigger */
    EXTI_IMR  |= (1 << 5);
    EXTI_RTSR |= (1 << 5);

    /* 6. Enable EXTI9_5 IRQ (Vector 23) in NVIC */
    NVIC_ISER0 = (1 << 23);
}

/* Hardware ISR called the exact microsecond MPU6050 pulls INT high */
void EXTI9_5_IRQHandler(void) {
    if (EXTI_PR & (1 << 5)) {
        EXTI_PR = (1 << 5);   // Clear pending bit by writing 1
        g_imu_ready = 1;      // Wake main loop
    }
}
