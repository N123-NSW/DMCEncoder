#include "uart.h"
#include "485.h"
uint8_t DMA_USART1_TxBuf[BufSize];
/* USART1 RX Buf */
uint8_t DMA_USART1_RxBuf[BufSize];
uint8_t RX_DATA[32];
uint8_t rx_data;
/* 最近一次 485 接收字节数(供 app 层解析整帧), 在 USART1_IDLE 中断里赋值 */
volatile uint16_t RX_LEN = 0;
volatile uint32_t usart_err_cnt = 0;
/** @brief  串口1初始化(轮询/中断收发共用基础配置) */
void uart1_Init(uint32_t baudRate)
{
		GPIO_Config_T GPIO_configStruct;
		USART_Config_T USART_ConfigStruct;

		RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA);
		RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_USART1);
		//TX
		GPIO_configStruct.pin = GPIO_PIN_9;
		GPIO_configStruct.mode = GPIO_MODE_AF_PP;
		GPIO_configStruct.speed = GPIO_SPEED_50MHz;
		GPIO_Config(GPIOA, &GPIO_configStruct);
		//RX (上拉: 485 发送期间 RE=1 时 RO 高阻, 上拉保持空闲电平避免误接收噪声)
		GPIO_configStruct.pin = GPIO_PIN_10;
		GPIO_configStruct.mode = GPIO_MODE_IN_PU;
		GPIO_configStruct.speed = GPIO_SPEED_50MHz;
		GPIO_Config(GPIOA, &GPIO_configStruct);
		//DE
		GPIO_configStruct.pin = GPIO_PIN_12;
		GPIO_configStruct.mode = GPIO_MODE_OUT_PP;
		GPIO_configStruct.speed = GPIO_SPEED_50MHz;
		GPIO_Config(GPIOA, &GPIO_configStruct);
		//RE
		GPIO_configStruct.pin = GPIO_PIN_11;
		GPIO_configStruct.mode = GPIO_MODE_OUT_PP;
		GPIO_configStruct.speed = GPIO_SPEED_50MHz;
		GPIO_Config(GPIOA, &GPIO_configStruct);
		
		USART_ConfigStruct.baudRate = baudRate;
		USART_ConfigStruct.hardwareFlow = USART_HARDWARE_FLOW_NONE;
		USART_ConfigStruct.mode = USART_MODE_TX_RX;
		USART_ConfigStruct.parity = USART_PARITY_NONE;
		USART_ConfigStruct.stopBits = USART_STOP_BIT_1;
		USART_ConfigStruct.wordLength = USART_WORD_LEN_8B;
		USART_Config(USART1, &USART_ConfigStruct);

		USART_Enable(USART1);

		/* 开启接收中断: RXNE 逐字节接收 + IDLE 帧间复位 */
		USART_EnableInterrupt(USART1, USART_INT_RXBNE);
		USART_EnableInterrupt(USART1, USART_INT_IDLE);
		NVIC_EnableIRQRequest(USART1_IRQn, 0, 0);

		DMA_Config_T dmaConfig;

		/* 使能 DMA1 时钟 */
		RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);

		/* ---- TX: DMA1_Channel4 ---- */
		DMA_ConfigStructInit(&dmaConfig);
		dmaConfig.peripheralBaseAddr = (uint32_t)USART1 + 0x04; /* USART1 数据寄存器地址(DR 偏移 0x04) */
		dmaConfig.memoryBaseAddr     = (uint32_t)DMA_USART1_TxBuf;            /* 发送时动态指定 */
		dmaConfig.dir                = DMA_DIR_PERIPHERAL_DST; /* 内存 -> 外设 */
		dmaConfig.bufferSize         = BufSize;
		dmaConfig.peripheralInc      = DMA_PERIPHERAL_INC_DISABLE;
		dmaConfig.memoryInc          = DMA_MEMORY_INC_ENABLE;
		dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_BYTE;
		dmaConfig.memoryDataSize     = DMA_MEMORY_DATA_SIZE_BYTE;
		dmaConfig.loopMode           = DMA_MODE_NORMAL;
		dmaConfig.priority           = DMA_PRIORITY_HIGH;
		dmaConfig.M2M                = DMA_M2MEN_DISABLE;
		DMA_Config(DMA1_Channel4, &dmaConfig);

		USART_EnableDMA(USART1, USART_DMA_TX);
		DMA_Disable(DMA1_Channel4);

		/* ---- RX: DMA1_Channel5 ---- */
		DMA_ConfigStructInit(&dmaConfig);
		dmaConfig.peripheralBaseAddr = (uint32_t)USART1 + 0x04; /* USART1 数据寄存器地址(DR 偏移 0x04) */
		dmaConfig.memoryBaseAddr     = (uint32_t)DMA_USART1_RxBuf;            /* 接收时动态指定 */
		dmaConfig.dir                = DMA_DIR_PERIPHERAL_SRC; /* 外设 -> 内存 */
		dmaConfig.bufferSize         = BufSize;
		dmaConfig.peripheralInc      = DMA_PERIPHERAL_INC_DISABLE;
		dmaConfig.memoryInc          = DMA_MEMORY_INC_ENABLE;
		dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_BYTE;
		dmaConfig.memoryDataSize     = DMA_MEMORY_DATA_SIZE_BYTE;
		dmaConfig.loopMode           = DMA_MODE_NORMAL;
		dmaConfig.priority           = DMA_PRIORITY_HIGH;
		dmaConfig.M2M                = DMA_M2MEN_DISABLE;
		DMA_Config(DMA1_Channel5, &dmaConfig);

		/* 清除残留标志 */
		DMA_ClearStatusFlag(DMA1_FLAG_TC4);
		}
