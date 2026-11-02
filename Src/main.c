#include <stdint.h>
#include <math.h>
#include "systick.h"
#include "usart.h"
#include "i2c.h"
#include "madgwick.h"

/* ================= Hardware Register Definitions ================= */
#define FLASH_ACR          (*(volatile uint32_t *)0x40022000)
#define RCC_CR             (*(volatile uint32_t *)0x40021000)
#define RCC_CFGR           (*(volatile uint32_t *)0x40021004)
#define RCC_APB2ENR_REG    (*(volatile uint32_t *)0x40021018)

#define GPIOB_BASE         0x40010C00
#define GPIOB_CRL          (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_ODR          (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))

#define GPIOC_BASE         0x40011000
#define GPIOC_CRH          (*(volatile uint32_t *)(GPIOC_BASE + 0x04))
#define GPIOC_ODR          (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))

/* ================= MPU6050 Registers ================= */
#define MPU6050_SMPLRT_DIV   0x19
#define MPU6050_CONFIG       0x1A
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_WHO_AM_I     0x75

/* ================= System Constants ================= */
#define CALIBRATION_SAMPLES 500
#define DEG_TO_RAD          0.01745329251f
#define GYRO_SCALE          131.0f

typedef struct __attribute__((packed)) {
    uint8_t  header[2];   // 0xAA, 0x55
    int16_t  q0_x10k;     // w * 10,000
    int16_t  q1_x10k;     // x * 10,000
    int16_t  q2_x10k;     // y * 10,000
    int16_t  q3_x10k;     // z * 10,000
    uint8_t  checksum;    // XOR over first 10 bytes
} QuaternionPacket_t;

void rcc_clock_init_72mhz(void) {
    FLASH_ACR &= ~0x7;
    FLASH_ACR |= 0x2;

    RCC_CR |= (1 << 16);
    while (!(RCC_CR & (1 << 17)));

    RCC_CFGR &= ~((0xF << 4) | (0x7 << 8) | (0x7 << 11) | (1 << 16) | (0xF << 18));
    RCC_CFGR |= (0x4 << 8) | (1 << 16) | (0x7 << 18);

    RCC_CR |= (1 << 24);
    while (!(RCC_CR & (1 << 25)));

    RCC_CFGR &= ~0x3;
    RCC_CFGR |= 0x2;
    while (((RCC_CFGR >> 2) & 0x3) != 0x2);
}

void i2c1_bus_recovery(void) {
    RCC_APB2ENR_REG |= (1 << 3);
    GPIOB_CRL &= ~(0xFF << 24);
    GPIOB_CRL |=  (0x66 << 24);
    GPIOB_ODR |=  (1 << 6) | (1 << 7);

    for (int i = 0; i < 9; i++) {
        GPIOB_ODR &= ~(1 << 6);
        for (volatile int d = 0; d < 1000; d++);
        GPIOB_ODR |= (1 << 6);
        for (volatile int d = 0; d < 1000; d++);
    }
}

/* Reusable 500-sample gyro calibration routine */
static void calibrate_gyro(uint8_t addr, int16_t *bgx, int16_t *bgy, int16_t *bgz) {
    int32_t sum_gx = 0, sum_gy = 0, sum_gz = 0;
    uint8_t raw[14];

    for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
        i2c1_read_multibyte(addr, MPU6050_ACCEL_XOUT_H, raw, 14);
        sum_gx += (int16_t)((raw[8]  << 8) | raw[9]);
        sum_gy += (int16_t)((raw[10] << 8) | raw[11]);
        sum_gz += (int16_t)((raw[12] << 8) | raw[13]);

        if (i % 15 == 0) {
            GPIOC_ODR ^= (1 << 13); // Fast blink during calibration
        }
        systick_delay_ms(3);
    }

    *bgx = (int16_t)(sum_gx / CALIBRATION_SAMPLES);
    *bgy = (int16_t)(sum_gy / CALIBRATION_SAMPLES);
    *bgz = (int16_t)(sum_gz / CALIBRATION_SAMPLES);
}

