#include "485.h"
#include "ktm52.h"
#include "gt24c64a.h"
#include "spi.h"
#include "i2c.h"
#include "Rcm.h"

/* ============================ 485 底层收发 ============================ */

/*!
 * @brief   设置485收发方向
 */
void Encoder_SetDirection(uint16_t dir)
{
    if (dir == RS485_MODE_TX)
    {
        GPIO_SetBit(GPIOA, GPIO_PIN_12);
        GPIO_SetBit(GPIOA, GPIO_PIN_11);
    }
    else
    {
        GPIO_ResetBit(GPIOA, GPIO_PIN_12);
        GPIO_ResetBit(GPIOA, GPIO_PIN_11);
    }
}

/*!
 * @brief   经 485 DMA 发送 number 字节(DMA_USART1_TxBuf 内容)
 */
void Encoder_485_Senddata(uint16_t number)
{
    /* 发送期间屏蔽 RXNE/IDLE: 485 半双工 TX 时 RX 收到的是自身回声,
     * 不屏蔽会误触发接收状态机 */
    USART_DisableInterrupt(USART1, USART_INT_RXBNE);
    USART_DisableInterrupt(USART1, USART_INT_IDLE);
    Encoder_SetDirection(RS485_MODE_TX);
    UART1_DMASendByte(number);
    while(DMA_ReadStatusFlag(DMA1_FLAG_TC4) == RESET);
    DMA_ClearStatusFlag(DMA1_FLAG_TC4);  //清除传输完成标志
    while(USART_ReadStatusFlag(USART1, USART_FLAG_TXC) == RESET);
    Encoder_SetDirection(RS485_MODE_RX);
    (void)USART_RxData(USART1);                /* 丢弃发送期间可能误收的噪声字节 */
    USART_ClearIntFlag(USART1, USART_INT_ERR); /* 清 ORE/NE/FE/PE 错误标志 */
    Encoder_ResetRx();                         /* 复位接收状态机 */
    USART_EnableInterrupt(USART1, USART_INT_RXBNE);
    USART_EnableInterrupt(USART1, USART_INT_IDLE);
}

/* ============================ 编码器协议应用层 ============================ */

/* 全局编码器上下文 */
Encoder_t g_encoder;

/* 把 21bit abs_pos 拆为 ABS0/1/2(低中高) */
static void Encoder_AbsSplit(uint32_t abs, uint8_t* pAB0, uint8_t* pAB1, uint8_t* pAB2)
{
    *pAB0 = (uint8_t)abs;          /* ABS0*/
    *pAB1 = (uint8_t)((abs >> 8) & 0xFF);   /* ABS1*/
    *pAB2 = (uint8_t)((abs >> 16) & 0xFF);  /* ABS2*/
}

/* 内部接口前向声明(供中断内快速响应调用) */
static uint8_t EE_WriteOne(uint8_t addr, uint8_t data);
static uint8_t EE_ReadOne(uint8_t addr, uint8_t* pData);
static void Encoder_PrepareResponses(void);

/* 预组装响应帧(供中断内零组装直发, 需在使用前定义) */
uint8_t s_tx_id0[6];   /* 供 uart.c 中断直接访问(零函数调用) */
uint8_t s_tx_id3[11];  /* 供 uart.c 中断直接访问(零函数调用) */
uint8_t s_golden_buf[256];  /* Golden 批量写接收缓冲(129字节), uart.c 接收共用 */

/* 返回预组装响应缓冲指针(供中断直接写 TDR 用) */
uint8_t* Encoder_GetTxID0(void) { return s_tx_id0; }
uint8_t* Encoder_GetTxID3(void) { return s_tx_id3; }

/*****************************************************************************
 * @brief  中断内快速处理收到的一帧(USART1 IDLE 中断调用)
 * @note   角度已由后台锁存快照, 所有非阻塞命令都在中断内零拷贝直接回包,
 *         不经过主循环; 仅 ID4/ID5(自校准, 阻塞数百ms)交主循环处理
 *****************************************************************************/