int fputc(int ch, FILE *f)
{
    /* send a byte of data to the serial port */
    USART_TxData(USART1, (uint8_t)ch);

    /* wait for the data to be send  */
    while (USART_ReadStatusFlag(USART1, USART_FLAG_TXBE) == RESET);

    return (ch);
}
/** @brief  开启一次DMA传输 */
void UART1_DMASendByte(uint16_t number)
{		
	DMA_Disable(DMA1_Channel4);
	DMA1_Channel4->CHMADDR = (uint32_t)DMA_USART1_TxBuf; /* 恢复默认发送缓冲 */
	DMA_ConfigDataNumber(DMA1_Channel4,number);
	DMA_Enable(DMA1_Channel4);
}

/* RXNE 逐字节接收状态机:
 * 单字节命令收到 CF 立即响应, 多字节收满后响应.
 * s_rx_buf[8] 常规命令(最大 ID6=4B); s_golden_buf[256] Golden 批量写专用(129B) */
static uint8_t  s_rx_buf[8];
static uint8_t* s_rx_ptr  = s_rx_buf;  /* 当前接收缓冲指针(常规或 golden) */
static uint8_t  s_rx_cnt  = 0;         /* 已收字节数 */
static uint8_t  s_rx_need = 0;         /* 本帧总字节数 */

static uint8_t Encoder_GetFrameLen(uint8_t cf)
{
    /* 常规命令: 始终有效, 不依赖 op_enable */
    switch (cf)
    {
        case ENC_CF_ID6: return 4;   /* CF+ADF+EDF+CRC */
        case ENC_CF_IDD: return 3;   /* CF+ADF+CRC */
        default: break;
    }

    /* 校准/寄存器/golden 命令: 仅 op_enable==1 时才判断帧长 */
    if (g_encoder.op_enable)
    {
        switch (cf)
        {
            case ENC_CF_ID9:  return 2;   /* CF+RegAddr */
            case ENC_CF_IDA:  return 3;   /* CF+RegAddr+RegData */
            case ENC_CF_IDB:  return 2;   /* CF+Idx */
            case ENC_CF_IDE:  return 4;   /* CF+Idx+Low+High */
            case ENC_CF_ID1D: return 129; /* CF+128字节 */
            default: break;
        }
    }

    return 1;  /* 其余单字节命令(ID0/2/3/4/5/7/8/C/0x00/0x01/ID1C/未知) */
}

