#ifndef SYSTICK_H_
#define SYSTICK_H_

#include <stdint.h>

#define SYSTICK           ((volatile SysTick_Type *)0xE000E010)

typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_Type;

#define SYSTICK_CTRL_ENABLE    (1 << 0)
#define SYSTICK_CTRL_TICKINT   (1 << 1)
#define SYSTICK_CTRL_CLKSOURCE (1 << 2)

extern volatile uint8_t g_imu_ready;

void systick_init(void);
void SysTick_Handler(void);
uint32_t systick_get_tick(void);
void systick_delay_ms(uint32_t ms);

#endif /* SYSTICK_H_ */