void Encoder_HandleFrameISR(uint8_t* pBuf, uint16_t len)
{
    uint8_t cf;

    if (pBuf == NULL || len == 0) return;

    cf = pBuf[0];

    switch (cf)
    {
        case ENC_CF_ID2:  /* 读编码器 ID */
            DMA_USART1_TxBuf[0] = ENC_CF_ID2;
            DMA_USART1_TxBuf[1] = g_encoder.sf;
            DMA_USART1_TxBuf[2] = g_encoder.enid;
            DMA_USART1_TxBuf[3] = Encoder_CalcCRC8(DMA_USART1_TxBuf, 3);
            Encoder_485_Senddata(4);
            break;

        case ENC_CF_ID7:  /* 多圈归零: 后台归零 + 立即回单圈信息 */
            g_encoder.zero_pending = 1;
            DMA_USART1_TxBuf[0] = ENC_CF_ID7;
            DMA_USART1_TxBuf[1] = g_encoder.sf;
            Encoder_AbsSplit(g_encoder.abs_pos, &DMA_USART1_TxBuf[2],
                             &DMA_USART1_TxBuf[3], &DMA_USART1_TxBuf[4]);
            DMA_USART1_TxBuf[5] = Encoder_CalcCRC8(DMA_USART1_TxBuf, 5);
            Encoder_485_Senddata(6);
            break;

        case ENC_CF_ID8:  /* 单圈归零: 后台归零 + 立即回单圈信息 */
            g_encoder.zero_pending = 1;
            DMA_USART1_TxBuf[0] = ENC_CF_ID8;
            DMA_USART1_TxBuf[1] = g_encoder.sf;
            Encoder_AbsSplit(g_encoder.abs_pos, &DMA_USART1_TxBuf[2],
                             &DMA_USART1_TxBuf[3], &DMA_USART1_TxBuf[4]);
            DMA_USART1_TxBuf[5] = Encoder_CalcCRC8(DMA_USART1_TxBuf, 5);
            Encoder_485_Senddata(6);
            break;

        case ENC_CF_IDC:  /* 故障复位: 后台清 alarm + 立即回单圈信息 */
            g_encoder.alarm_reset_pending = 1;
            DMA_USART1_TxBuf[0] = ENC_CF_IDC;
            DMA_USART1_TxBuf[1] = g_encoder.sf;
            Encoder_AbsSplit(g_encoder.abs_pos, &DMA_USART1_TxBuf[2],
                             &DMA_USART1_TxBuf[3], &DMA_USART1_TxBuf[4]);
            DMA_USART1_TxBuf[5] = Encoder_CalcCRC8(DMA_USART1_TxBuf, 5);
            Encoder_485_Senddata(6);
            break;

        case ENC_CF_ID6:  /* 写 E2PROM: 立即回显 + 后台脏写 */
            if (len >= 4)
            {
                DMA_USART1_TxBuf[0] = ENC_CF_ID6;
                DMA_USART1_TxBuf[1] = pBuf[1];
                DMA_USART1_TxBuf[2] = pBuf[2];
                DMA_USART1_TxBuf[3] = Encoder_CalcCRC8(DMA_USART1_TxBuf, 3);
                Encoder_485_Senddata(4);
                EE_WriteOne(pBuf[1], pBuf[2]);
            }
            break;

        case ENC_CF_IDD:  /* 读 E2PROM */
            if (len >= 3)
            {
                uint8_t e2p_data = 0;
                if (!EE_ReadOne(pBuf[1], &e2p_data)) e2p_data = 0x00;
                DMA_USART1_TxBuf[0] = ENC_CF_IDD;
                DMA_USART1_TxBuf[1] = pBuf[1];
                DMA_USART1_TxBuf[2] = e2p_data;
                DMA_USART1_TxBuf[3] = Encoder_CalcCRC8(DMA_USART1_TxBuf, 3);
                Encoder_485_Senddata(4);
            }
            break;

        case ENC_CF_CTRL_OFF:  /* 关闭校准/寄存器/golden 操作 */
            g_encoder.op_enable = 0;
            DMA_USART1_TxBuf[0] = ENC_CF_CTRL_OFF;
            DMA_USART1_TxBuf[1] = 0x00;   /* Status=0 表示已关闭 */
            DMA_USART1_TxBuf[2] = Encoder_CalcCRC8(DMA_USART1_TxBuf, 2);
            Encoder_485_Senddata(3);
            break;

        case ENC_CF_CTRL_ON:   /* 开启校准/寄存器/golden 操作 */
            g_encoder.op_enable = 1;
            DMA_USART1_TxBuf[0] = ENC_CF_CTRL_ON;
            DMA_USART1_TxBuf[1] = 0x01;   /* Status=1 表示已开启 */
            DMA_USART1_TxBuf[2] = Encoder_CalcCRC8(DMA_USART1_TxBuf, 2);
            Encoder_485_Senddata(3);
            break;

        case ENC_CF_ID4:   /* 快速自校准: 阻塞数百ms, 交主循环 */
        case ENC_CF_ID5:   /* 慢自校准: 阻塞数百ms, 交主循环 */
        case ENC_CF_ID9:   /* 读寄存器: 需 SPI 阻塞读, 交主循环 */
        case ENC_CF_IDA:   /* 写寄存器: 需 SPI 阻塞写, 交主循环 */
        case ENC_CF_IDB:   /* 读 Golden 单个: 需 SPI 阻塞读, 交主循环 */
        case ENC_CF_IDE:   /* 写 Golden 单个: 需 SPI 阻塞写, 交主循环 */
        case ENC_CF_ID1C:  /* 读 Golden 全部: 阻塞 ~2.5ms, 交主循环 */
            {
                uint16_t i;
                for (i = 0; i < len && i < sizeof(RX_DATA); i++) RX_DATA[i] = pBuf[i];
                RX_LEN = len;
                rx_data = cf;
            }
            break;

        case ENC_CF_ID1D:  /* 写 Golden 全部: 129字节, 数据在 s_golden_buf, 交主循环 */
            rx_data = cf;  /* 不复制到 RX_DATA(32字节会溢出), 主循环直接读 s_golden_buf */
            break;

        default:  /* 未知/非法 CF: 异常响应 */
            DMA_USART1_TxBuf[0] = ENC_CF_ID3;
            DMA_USART1_TxBuf[1] = 0x00;
            DMA_USART1_TxBuf[2] = 0x00;
            DMA_USART1_TxBuf[3] = Encoder_CalcCRC8(DMA_USART1_TxBuf, 3);
            Encoder_485_Senddata(4);
            break;
    }
}

