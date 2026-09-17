#ifndef __RCM_H__
#define __RCM_H__
#include "apm32f10x.h"
#include "apm32f10c_conf.h"

void REM_HSE_Confing(void);

/* SysTick 毫秒计数值: SysTick_Handler 中每 1ms 减 1 */
extern volatile uint32_t SysTick_Count;

/* SysTick 初始化: 配置 1ms 周期中断(需在 SysTick_Handler 中递减 SysTick_Count) */
void SysTick_Init(void);

/* 延时函数 */
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);

/* SysTick 中断钩子(供 apm32f10x_int.c 的 SysTick_Handler 调用) */
void Rcm_SysTickHook(void);

/* 获取自 SysTick_Init 起的毫秒计数 */
uint32_t millis(void);

#endif
