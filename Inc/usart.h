#ifndef USART_H_
#define USART_H_

#include <stdint.h>

#ifndef RCC_BASE
#define RCC_BASE            0x40021000UL
#endif

#ifndef GPIOA_BASE
#define GPIOA_BASE          0x40010800UL
#endif

#define USART1_BASE         0x40013800UL

/* RCC Register */
#ifndef RCC_APB2ENR
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x18))
#endif

/* GPIOA Register */
#ifndef GPIOA_CRH
#define GPIOA_CRH           (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#endif

/* USART1 Registers */
#define USART1_SR           (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR           (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR          (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1          (*(volatile uint32_t *)(USART1_BASE + 0x0C))

/* Function Prototypes */
void usart1_init(void);
void usart1_write(char ch);
void usart1_print(const char *str);

#endif /* USART_H_ */
