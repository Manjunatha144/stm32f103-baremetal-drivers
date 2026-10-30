#include "gpio.h"
#include "systick.h"

int main(void) {
    /* 1. Initialize SysTick core timer */
    systick_init();

    /* 2. Initialize PC13 as Output Push-Pull */
    gpio_clock_enable(GPIOC);
    gpio_init(GPIOC, GPIO_PIN_13, GPIO_MODE_OUTPUT_PP_2MHZ);

    /* 3. Blink LED with calibrated 1-second delay */
    while (1) {
        gpio_toggle_pin(GPIOC, GPIO_PIN_13);
        systick_delay_ms(1000);  // Exactly 1000 ms
    }
}
