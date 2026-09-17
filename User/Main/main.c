/* Includes */
#include "apm32f10x.h"
#include "apm32f10c_conf.h"
#include "Rcm.h"
#include "uart.h"
#include "485.h"
#include "gt24c64a.h"
#include "ktm52.h"
//eeprom:635ms写完
/* Test: 1=持续读写 EEPROM 用于示波器测 I2C 速率; 测完改回 0 */
#define MEASURE_I2C    0

#define satus		 0
#if satus
#define UART_BaudRate		115200
#else
#define UART_BaudRate		2500000
#endif

int main(void)
{
    REM_HSE_Confing();
    NVIC_ConfigPriorityGroup (NVIC_PRIORITY_GROUP_2);
    SysTick_Init();
    uart1_Init(UART_BaudRate);

	/* 编码器应用层初始化: SPI/I2C + KTM52 解锁，上电10ms在读取 */
    Encoder_Init();

#if MEASURE_I2C
    /* ===== 临时测试: KTM52 读角度 + 485 发送(模拟 ID0 响应) ===== */
    GT24C64A_Init();
    {
        while (1)
        {
            KTM52_Angle_t a;
            uint8_t tx[6];
            uint8_t i;

            /* 读 KTM52 角度 */
            KTM52_ReadAngle(&a);

            /* 模拟 ID0 响应: CF + SF + ABS0/1/2 + CRC, 经 485 发出 */
            tx[0] = ENC_CF_ID0;                          /* 0x02 */
            tx[1] = 0x00;                                 /* SF */
            tx[2] = (uint8_t)(a.angle >> 8);              /* ABS0 */
            tx[3] = (uint8_t)((a.angle >> 16) & 0xFF);    /* ABS1 */
            tx[4] = (uint8_t)((a.angle >> 24) & 0xFF);    /* ABS2 */
            tx[5] = Encoder_CalcCRC8(tx, 5);

            for (i = 0; i < 6; i++) DMA_USART1_TxBuf[i] = tx[i];
            Encoder_485_Senddata(6);

            Delay_ms(100);   /* 100ms 发一次 */
        }
    }
#else
    while (1)
    {
        /* 持续非阻塞读角度 + 锁存快照 + 后台回写 EEPROM */
        Encoder_AnglePoll();

        /* 检测 485 接收: IDLE 中断置 rx_data=1, 整帧在 RX_DATA/RX_LEN */
        if (rx_data != 0)
        {
            uint8_t cf = rx_data;
            rx_data = 0;
            if (cf == ENC_CF_ID1D)          /* 写 Golden 全部: 数据在 s_golden_buf */
            {
                Encoder_ProcessRx(s_golden_buf, 129);
            }
            else
            {
                Encoder_ProcessRx(RX_DATA, RX_LEN);
            }
        }
    }
#endif
}
