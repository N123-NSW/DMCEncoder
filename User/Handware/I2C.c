#include "i2c.h"

/* 等待超时(空循环, 防止总线错误时死等) */
#define I2C_TIMEOUT  50000UL

/*****************************************************************************
 * @brief  初始化 I2C1 为硬件主机(PB6=SCL, PB7=SDA)
 * @param  clockSpeed: I2C 速率 Hz, 如 700000
 *****************************************************************************/
void I2C_Init(uint32_t clockSpeed)
{
    GPIO_Config_T gpioCfg;
    I2C_Config_T i2cCfg;

    /* 时钟 */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOB | RCM_APB2_PERIPH_AFIO);
    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_I2C1);

    /* PB6/PB7 复用开漏 */
    gpioCfg.pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpioCfg.mode = GPIO_MODE_AF_OD;
    gpioCfg.speed = GPIO_SPEED_50MHz;
    GPIO_Config(GPIOB, &gpioCfg);

    /* 速率钳制: APM32F103 硬件 I2C 最大 400kHz(快速模式), 700kHz 超规格会导致写入失败 */
    if (clockSpeed > 400000UL)
    {
        clockSpeed = 400000UL;
    }

    /* I2C 主机 */
    I2C_Reset(I2C1);
    i2cCfg.mode = I2C_MODE_I2C;
    i2cCfg.dutyCycle = I2C_DUTYCYCLE_2;   /* 400kHz 用 2:1 占空比, CCR=30 时序稳定 */
    i2cCfg.ackAddress = I2C_ACK_ADDRESS_7BIT;
    i2cCfg.ownAddress1 = 0x00;
    i2cCfg.ack = I2C_ACK_ENABLE;
    i2cCfg.clockSpeed = clockSpeed;
    I2C_Config(I2C1, &i2cCfg);
    I2C_Enable(I2C1);
}

/*****************************************************************************
 * @brief  内部: 等待事件(带超时与错误检测)
 * @retval I2C_OK / I2C_NACK
 *****************************************************************************/
static uint8_t I2C_WaitEvent(I2C_EVENT_T event)
{      
    uint32_t timeout = I2C_TIMEOUT;
    while (timeout--)
    {
        /* 先检查错误: 从机 NACK(AE)/总线错误(BERR)/仲裁丢失(AL)。
           注意: 从机 NACK 时 ADDR 位同样会置位, 若先判事件会误判为成功,
           导致 I2C_IsDeviceReady 的 ACK 轮询失效。 */
        if (I2C_ReadStatusFlag(I2C1, I2C_FLAG_BERR) ||
            I2C_ReadStatusFlag(I2C1, I2C_FLAG_AL)    ||
            I2C_ReadStatusFlag(I2C1, I2C_FLAG_AE))
        {
            I2C_ClearStatusFlag(I2C1, I2C_FLAG_BERR);
            I2C_ClearStatusFlag(I2C1, I2C_FLAG_AL);
            I2C_ClearStatusFlag(I2C1, I2C_FLAG_AE);
            return I2C_NACK;
        }
        if (I2C_ReadEventStatus(I2C1, event))
        {
            return I2C_OK;
        }
    }
    return I2C_NACK;
}

/*****************************************************************************
 * @brief  检测器件是否就绪(ACK 轮询)
 *****************************************************************************/
uint8_t I2C_IsDeviceReady(uint8_t devAddr)
{
    uint8_t ready = 0;

    I2C_EnableAcknowledge(I2C1);

    /* 产生 START */
    I2C_EnableGenerateStart(I2C1);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != I2C_OK)
    {
        I2C_EnableGenerateStop(I2C1);
        return I2C_NACK;
    }

    /* 发器件地址(写方向), 若从机忙则 NACK(AE), 不成立则等待超时 */
    I2C_Tx7BitAddress(I2C1, (uint8_t)(devAddr << 1), I2C_DIRECTION_TX);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == I2C_OK)
    {
        ready = I2C_OK;
    }

    /* 发 STOP */
    I2C_EnableGenerateStop(I2C1);
    return ready;
}

/*****************************************************************************
 * @brief  写多字节到器件内存地址
 *****************************************************************************/
