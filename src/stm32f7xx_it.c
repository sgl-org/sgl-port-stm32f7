#include "stm32f7xx_hal.h"
#include "stm32f7xx_it.h"

void __attribute__((__weak__))
NMI_Handler(void)
{
    /* Nothing */
}

void __attribute__((__weak__))
HardFault_Handler(void)
{
    while (1);
}

void __attribute__((__weak__))
BusFault_Handler(void)
{
    /* Nothing */
}

void __attribute__((__weak__))
UsageFault_Handler(void)
{
    /* Nothing */
}

void __attribute__((__weak__))
SVC_Handler(void)
{
    /* Nothing */
}

void __attribute__((__weak__))
DebugMon_Handler(void)
{
    /* Nothing */
}

void __attribute__((__weak__))
MemManage_Handler(void)
{
    /* Nothing */
}

void __attribute__((__weak__))
PendSV_Handler(void)
{
    /* Nothing */
}

void __attribute__((__weak__))
SysTick_Handler(void)
{
    HAL_IncTick();
}
