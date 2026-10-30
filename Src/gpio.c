#include "gpio.h"

/**
 * @brief  Enables the APB2 peripheral clock for the given GPIO port.
 */
void gpio_clock_enable(GPIO_RegDef_t *pGPIOx) {
    if (pGPIOx == GPIOA) {
        RCC->APB2ENR |= (1UL << 2);
    } else if (pGPIOx == GPIOB) {
        RCC->APB2ENR |= (1UL << 3);
    } else if (pGPIOx == GPIOC) {
        RCC->APB2ENR |= (1UL << 4);
    }
}

/**
 * @brief  Configures mode and speed for a specific GPIO pin.
 *         Pins 0-7 use CRL; Pins 8-15 use CRH.
 */
void gpio_init(GPIO_RegDef_t *pGPIOx, uint8_t pin, GPIO_PinMode_t mode) {
    if (pin < 8) {
        uint8_t shift = pin * 4;
        pGPIOx->CRL &= ~(0xFUL << shift);         // Clear 4-bit field (MODE + CNF)
        pGPIOx->CRL |= ((uint32_t)mode << shift);  // Set new configuration
    } else if (pin <= 15) {
        uint8_t shift = (pin - 8) * 4;
        pGPIOx->CRH &= ~(0xFUL << shift);         // Clear 4-bit field (MODE + CNF)
        pGPIOx->CRH |= ((uint32_t)mode << shift);  // Set new configuration
    }
}

/**
 * @brief  Sets or resets a GPIO pin using atomic bit registers.
 */
void gpio_write_pin(GPIO_RegDef_t *pGPIOx, uint8_t pin, GPIO_PinState_t state) {
    if (state == GPIO_PIN_SET) {
        pGPIOx->BSRR = (1UL << pin);       // Atomic bit set
    } else {
        pGPIOx->BRR = (1UL << pin);        // Atomic bit reset
    }
}

/**
 * @brief  Toggles the output state of a GPIO pin.
 */
void gpio_toggle_pin(GPIO_RegDef_t *pGPIOx, uint8_t pin) {
    pGPIOx->ODR ^= (1UL << pin);
}

/**
 * @brief  Reads the logic level of a GPIO input pin.
 */
GPIO_PinState_t gpio_read_pin(GPIO_RegDef_t *pGPIOx, uint8_t pin) {
    uint8_t bit_status = (uint8_t)((pGPIOx->IDR >> pin) & 0x01);
    return (bit_status != 0) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}