/*****************************************************************************
 * @brief  CRC-8 (x8+1 多项式): 对所有数据字节异或
 *****************************************************************************/
uint8_t Encoder_CalcCRC8(uint8_t* pData, uint16_t len)
{
    uint8_t crc = 0x00;
    uint16_t i;
    if (pData == NULL || len == 0) return 0;
    for (i = 0; i < len; i++) crc ^= pData[i];
    return crc;
}

/* ============================ 预组装响应帧 ============================ */
/* ID0/ID3 是 62.5us 轮询热路径: 后台预组装好响应帧(含 CRC),
 * 中断里零组装零拷贝直发, 把响应延迟压到最低 */
static void Encoder_PrepareResponses(void)
{
    /* ID0: CF+SF+ABS0+ABS1+ABS2+CRC */
    s_tx_id0[0] = ENC_CF_ID0;
    s_tx_id0[1] = g_encoder.sf;
    s_tx_id0[2] = (uint8_t)g_encoder.abs_pos;
    s_tx_id0[3] = (uint8_t)(g_encoder.abs_pos >> 8);
    s_tx_id0[4] = (uint8_t)(g_encoder.abs_pos >> 16);
    s_tx_id0[5] = Encoder_CalcCRC8(s_tx_id0, 5);

    /* ID3: CF+SF+ABS0..2+ENID+0+0+0+ALMC+CRC */
    s_tx_id3[0] = ENC_CF_ID3;
    s_tx_id3[1] = g_encoder.sf;
    s_tx_id3[2] = (uint8_t)g_encoder.abs_pos;
    s_tx_id3[3] = (uint8_t)(g_encoder.abs_pos >> 8);
    s_tx_id3[4] = (uint8_t)(g_encoder.abs_pos >> 16);
    s_tx_id3[5] = g_encoder.enid;
    s_tx_id3[6] = 0x00;
    s_tx_id3[7] = 0x00;
    s_tx_id3[8] = 0x00;
    s_tx_id3[9] = g_encoder.almc;
    s_tx_id3[10] = Encoder_CalcCRC8(s_tx_id3, 9);
}