int main(void) {
    rcc_clock_init_72mhz();
    systick_init();

    RCC_APB2ENR_REG |= (1 << 4);
    GPIOC_CRH &= ~(0xF << 20);
    GPIOC_CRH |=  (0x2 << 20);
    GPIOC_ODR &= ~(1 << 13);

    i2c1_bus_recovery();
    usart1_init();
    usart1_dma_init();
    i2c1_init();

    madgwick_init(50.0f, 0.08f);
    systick_delay_ms(150);

    uint8_t active_addr = 0x68;
    i2c1_write_register(0x68, MPU6050_PWR_MGMT_1, 0x01);
    systick_delay_ms(50);

    uint8_t who_am_i = i2c1_read_register(0x68, MPU6050_WHO_AM_I);
    if (who_am_i != 0x68 && who_am_i != 0x70 && who_am_i != 0x98) {
        active_addr = 0x69;
        i2c1_write_register(0x69, MPU6050_PWR_MGMT_1, 0x01);
        systick_delay_ms(50);
    }

    i2c1_write_register(active_addr, MPU6050_CONFIG, 0x02);
    i2c1_write_register(active_addr, MPU6050_SMPLRT_DIV, 19);

    int16_t bias_gx = 0, bias_gy = 0, bias_gz = 0;
    calibrate_gyro(active_addr, &bias_gx, &bias_gy, &bias_gz);

    uint8_t raw[14];
    uint32_t loop_counter = 0;
    static QuaternionPacket_t pkt;
    pkt.header[0] = 0xAA;
    pkt.header[1] = 0x55;

    while (1) {
        /* 1. Check for incoming Host Command via Circular RX DMA */
        uint8_t cmd = usart1_dma_read_cmd();
        if (cmd == 'C') {
            calibrate_gyro(active_addr, &bias_gx, &bias_gy, &bias_gz);
            madgwick_reset();
        } else if (cmd == '1') {
            madgwick_set_beta(0.03f); // Smooth / High-Stability
        } else if (cmd == '2') {
            madgwick_set_beta(0.08f); // Balanced Default
        } else if (cmd == '3') {
            madgwick_set_beta(0.25f); // Fast / High-Response
        }

        /* 2. Heartbeat LED */
        if (++loop_counter >= 25) {
            loop_counter = 0;
            GPIOC_ODR ^= (1 << 13);
        }

        /* 3. Sensor Read & Madgwick Fusion */
        i2c1_read_multibyte(active_addr, MPU6050_ACCEL_XOUT_H, raw, 14);

        int16_t ax = (int16_t)((raw[0] << 8) | raw[1]);
        int16_t ay = (int16_t)((raw[2] << 8) | raw[3]);
        int16_t az = (int16_t)((raw[4] << 8) | raw[5]);

        int16_t gx = ((int16_t)((raw[8]  << 8) | raw[9]))  - bias_gx;
        int16_t gy = ((int16_t)((raw[10] << 8) | raw[11])) - bias_gy;
        int16_t gz = ((int16_t)((raw[12] << 8) | raw[13])) - bias_gz;

        float gx_rad = ((float)gx / GYRO_SCALE) * DEG_TO_RAD;
        float gy_rad = ((float)gy / GYRO_SCALE) * DEG_TO_RAD;
        float gz_rad = ((float)gz / GYRO_SCALE) * DEG_TO_RAD;

        madgwick_update_6dof(gx_rad, gy_rad, gz_rad, (float)ax, (float)ay, (float)az);

        float q0, q1, q2, q3;
        madgwick_get_quaternion(&q0, &q1, &q2, &q3);

        pkt.q0_x10k = (int16_t)(q0 * 10000.0f);
        pkt.q1_x10k = (int16_t)(q1 * 10000.0f);
        pkt.q2_x10k = (int16_t)(q2 * 10000.0f);
        pkt.q3_x10k = (int16_t)(q3 * 10000.0f);

        uint8_t *raw_pkt = (uint8_t *)&pkt;
        uint8_t checksum = 0;
        for (int i = 0; i < 10; i++) {
            checksum ^= raw_pkt[i];
        }
        pkt.checksum = checksum;

        usart1_dma_send(raw_pkt, sizeof(QuaternionPacket_t));
        systick_delay_ms(18);
    }
}