/* 复位接收状态机(485 发送完切回 RX 后调用, 丢弃发送期间噪声) */
void Encoder_ResetRx(void)
{
    s_rx_cnt  = 0;
    s_rx_need = 0;
    s_rx_ptr  = s_rx_buf;
}

/** @brief  USART1 中断服务函数(接收) */
void USART1_IRQHandler(void)
{
    /* RXNE: 逐字节接收, 收满一帧立即响应
     * 直接访问寄存器(不用 USART_ReadIntFlag 库函数, 省掉多层判断开销) */
    if (USART1->STS & (1U << 5))  /* RXNE 位 */
    {
        uint8_t b = (uint8_t)USART1->DATA;

        /* 首字节热路径: ID0/ID3(62.5us 轮询) 直接响应, 零帧长判断, 不碰 s_rx_buf */
        if (s_rx_cnt == 0 && (b == ENC_CF_ID0 || b == ENC_CF_ID3))
        {
            uint8_t* txbuf;
            uint16_t txlen;
            uint16_t i;
            if (b == ENC_CF_ID3) { txbuf = Encoder_GetTxID3(); txlen = 11; }
            else                 { txbuf = Encoder_GetTxID0(); txlen = 6;  }

            USART1->CTRL1 &= ~((1U << 5) | (1U << 4));  /* 禁 RXBNE + IDLE */
            GPIOA->BSC = (1U << 12) | (1U << 11);        /* 切 TX: DE+RE 高 */
            USART1->DATA = txbuf[0];                     /* 首字节零等待(TXE 上电/发完后=1) */
            for (i = 1; i < txlen; i++)
            {
                while (!(USART1->STS & (1U << 7)));    /* 后续字节等 TXBE */
                USART1->DATA = txbuf[i];
            }
            while (!(USART1->STS & (1U << 6)));         /* 等 TC */
            GPIOA->BC = (1U << 12) | (1U << 11);         /* 切 RX: DE+RE 低 */
            (void)USART1->STS;
            (void)USART1->DATA;                          /* 清 ORE/IDLE */
            USART1->CTRL1 |= (1U << 5) | (1U << 4);    /* 使能 RXBNE + IDLE */
            return;
        }

        /* 其余命令: 正常状态机收满后交 Encoder_HandleFrameISR */
        if (s_rx_cnt == 0)
        {
            /* 首字节: 确定帧长并选择缓冲(长帧用 golden 缓冲, 短帧用常规缓冲) */
            s_rx_need = Encoder_GetFrameLen(b);
            s_rx_ptr  = (s_rx_need > (uint16_t)sizeof(s_rx_buf)) ? s_golden_buf : s_rx_buf;
        }

        s_rx_ptr[s_rx_cnt] = b;
        s_rx_cnt++;

        if (s_rx_cnt >= s_rx_need)
        {
            Encoder_HandleFrameISR(s_rx_ptr, s_rx_cnt);
            s_rx_cnt  = 0;
            s_rx_need = 0;
            s_rx_ptr  = s_rx_buf;   /* 复位指针 */
        }
    }
    /* IDLE: 帧间空闲, 复位状态机(防止多字节命令中途出错卡住) */
    else if (USART_ReadIntFlag(USART1, USART_INT_IDLE) != RESET)
    {
        USART_ClearIntFlag(USART1, USART_INT_IDLE);
        (void)USART_RxData(USART1);
        s_rx_cnt  = 0;
        s_rx_need = 0;
    }
    /* ERR: 溢出/帧错误, 读完 DR 清标志 */
    else if (USART_ReadIntFlag(USART1, USART_INT_ERR) != RESET)
    {
        usart_err_cnt++;
        (void)USART_RxData(USART1);
        USART_ClearIntFlag(USART1, USART_INT_ERR);
        s_rx_cnt  = 0;
        s_rx_need = 0;
    }
}
