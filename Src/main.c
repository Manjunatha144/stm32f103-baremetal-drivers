#include "gpio.h"

/* Simple software delay loop */
static void delay(volatile uint32_t count) {
    while (count--) {
        __asm volatile ("nop");
    }
}

int main(void) {
    /* 1. Enable peripheral clock for Port C */
    gpio_clock_enable(GPIOC);

    /* 2. Configure PC13 as Output Push-Pull, 2 MHz */
    gpio_init(GPIOC, GPIO_PIN_13, GPIO_MODE_OUTPUT_PP_2MHZ);

    /* 3. Blink loop using the new driver APIs */
    while (1) {
        // Active-low: Pull LOW to turn LED ON
        gpio_write_pin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        delay(30000);

        // Pull HIGH to turn LED OFF
        gpio_write_pin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        delay(600000);
    }
}