/* ============================ EEPROM 页模型 + RAM 镜像 ============================ */

#define EE_TOTAL_BYTES  (ENC_EEPROM_PAGE_COUNT * ENC_EEPROM_PAGE_SIZE)   /* 762 */

static uint8_t  s_ee_page = 0;
static uint8_t  s_ee_ram[EE_TOTAL_BYTES];               /* RAM 镜像 */
static uint8_t  s_ee_dirty[(EE_TOTAL_BYTES + 7) / 8];   /* 脏位图 */
static uint16_t s_ee_dirty_count = 0;                   /* 脏字节计数, 0=无需扫描 */

static uint8_t EE_IsDirty(uint16_t off)  { return (s_ee_dirty[off >> 3] >> (off & 7)) & 1u; }
static void    EE_SetDirty(uint16_t off) {
    if (!EE_IsDirty(off)) { s_ee_dirty[off >> 3] |= (uint8_t)(1u << (off & 7)); s_ee_dirty_count++; }
}
static void    EE_ClrDirty(uint16_t off) {
    if (EE_IsDirty(off)) { s_ee_dirty[off >> 3] &= (uint8_t)~(1u << (off & 7)); s_ee_dirty_count--; }
}

/* 上电: EEPROM 全部加载到 RAM */
void Encoder_EEPROM_Load(void)
{
    uint16_t i;
    GT24C64A_ReadBytes(0, s_ee_ram, EE_TOTAL_BYTES);
    for (i = 0; i < sizeof(s_ee_dirty); i++) s_ee_dirty[i] = 0;
    s_ee_dirty_count = 0;
}

/* addr=127 切页; addr=0~126 写当前页(写 RAM, 后台回写 EEPROM) */
static uint8_t EE_WriteOne(uint8_t addr, uint8_t data)
{
    if (addr == ENC_EEPROM_PAGE_REG_ADDR)
    {
        if (data >= ENC_EEPROM_PAGE_COUNT) return 0;
        s_ee_page = data;
        return 1;
    }
    if (addr >= ENC_EEPROM_PAGE_REG_ADDR) return 0;
    {
        uint16_t off = (uint16_t)s_ee_page * ENC_EEPROM_PAGE_SIZE + addr;
        s_ee_ram[off] = data;
        EE_SetDirty(off);
    }
    return 1;
}

/* addr=127 读当前页; addr=0~126 读 RAM */
static uint8_t EE_ReadOne(uint8_t addr, uint8_t* pData)
{
    if (pData == NULL) return 0;
    if (addr == ENC_EEPROM_PAGE_REG_ADDR) { *pData = s_ee_page; return 1; }
    if (addr >= ENC_EEPROM_PAGE_REG_ADDR) return 0;
    *pData = s_ee_ram[(uint16_t)s_ee_page * ENC_EEPROM_PAGE_SIZE + addr];
    return 1;
}

/*****************************************************************************
 * @brief  编码器初始化
 *****************************************************************************/
