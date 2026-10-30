#include "systick.h"

/**
 * @brief  Initializes the SysTick timer.
 */
void systick_init(void) {
    /* Disable SysTick during configuration */
    SYSTICK->CTRL = 0;

    /* Clear current value register */
    SYSTICK->VAL = 0;
}

/**
 * @brief  Blocking millisecond delay using the Cortex-M3 SysTick timer.
 *         Default HSI clock = 8 MHz (8,000 cycles per millisecond).
 */
void systick_delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        /* Set reload value for 1 ms (8000 ticks - 1) */
        SYSTICK->LOAD = (8000 - 1);

        /* Clear current value register */
        SYSTICK->VAL = 0;

        /* Enable counter and select processor clock (AHB) */
        SYSTICK->CTRL = SYSTICK_CTRL_ENABLE | SYSTICK_CTRL_CLKSOURCE;

        /* Wait until COUNTFLAG is set */
        while (!(SYSTICK->CTRL & SYSTICK_CTRL_COUNTFLAG)) {
            /* Spin wait */
        }
    }

    /* Disable SysTick */
    SYSTICK->CTRL = 0;
}
