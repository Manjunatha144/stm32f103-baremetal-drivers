#ifndef SYSTICK_H_
#define SYSTICK_H_

#include <stdint.h>

/* SysTick Base Address (ARM Cortex-M3 Core Peripheral) */
#define SYSTICK_BASE        (0xE000E010UL)

typedef struct {
    volatile uint32_t CTRL;   /* SysTick Control and Status Register */
    volatile uint32_t LOAD;   /* SysTick Reload Value Register       */
    volatile uint32_t VAL;    /* SysTick Current Value Register      */
    volatile uint32_t CALIB;  /* SysTick Calibration Register        */
} SysTick_RegDef_t;

#define SYSTICK             ((SysTick_RegDef_t *)SYSTICK_BASE)

/* SysTick Register Bit Definitions */
#define SYSTICK_CTRL_ENABLE       (1UL << 0)  /* Counter enable               */
#define SYSTICK_CTRL_TICKINT      (1UL << 1)  /* Interrupt enable (1=on, 0=off)*/
#define SYSTICK_CTRL_CLKSOURCE    (1UL << 2)  /* 1 = Processor clock (AHB)    */
#define SYSTICK_CTRL_COUNTFLAG    (1UL << 16) /* Returns 1 if timer counted to 0 */

/* SysTick Driver APIs */
void systick_init(void);
void systick_delay_ms(uint32_t ms);

#endif /* SYSTICK_H_ */
