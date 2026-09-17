#include "Rcm.h"
#include "system_apm32f10x.h"   /* 引入 SystemCoreClock, 用于 SysTick 延时换算 */

//配置外部高速时钟
void REM_HSE_Confing(void)
{
	RCM_Reset();//复位系统时钟
	RCM_ConfigHSE(RCM_HSE_OPEN);//开启HSE
	if (RCM_WaitHSEReady() == SUCCESS)//HSE成功启动
	{
		FMC_EnablePrefetchBuffer();        // 开启Flash预取缓冲，提升Flash取指速度
		FMC_ConfigLatency(FMC_LATENCY_3);  // 设置Flash读等待周期 = 2个HCLK周期
		
		RCM_ConfigAHB(RCM_AHB_DIV_1);//AHB:72M
		RCM_ConfigAPB1(RCM_APB_DIV_2);//APB1:36M
		RCM_ConfigAPB2(RCM_APB_DIV_1);//APB2:72M
		
	  RCM_ConfigPLL(RCM_PLLSEL_HSE,RCM_PLLMF_9);
		RCM_EnablePLL();	
		while(RCM_ReadStatusFlag(RCM_FLAG_PLLRDY) == RESET);//等待PLL就绪
		
		RCM_ConfigSYSCLK(RCM_SYSCLK_SEL_PLL);//配置PLL为sysclk来源
		while(RCM_ReadSYSCLKSource() != RCM_SYSCLK_SEL_PLL);//读取SYSCLK来源是否PLL
	}
}

/* ============================ SysTick 延时 ============================ */
/* 系统主频: 72MHz (HSE 8MHz x PLL9)
 * 采用 SysTick 中断方式: SysTick_Init 一次性配置 1ms 周期中断,
 * SysTick_Handler 中每 1ms 将 SysTick_Count 减 1,
 * Delay_ms 通过轮询 SysTick_Count 实现, Delay_us 用软件循环近似. */

/** SysTick 毫秒计数值(全局, 供 SysTick_Handler 递减) */
volatile uint32_t SysTick_Count;

/* 上电以来的毫秒计数(由 SysTick_Handler 中断自增), 用于时间戳 */
static volatile uint32_t s_tick_ms = 0;

/**
 * @brief  获取自 SysTick_Init 起的毫秒计数(由 SysTick_Handler 1ms 累加)
 * @note   用于主循环周期性任务的时间基准
 */
uint32_t millis(void)
{
    return s_tick_ms;
}

/**
 * @brief  SysTick 中断钩子(供 SysTick_Handler 调用)
 * @note   每 1ms 调用一次, 用于递增系统毫秒计数
 */
void Rcm_SysTickHook(void)
{
    s_tick_ms++;
}

/**
 * @brief  SysTick 初始化: 配置为 1ms 周期中断(须在 main 中调用一次)
 * @note   使用 CMSIS SysTick_Config: 时钟源 HCLK(72MHz), 自动使能中断
 */
void SysTick_Init(void)
{
    SysTick_Count = 0;
    /* SysTick_Config 返回 1 表示参数非法(超出 24bit), 正常应返回 0 */
    if (SysTick_Config(SystemCoreClock / 1000UL) != 0)
    {
        while (1);  /* 配置失败, 卡住提示 */
    }
}

/**
 * @brief  毫秒级延时(中断方式, 精确)
 * @param  ms: 延时毫秒数
 */
void Delay_ms(uint32_t ms)
{
    SysTick_Count = ms;
    while (SysTick_Count != 0);   /* 等待 SysTick_Handler 将计数值减到 0 */
}

/**
 * @brief  微秒级延时(软件空循环近似, 基于 72MHz)
 * @param  us: 延时微秒数
 * @note   中断粒度 1ms, us 级延时用循环近似; 若需精确 us 延时请另配 DWT
 */
void Delay_us(uint32_t us)
{
    volatile uint32_t i;
    while (us--)
    {
        for (i = 0; i < 6; i++)   /* 每 us 约 6 次空循环, 实测可微调 */
        {
            __NOP();
        }
    }
}