void Encoder_Init(void)
{
    GPIO_Config_T gpioCfg;

    SPI_Init();
    GT24C64A_Init();   /* 硬件 I2C1 + EEPROM, 700kHz */
    KTM52_UnlockReg();

    /* PB5: alarm 故障输入(高电平=故障), 配置为下拉输入(默认低=正常) */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB);
    gpioCfg.pin = GPIO_PIN_5;
    gpioCfg.mode = GPIO_MODE_IN_PD;
    gpioCfg.speed = GPIO_SPEED_50MHz;
    GPIO_Config(GPIOB, &gpioCfg);

    g_encoder.abs_pos = 0;
    g_encoder.abs_crc_ok = 0;
    g_encoder.sf = 0;
    g_encoder.almc = 0;
    g_encoder.enid = 0x15;   /* 默认 ENID */
    g_encoder.zero_pending = 0;
    g_encoder.alarm_reset_pending = 0;
    g_encoder.op_enable = 0;   /* 默认关闭所有操作, 收到 0x01 才开启 */

    /* EEPROM 页模型初始化: 默认页 0 + 加载到 RAM 镜像 */
    s_ee_page = 0;
    Encoder_EEPROM_Load();

    /* 预组装 ID0/ID3 响应帧 */
    Encoder_PrepareResponses();
}
/*****************************************************************************
 * @brief  主循环持续调用: 非阻塞读角度并锁存快照 + 后台回写 EEPROM
 * @note    ID0 能零等待返回最新角度快照
 *****************************************************************************/
void Encoder_AnglePoll(void)
{
    static uint8_t  busy = 0;
    static uint16_t flushing_addr = 0xFFFF;  /* 0xFFFF = 无正在写的字节 */
    static uint32_t flush_start = 0;

    /* ---- 后台故障复位: 不涉及 SPI, 随时清 alarm ---- */
    if (g_encoder.alarm_reset_pending)
    {
        g_encoder.alarm_reset_pending = 0;
        g_encoder.almc = 0;
    }

    /* ---- 后台归零: 需独占 SPI, 等角度读取空闲(!busy)时执行 ---- */
    if (g_encoder.zero_pending && !busy)
    {
        g_encoder.zero_pending = 0;
        KTM52_SetZero();
    }

    /* 持续非阻塞读角度: SPI 空闲就启动, 完成就锁存快照 */
    if (!busy)
    {
        KTM52_StartAngle();
        busy = 1;
    }
    else if (SPI_DMA_IsDone())
    {
        KTM52_Angle_t a;
        g_encoder.abs_crc_ok = KTM52_FinishAngle(&a);
        g_encoder.abs_pos = a.angle;   /* 21bit 单圈位置 */
        busy = 0;

        /* alarm bit2: 单圈获取失败(CRC 失败)锁存 */
        if (g_encoder.abs_crc_ok != KTM52_OK)
        {
            g_encoder.almc |= (1u << 2);
        }
    }

    /* alarm bit7: PB5 输入高电平 = 故障(锁存) */
    if (GPIO_ReadInputBit(GPIOB, GPIO_PIN_5) == SET)
    {
        g_encoder.almc |= (1u << 7);
    }

    /* EEPROM 非阻塞后台写: I2C_MemWrite 启动后立即返回(不等 5ms tWR),
     * 5ms 后用 I2C_IsDeviceReady 检查 ACK(此时 ACK 已就绪, 立即返回).
     * 避免 5ms 阻塞主循环导致 62.5us 快速命令下大量丢帧 */
    if (flushing_addr != 0xFFFF)
    {
        /* 正在写, 5ms 后检查 ACK */
        if (millis() - flush_start >= 5)
        {
            if (I2C_IsDeviceReady(GT24C64A_ADDR) == I2C_OK)
            {
                EE_ClrDirty(flushing_addr);
                flushing_addr = 0xFFFF;   /* 清标记, 下次循环找新脏字节 */
            }
        }
    }
    else if (s_ee_dirty_count > 0)   /* 无脏字节时 O(1) 跳过, 避免每轮扫 1016 字节拖慢响应 */
    {
        /* 找一个脏字节, 启动 IIC 写(不阻塞等 tWR) */
        uint16_t i;
        for (i = 0; i < EE_TOTAL_BYTES; i++)
        {
            if (EE_IsDirty(i))
            {
                uint8_t data = s_ee_ram[i];
                if (I2C_MemWrite(GT24C64A_ADDR, (uint16_t)i, &data, 1) == I2C_OK)
                {
                    flushing_addr = i;
                    flush_start = millis();
                }
                break;
            }
        }
    }

    /* 预组装 ID0/ID3 响应帧(含最新 abs_pos/sf/almc), 供中断零组装直发 */
    Encoder_PrepareResponses();
}

