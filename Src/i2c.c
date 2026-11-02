#include "i2c.h"

/* Clock enable registers */
#define RCC_APB1ENR       (*(volatile uint32_t *)0x4002101C)
#define RCC_APB2ENR       (*(volatile uint32_t *)0x40021018)
#define RCC_AHBENR        (*(volatile uint32_t *)0x40021014)

/* GPIOB registers */
#define GPIOB_CRL         (*(volatile uint32_t *)0x40010C00)

/* I2C1 registers */
#define I2C1_BASE         0x40005400
#define I2C1_CR1          (*(volatile uint32_t *)(I2C1_BASE + 0x00))
#define I2C1_CR2          (*(volatile uint32_t *)(I2C1_BASE + 0x04))
#define I2C1_OAR1         (*(volatile uint32_t *)(I2C1_BASE + 0x08))
#define I2C1_DR           (*(volatile uint32_t *)(I2C1_BASE + 0x10))
#define I2C1_SR1          (*(volatile uint32_t *)(I2C1_BASE + 0x14))
#define I2C1_SR2          (*(volatile uint32_t *)(I2C1_BASE + 0x18))
#define I2C1_CCR          (*(volatile uint32_t *)(I2C1_BASE + 0x1C))
#define I2C1_TRISE        (*(volatile uint32_t *)(I2C1_BASE + 0x20))

/* DMA1 Channel 7 (I2C1_RX) */
#define DMA1_CH7_BASE     0x40020078
#define DMA1_CCR7         (*(volatile uint32_t *)(DMA1_CH7_BASE + 0x00))
#define DMA1_CNDTR7       (*(volatile uint32_t *)(DMA1_CH7_BASE + 0x04))
#define DMA1_CPAR7        (*(volatile uint32_t *)(DMA1_CH7_BASE + 0x08))
#define DMA1_CMAR7        (*(volatile uint32_t *)(DMA1_CH7_BASE + 0x0C))
#define DMA1_IFCR         (*(volatile uint32_t *)0x40020004)
#define DMA1_ISR          (*(volatile uint32_t *)0x40020000)

/* NVIC ISER0 */
#define NVIC_ISER0        (*(volatile uint32_t *)0xE000E100)

volatile uint8_t g_i2c_rx_complete = 0;

void i2c1_init(void) {
    /* 1. Enable GPIOB and I2C1 peripheral clocks */
    RCC_APB2ENR |= (1 << 3);  // IOPBEN
    RCC_APB1ENR |= (1 << 21); // I2C1EN

    /* 2. Configure PB6 (SCL) and PB7 (SDA) as Alternate Function Open-Drain 10MHz (0x0E) */
    GPIOB_CRL &= ~(0xFF << 24);
    GPIOB_CRL |=  (0xEE << 24);

    /* 3. Reset and release I2C1 */
    I2C1_CR1 |= (1 << 15);
    I2C1_CR1 &= ~(1 << 15);

    /* 4. Program peripheral clock frequency in CR1/CR2 (PCLK1 = 36 MHz) */
    I2C1_CR2 = 36;

    /* 5. Configure clock control registers for 100 kHz standard mode */
    /* Thigh = Tlow = 5000 ns -> CCR = 36MHz * 5000ns = 180 */
    I2C1_CCR = 180;

    /* 6. Configure rise time: (1000ns / 27.7ns) + 1 = 37 */
    I2C1_TRISE = 37;

    /* 7. Enable peripheral and acknowledge */
    I2C1_CR1 |= (1 << 10); // ACK
    I2C1_CR1 |= (1 << 0);  // PE
}

void i2c1_dma_init(void) {
    /* Enable DMA1 clock (bit 0 in AHBENR) */
    RCC_AHBENR |= (1 << 0);

    /* Configure DMA1 Channel 7 (I2C1_RX) */
    DMA1_CCR7 = 0; // Disable channel first
    DMA1_CPAR7 = (uint32_t)&I2C1_DR;

    /*
     * Priority = Very High (bits [13:12] = 11)
     * Memory increment mode (bit 7 = 1)
     * Peripheral size = 8-bit (bits [9:8] = 00)
     * Memory size = 8-bit (bits [11:10] = 00)
     * Transfer complete interrupt enable (bit 1 = 1)
     */
    DMA1_CCR7 = (0x3 << 12) | (1 << 7) | (1 << 1);

    /* Enable DMA1 Channel 7 IRQ (IRQ 17) in NVIC ISER0 */
    NVIC_ISER0 = (1 << 17);
}

void DMA1_Channel7_IRQHandler(void) {
    if (DMA1_ISR & (1 << 25)) { // TCIF7 bit
        DMA1_IFCR = (1 << 25);   // Clear transfer complete flag
        DMA1_CCR7 &= ~(1 << 0);  // Disable DMA channel

        /* Send I2C STOP condition */
        I2C1_CR1 |= (1 << 9);

        /* Disable I2C DMA request mode and ACK generation */
        I2C1_CR2 &= ~(1 << 11);
        I2C1_CR1 &= ~(1 << 10);

        g_i2c_rx_complete = 1;
    }
}

