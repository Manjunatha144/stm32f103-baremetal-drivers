#include "systick.h"

volatile uint8_t g_imu_ready = 0;
static volatile uint32_t g_ms_counter = 0;

void SysTick_Handler(void) {
    g_ms_counter++;
}

void systick_init(void) {
    /* 72,000 cycles = 1 ms tick @ 72 MHz */
    SYSTICK->LOAD = 72000 - 1;
    SYSTICK->VAL  = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_ENABLE |
                    SYSTICK_CTRL_TICKINT |
                    SYSTICK_CTRL_CLKSOURCE;
}

uint32_t systick_get_tick(void) {
    return g_ms_counter;
}

void systick_delay_ms(uint32_t ms) {
    uint32_t start = g_ms_counter;
    while ((g_ms_counter - start) < ms);
}