/*****************************************************************************
 * @brief  主循环处理: 仅处理 ID4/ID5 自校准(阻塞数百ms, 不能进中断)
 * @note   其余命令(ID0/2/3/6/D/7/8/C)已在 USART1 中断内处理完毕
 *****************************************************************************/
void Encoder_ProcessRx(uint8_t* pBuf, uint16_t len)
{
    uint8_t cf;

    if (pBuf == NULL || len == 0) return;

    cf = pBuf[0];

    /* 校准命令: 检查 op_enable */
    if (cf == ENC_CF_ID4)  /* 快速自校准 */
    {
        if (!g_encoder.op_enable)
        {
            DMA_USART1_TxBuf[0] = ENC_CF_ID4;
            DMA_USART1_TxBuf[1] = 0xFF;  /* 0xFF = 操作被禁止 */
            DMA_USART1_TxBuf[2] = Encoder_CalcCRC8(DMA_USART1_TxBuf, 2);
            Encoder_485_Senddata(3);
            return;
        }
        DMA_USART1_TxBuf[0] = ENC_CF_ID4;
        DMA_USART1_TxBuf[1] = KTM52_FastSelfCorrection();
        Encoder_485_Senddata(2);
    }
    else if (cf == ENC_CF_ID5)  /* 慢自校准 */
    {
        if (!g_encoder.op_enable)
        {
            DMA_USART1_TxBuf[0] = ENC_CF_ID5;
            DMA_USART1_TxBuf[1] = 0xFF;
            DMA_USART1_TxBuf[2] = Encoder_CalcCRC8(DMA_USART1_TxBuf, 2);
            Encoder_485_Senddata(3);
            return;
        }
        DMA_USART1_TxBuf[0] = ENC_CF_ID5;
        DMA_USART1_TxBuf[1] = KTM52_SlowSelfCorrection();
        Encoder_485_Senddata(2);
    }
    else if (cf == ENC_CF_ID9)  /* 读寄存器(无CRC) */
    {
        uint8_t regVal = 0xFF;
        if (g_encoder.op_enable && len >= 2)
        {
            while (!SPI_DMA_IsDone());
            SPI_DMA_Finish();
            KTM52_ReadReg(pBuf[1], &regVal);
        }
        DMA_USART1_TxBuf[0] = ENC_CF_ID9;
        DMA_USART1_TxBuf[1] = regVal;
        Encoder_485_Senddata(2);
    }
    else if (cf == ENC_CF_IDA)  /* 写寄存器(无CRC) */
    {
        uint8_t result = 0xFF;
        if (g_encoder.op_enable && len >= 3)
        {
            while (!SPI_DMA_IsDone());
            SPI_DMA_Finish();
            result = KTM52_WriteReg(pBuf[1], pBuf[2]);
        }
        DMA_USART1_TxBuf[0] = ENC_CF_IDA;
        DMA_USART1_TxBuf[1] = (len >= 3) ? pBuf[1] : 0xFF;
        DMA_USART1_TxBuf[2] = (result == KTM52_OK) ? pBuf[2] : 0xFF;
        Encoder_485_Senddata(3);
    }
    else if (cf == ENC_CF_IDB)  /* 读 Golden 单个(无CRC): Req=[CF+Idx] */
    {
        uint8_t low  = 0xFF;
        uint8_t high = 0xFF;
        uint8_t idx  = (len >= 2) ? pBuf[1] : 0xFF;
        if (g_encoder.op_enable && idx < 64)
        {
            while (!SPI_DMA_IsDone());
            SPI_DMA_Finish();
            low  = KTM52_ReadRegRaw(0x80 + 2 * idx);
            high = KTM52_ReadRegRaw(0x81 + 2 * idx);
        }
        DMA_USART1_TxBuf[0] = ENC_CF_IDB;
        DMA_USART1_TxBuf[1] = low;
        DMA_USART1_TxBuf[2] = high;
        Encoder_485_Senddata(3);
    }
    else if (cf == ENC_CF_IDE)  /* 写 Golden 单个(无CRC): Req=[CF+Idx+Low+High] */
    {
        uint8_t low  = (len >= 4) ? pBuf[2] : 0xFF;
        uint8_t high = (len >= 4) ? pBuf[3] : 0xFF;
        uint8_t idx  = (len >= 4) ? pBuf[1] : 0xFF;
        if (g_encoder.op_enable && idx < 64 && len >= 4)
        {
            while (!SPI_DMA_IsDone());
            SPI_DMA_Finish();
            KTM52_UnlockReg();
            if (KTM52_WriteReg(0x80 + 2 * idx, low)  != KTM52_OK) low  = 0xFF;
            if (KTM52_WriteReg(0x81 + 2 * idx, high) != KTM52_OK) high = 0xFF;
        }
        DMA_USART1_TxBuf[0] = ENC_CF_IDE;
        DMA_USART1_TxBuf[1] = low;
        DMA_USART1_TxBuf[2] = high;
        Encoder_485_Senddata(3);
    }
    else if (cf == ENC_CF_ID1C)  /* 读 Golden 全部(无CRC): 响应129字节 */
    {
        uint8_t i;
        if (!g_encoder.op_enable)
        {
            DMA_USART1_TxBuf[0] = ENC_CF_ID1C;
            DMA_USART1_TxBuf[1] = 0xFE;
            Encoder_485_Senddata(2);
            return;
        }
        while (!SPI_DMA_IsDone());
        SPI_DMA_Finish();

        DMA_USART1_TxBuf[0] = ENC_CF_ID1C;
        for (i = 0; i < 64; i++)
        {
            DMA_USART1_TxBuf[1 + 2*i]     = KTM52_ReadRegRaw(0x80 + 2*i);
            DMA_USART1_TxBuf[1 + 2*i + 1] = KTM52_ReadRegRaw(0x81 + 2*i);
        }
        Encoder_485_Senddata(129);
    }
    else if (cf == ENC_CF_ID1D)  /* 写 Golden 全部(无CRC): 请求129字节 */
    {
        uint8_t i;
        uint8_t status = 0xFF;
        if (!g_encoder.op_enable || len < 129)
        {
            DMA_USART1_TxBuf[0] = ENC_CF_ID1D;
            DMA_USART1_TxBuf[1] = 0xFE;
            Encoder_485_Senddata(2);
            return;
        }
        while (!SPI_DMA_IsDone());
        SPI_DMA_Finish();
        KTM52_UnlockReg();
        for (i = 0; i < 64; i++)
        {
            uint8_t low  = pBuf[1 + 2*i];
            uint8_t high = pBuf[1 + 2*i + 1];
            if (KTM52_WriteReg(0x80 + 2*i, low) != KTM52_OK ||
                KTM52_WriteReg(0x81 + 2*i, high) != KTM52_OK)
            {
                status = i;
                break;
            }
        }
        DMA_USART1_TxBuf[0] = ENC_CF_ID1D;
        DMA_USART1_TxBuf[1] = status;
        Encoder_485_Senddata(2);
    }
}