void i2c1_write_register(uint8_t dev_addr, uint8_t reg_addr, uint8_t data) {
    /* Send START condition */
    I2C1_CR1 |= (1 << 8);
    while (!(I2C1_SR1 & (1 << 0)));

    /* Send device write address */
    I2C1_DR = (dev_addr << 1);
    while (!(I2C1_SR1 & (1 << 1)));
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    /* Send target register address */
    I2C1_DR = reg_addr;
    while (!(I2C1_SR1 & (1 << 2)));

    /* Send data byte */
    I2C1_DR = data;
    while (!(I2C1_SR1 & (1 << 2)));

    /* Send STOP condition */
    I2C1_CR1 |= (1 << 9);
}

uint8_t i2c1_read_register(uint8_t dev_addr, uint8_t reg_addr) {
    uint8_t data = 0;

    /* Send START */
    I2C1_CR1 |= (1 << 8);
    while (!(I2C1_SR1 & (1 << 0)));

    /* Send device write address */
    I2C1_DR = (dev_addr << 1);
    while (!(I2C1_SR1 & (1 << 1)));
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    /* Send register address */
    I2C1_DR = reg_addr;
    while (!(I2C1_SR1 & (1 << 2)));

    /* Repeated START */
    I2C1_CR1 |= (1 << 8);
    while (!(I2C1_SR1 & (1 << 0)));

    /* Send device read address */
    I2C1_DR = (dev_addr << 1) | 0x01;
    while (!(I2C1_SR1 & (1 << 1)));
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    /* Disable ACK and send STOP for 1 byte read */
    I2C1_CR1 &= ~(1 << 10);
    I2C1_CR1 |= (1 << 9);

    /* Wait for byte to arrive */
    while (!(I2C1_SR1 & (1 << 6)));
    data = (uint8_t)I2C1_DR;

    /* Re-enable ACK for future transfers */
    I2C1_CR1 |= (1 << 10);

    return data;
}

void i2c1_read_multibyte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buffer, uint8_t length) {
    /* Send START */
    I2C1_CR1 |= (1 << 8);
    while (!(I2C1_SR1 & (1 << 0)));

    /* Send device write address */
    I2C1_DR = (dev_addr << 1);
    while (!(I2C1_SR1 & (1 << 1)));
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    /* Send register address */
    I2C1_DR = reg_addr;
    while (!(I2C1_SR1 & (1 << 2)));

    /* Repeated START */
    I2C1_CR1 |= (1 << 8);
    while (!(I2C1_SR1 & (1 << 0)));

    /* Send device read address */
    I2C1_DR = (dev_addr << 1) | 0x01;
    while (!(I2C1_SR1 & (1 << 1)));
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    /* Read bytes with polling */
    for (uint8_t i = 0; i < length; i++) {
        if (i == (length - 1)) {
            I2C1_CR1 &= ~(1 << 10); // NACK last byte
            I2C1_CR1 |= (1 << 9);   // Send STOP
        }
        while (!(I2C1_SR1 & (1 << 6))); // Wait for RXNE
        buffer[i] = (uint8_t)I2C1_DR;
    }

    I2C1_CR1 |= (1 << 10); // Re-enable ACK
}

void i2c1_dma_start_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *dst_buffer, uint16_t length) {
    g_i2c_rx_complete = 0;

    /* Setup DMA destination and count */
    DMA1_CCR7 &= ~(1 << 0);
    DMA1_CMAR7 = (uint32_t)dst_buffer;
    DMA1_CNDTR7 = length;

    /* 1. Send START */
    I2C1_CR1 |= (1 << 8);
    while (!(I2C1_SR1 & (1 << 0)));

    /* 2. Send Device Write Address */
    I2C1_DR = (dev_addr << 1);
    while (!(I2C1_SR1 & (1 << 1)));
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    /* 3. Send Register Address */
    I2C1_DR = reg_addr;
    while (!(I2C1_SR1 & (1 << 2)));

    /* 4. Enable I2C DMA Mode and ACK generation */
    I2C1_CR2 |= (1 << 11); // DMAEN bit
    I2C1_CR1 |= (1 << 10); // ACK bit

    /* 5. Enable DMA Channel 7 */
    DMA1_CCR7 |= (1 << 0);

    /* 6. Send Repeated START */
    I2C1_CR1 |= (1 << 8);
    while (!(I2C1_SR1 & (1 << 0)));

    /* 7. Send Device Read Address */
    I2C1_DR = (dev_addr << 1) | 0x01;
    while (!(I2C1_SR1 & (1 << 1)));
    (void)I2C1_SR1;
    (void)I2C1_SR2;
}
