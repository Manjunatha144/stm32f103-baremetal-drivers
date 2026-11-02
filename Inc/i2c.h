#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>

extern volatile uint8_t g_i2c_rx_complete;

void i2c1_init(void);
void i2c1_dma_init(void);
void i2c1_write_register(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);
uint8_t i2c1_read_register(uint8_t dev_addr, uint8_t reg_addr);
void i2c1_read_multibyte(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buffer, uint8_t length);

/* Non-blocking DMA burst read */
void i2c1_dma_start_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *dst_buffer, uint16_t length);

#endif /* I2C_H_ */