uint8_t I2C_MemWrite(uint8_t devAddr, uint16_t memAddr, uint8_t* pBuf, uint16_t len)
{
    uint16_t i;

    if (len == 0) return I2C_OK;

    /* 产生 START */
    I2C_EnableGenerateStart(I2C1);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != I2C_OK)
    {
        return I2C_NACK;
    }

    /* 器件地址(写) */
    I2C_Tx7BitAddress(I2C1, (uint8_t)(devAddr << 1), I2C_DIRECTION_TX);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != I2C_OK)
    {
        I2C_EnableGenerateStop(I2C1);
        return I2C_NACK;
    }

    /* 16bit 字地址, 高字节在前 */
    I2C_TxData(I2C1, (uint8_t)(memAddr >> 8));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != I2C_OK)
    {
        I2C_EnableGenerateStop(I2C1);
        return I2C_NACK;
    }
    I2C_TxData(I2C1, (uint8_t)(memAddr & 0xFF));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != I2C_OK)
    {
        I2C_EnableGenerateStop(I2C1);
        return I2C_NACK;
    }

    /* 连续写数据: 第1个字节 DR 已空可直接写; 后续每个字节前等 TXBE 空 */
    for (i = 0; i < len; i++)
    {
        if (i > 0)
        {
            if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTING) != I2C_OK)
            {
                I2C_EnableGenerateStop(I2C1);
                return I2C_NACK;
            }
        }
        I2C_TxData(I2C1, pBuf[i]);
    }

    /* 所有字节写完, 等最后一个字节完全移出(BTF) 再发 STOP */
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != I2C_OK)
    {
        I2C_EnableGenerateStop(I2C1);   /* 超时/失败也发 STOP, 释放总线 */
        return I2C_NACK;
    }
    I2C_EnableGenerateStop(I2C1);
    return I2C_OK;
}

/*****************************************************************************
 * @brief  从器件内存地址读多字节(单次完整会话)
 *****************************************************************************/
uint8_t I2C_MemRead(uint8_t devAddr, uint16_t memAddr, uint8_t* pBuf, uint16_t len)
{
    uint16_t i;

    if (len == 0) return I2C_OK;

    /* 1) START + 器件地址(写) + 内存地址 */
    I2C_EnableGenerateStart(I2C1);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != I2C_OK)
    {
        return I2C_NACK;
    }
    I2C_Tx7BitAddress(I2C1, (uint8_t)(devAddr << 1), I2C_DIRECTION_TX);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != I2C_OK)
    {
        I2C_EnableGenerateStop(I2C1);
        return I2C_NACK;
    }
    I2C_TxData(I2C1, (uint8_t)(memAddr >> 8));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != I2C_OK)
    {
        I2C_EnableGenerateStop(I2C1);
        return I2C_NACK;
    }
    I2C_TxData(I2C1, (uint8_t)(memAddr & 0xFF));
    if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != I2C_OK)
    {
        I2C_EnableGenerateStop(I2C1);
        return I2C_NACK;
    }

    /* 2) 重复 START + 器件地址(读) */
    I2C_EnableGenerateStart(I2C1);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != I2C_OK)
    {
        return I2C_NACK;
    }
    I2C_Tx7BitAddress(I2C1, (uint8_t)(devAddr << 1), I2C_DIRECTION_RX);
    if (I2C_WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != I2C_OK)
    {
        I2C_EnableGenerateStop(I2C1);
        return I2C_NACK;
    }

    /* 3) 读 len 字节: 前 len-1 个回 ACK, 最后一个回 NACK */
    I2C_EnableAcknowledge(I2C1);
    for (i = 0; i < len; i++)
    {
        if (I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED) != I2C_OK)
        {
            I2C_EnableGenerateStop(I2C1);
            return I2C_NACK;
        }
        pBuf[i] = I2C_RxData(I2C1);
        /* 读完倒数第二个字节后关闭 ACK, 使最后一个字节收到 NACK */
        if ((len > 1) && (i == len - 2))
        {
            I2C_DisableAcknowledge(I2C1);
        }
    }

    I2C_EnableGenerateStop(I2C1);
    return I2C_OK;
}
