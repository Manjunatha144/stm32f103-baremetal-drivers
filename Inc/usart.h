#ifndef USART_H_
#define USART_H_

#include <stdint.h>

void usart1_init(void);
void usart1_dma_init(void);
void usart1_dma_send(const uint8_t *data, uint16_t length);
uint8_t usart1_dma_read_cmd(void);

#endif /* USART_H_ */
