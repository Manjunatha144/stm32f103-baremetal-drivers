#include "usart.h"

/* Peripheral Clock Registers */
#define RCC_APB2ENR   (*(volatile uint32_t *)0x40021018)
#define RCC_AHBENR    (*(volatile uint32_t *)0x40021014)

/* GPIOA Registers */
#define GPIOA_CRH     (*(volatile uint32_t *)0x40010804)
#define GPIOA_ODR     (*(volatile uint32_t *)0x4001080C)

/* USART1 Registers */
#define USART1_BASE   0x40013800
#define USART1_SR     (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR     (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR    (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1    (*(volatile uint32_t *)(USART1_BASE + 0x0C))
#define USART1_CR3    (*(volatile uint32_t *)(USART1_BASE + 0x14))

/* DMA1 Channel 4 Registers (USART1_TX) */
#define DMA1_CH4_BASE 0x40020044
#define DMA1_CCR4     (*(volatile uint32_t *)(DMA1_CH4_BASE + 0x00))
#define DMA1_CNDTR4   (*(volatile uint32_t *)(DMA1_CH4_BASE + 0x04))
#define DMA1_CPAR4    (*(volatile uint32_t *)(DMA1_CH4_BASE + 0x08))
#define DMA1_CMAR4    (*(volatile uint32_t *)(DMA1_CH4_BASE + 0x0C))

/* DMA1 Channel 5 Registers (USART1_RX) */
#define DMA1_CH5_BASE 0x40020058
#define DMA1_CCR5     (*(volatile uint32_t *)(DMA1_CH5_BASE + 0x00))
#define DMA1_CNDTR5   (*(volatile uint32_t *)(DMA1_CH5_BASE + 0x04))
#define DMA1_CPAR5    (*(volatile uint32_t *)(DMA1_CH5_BASE + 0x08))
#define DMA1_CMAR5    (*(volatile uint32_t *)(DMA1_CH5_BASE + 0x0C))

/* DMA Interrupt Status / Clear Registers */
#define DMA1_ISR      (*(volatile uint32_t *)0x40020000)
#define DMA1_IFCR     (*(volatile uint32_t *)0x40020004)

/* Circular RX Ring Buffer Configuration */
#define RX_RING_SIZE  16
static volatile uint8_t s_rx_ring[RX_RING_SIZE];
static uint16_t s_rx_read_idx = 0;

void usart1_init(void) {
    /* 1. Enable GPIOA and USART1 peripheral clocks */
    RCC_APB2ENR |= (1 << 2) | (1 << 14);

    /* 2. Configure PA9 (TX) as Alternate Function Push-Pull 50MHz (0x0B)
     *    Configure PA10 (RX) as Input with Pull-Up (0x08) */
    GPIOA_CRH &= ~(0xFF << 4);
    GPIOA_CRH |=  (0x8B << 4);
    GPIOA_ODR |=  (1 << 10); // Pull-up on PA10

    /* 3. Baud rate 115200 @ 72 MHz APB2:
     *    72,000,000 / (16 * 115200) = 39.0625 -> Mantissa=39 (0x27), Fraction=1 (0x1) -> 0x0271 */
    USART1_BRR = 0x0271;

    /* 4. Enable USART1, Transmitter (TE), and Receiver (RE) */
    USART1_CR1 = (1 << 13) | (1 << 3) | (1 << 2);
}

void usart1_dma_init(void) {
    /* 1. Enable DMA1 clock */
    RCC_AHBENR |= (1 << 0);

    /* 2. Configure DMA1 Channel 4 (USART1_TX) */
    DMA1_CCR4 = 0;
    DMA1_CPAR4 = (uint32_t)&USART1_DR;
    /* Memory increment (bit 7), Memory-to-peripheral (bit 4), Priority medium (bit 12) */
    DMA1_CCR4 = (1 << 7) | (1 << 4) | (1 << 12);

    /* 3. Configure DMA1 Channel 5 (USART1_RX Circular Mode) */
    DMA1_CCR5 = 0;
    DMA1_CPAR5 = (uint32_t)&USART1_DR;
    DMA1_CMAR5 = (uint32_t)s_rx_ring;
    DMA1_CNDTR5 = RX_RING_SIZE;
    /* Memory increment (bit 7), Circular mode (bit 5), Channel enable (bit 0) */
    DMA1_CCR5 = (1 << 7) | (1 << 5) | (1 << 0);

    /* 4. Enable DMA Transmitter (DMAT bit 7) and Receiver (DMAR bit 6) inside USART1 */
    USART1_CR3 |= (1 << 7) | (1 << 6);
}

void usart1_dma_send(const uint8_t *data, uint16_t len) {
    /* Disable channel before re-arming byte count */
    DMA1_CCR4 &= ~(1 << 0);

    /* Clear all transfer interrupt and error flags for Channel 4 (bits 12-15) */
    DMA1_IFCR = (1 << 12) | (1 << 13) | (1 << 14) | (1 << 15);

    /* Set SRAM source address and packet byte count */
    DMA1_CMAR4 = (uint32_t)data;
    DMA1_CNDTR4 = len;

    /* Enable DMA1 Channel 4: Hardware immediately transmits buffer to USART1 */
    DMA1_CCR4 |= (1 << 0);
}

uint8_t usart1_dma_read_cmd(void) {
    uint16_t write_idx = RX_RING_SIZE - (uint16_t)DMA1_CNDTR5;
    if (s_rx_read_idx != write_idx) {
        uint8_t cmd = s_rx_ring[s_rx_read_idx];
        s_rx_read_idx = (s_rx_read_idx + 1) % RX_RING_SIZE;
        return cmd;
    }
    return 0;
}
