#include "spi.h"

#define SPI1_DMA_TX_CH       DMA1_Channel3   /* SPI1_TX -> DMA1_Channel3 */
#define SPI1_DMA_RX_CH       DMA1_Channel2   /* SPI1_RX -> DMA1_Channel2 */
#define SPI1_DMA_TX_TC_FLAG  DMA1_FLAG_TC3
#define SPI1_DMA_RX_TC_FLAG  DMA1_FLAG_TC2

/* 只发模式下用于接收的哑缓冲(避免 SPI RX 溢出) */
static uint8_t spi_dummy_rx[SPI_DMA_MAX_LEN];

/*****************************************************************************
 * @brief  初始化 SPI1(8bit, 模式3, MSB first) 及 TX/RX DMA
 *****************************************************************************/
void SPI_Init(void)
{
    GPIO_Config_T gpioConfig;
    SPI_Config_T spiConfig;
    DMA_Config_T dmaConfig;

    /* 使能时钟 */
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_SPI1);
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_GPIOA);
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);

    /* CS: PA4 推挽输出, 空闲拉高 */
    gpioConfig.pin = GPIO_PIN_4;
    gpioConfig.mode = GPIO_MODE_OUT_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    GPIO_Config(GPIOA, &gpioConfig);
    GPIO_SetBit(GPIOA, GPIO_PIN_4);

    /* MISO: PA6 输入上拉 */
    gpioConfig.pin = GPIO_PIN_6;
    gpioConfig.mode = GPIO_MODE_IN_PU;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    GPIO_Config(GPIOA, &gpioConfig);

    /* SCK: PA5, MOSI: PA7 复用推挽 */
    gpioConfig.pin = GPIO_PIN_5 | GPIO_PIN_7;
    gpioConfig.mode = GPIO_MODE_AF_PP;
    gpioConfig.speed = GPIO_SPEED_50MHz;
    GPIO_Config(GPIOA, &gpioConfig);

    /* SPI 配置: 8bit, 模式3(CPOL=1, CPHA=1), 主机, MSB first */
    SPI_ConfigStructInit(&spiConfig);
    spiConfig.length = SPI_DATA_LENGTH_8B;
    spiConfig.baudrateDiv = SPI_BAUDRATE_DIV_16;
    spiConfig.direction = SPI_DIRECTION_2LINES_FULLDUPLEX;
    spiConfig.firstBit = SPI_FIRSTBIT_MSB;
    spiConfig.mode = SPI_MODE_MASTER;
    spiConfig.polarity = SPI_CLKPOL_HIGH;   /* CPOL=1 */
    spiConfig.nss = SPI_NSS_SOFT;
    spiConfig.phase = SPI_CLKPHA_2EDGE;     /* CPHA=1 */
    SPI_Config(SPI1, &spiConfig);
    SPI_ConfigDataSize(SPI1, SPI_DATA_LENGTH_8B);
    SPI_Enable(SPI1);

    /* ---- SPI1_TX: DMA1_Channel3, 内存 -> 外设, 8bit ---- */
    DMA_ConfigStructInit(&dmaConfig);
    dmaConfig.peripheralBaseAddr = (uint32_t)&SPI1->DATA;
    dmaConfig.memoryBaseAddr     = (uint32_t)0;
    dmaConfig.dir                = DMA_DIR_PERIPHERAL_DST;
    dmaConfig.bufferSize         = 0;
    dmaConfig.peripheralInc      = DMA_PERIPHERAL_INC_DISABLE;
    dmaConfig.memoryInc          = DMA_MEMORY_INC_ENABLE;
    dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_BYTE;
    dmaConfig.memoryDataSize     = DMA_MEMORY_DATA_SIZE_BYTE;
    dmaConfig.loopMode           = DMA_MODE_NORMAL;
    dmaConfig.priority           = DMA_PRIORITY_HIGH;
    dmaConfig.M2M                = DMA_M2MEN_DISABLE;
    DMA_Config(SPI1_DMA_TX_CH, &dmaConfig);
    DMA_Disable(SPI1_DMA_TX_CH);
    SPI_I2S_EnableDMA(SPI1, SPI_I2S_DMA_REQ_TX);

    /* ---- SPI1_RX: DMA1_Channel2, 外设 -> 内存, 8bit ---- */
    DMA_ConfigStructInit(&dmaConfig);
    dmaConfig.peripheralBaseAddr = (uint32_t)&SPI1->DATA;
    dmaConfig.memoryBaseAddr     = (uint32_t)0;
    dmaConfig.dir                = DMA_DIR_PERIPHERAL_SRC;
    dmaConfig.bufferSize         = 0;
    dmaConfig.peripheralInc      = DMA_PERIPHERAL_INC_DISABLE;
    dmaConfig.memoryInc          = DMA_MEMORY_INC_ENABLE;
    dmaConfig.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_BYTE;
    dmaConfig.memoryDataSize     = DMA_MEMORY_DATA_SIZE_BYTE;
    dmaConfig.loopMode           = DMA_MODE_NORMAL;
    dmaConfig.priority           = DMA_PRIORITY_HIGH;
    dmaConfig.M2M                = DMA_M2MEN_DISABLE;
    DMA_Config(SPI1_DMA_RX_CH, &dmaConfig);
    DMA_Disable(SPI1_DMA_RX_CH);
    SPI_I2S_EnableDMA(SPI1, SPI_I2S_DMA_REQ_RX);

    DMA_ClearStatusFlag(SPI1_DMA_TX_TC_FLAG);
    DMA_ClearStatusFlag(SPI1_DMA_RX_TC_FLAG);
}

/*****************************************************************************
 * @brief  DMA 多字节全双工收发: 同时发 len 字节并收 len 字节(自动 CS)
 *****************************************************************************/
void SPI_DMA_Transfer(uint8_t* txBuf, uint8_t* rxBuf, uint16_t len)
{
    if (len == 0 || len > SPI_DMA_MAX_LEN) return;

    /* 关闭并重配 TX/RX DMA 地址与长度 */
    DMA_Disable(SPI1_DMA_TX_CH);
    DMA_Disable(SPI1_DMA_RX_CH);

    DMA1_Channel3->CHMADDR = (uint32_t)txBuf;
    DMA_ConfigDataNumber(SPI1_DMA_TX_CH, len);

    DMA1_Channel2->CHMADDR = (uint32_t)rxBuf;
    DMA_ConfigDataNumber(SPI1_DMA_RX_CH, len);

    DMA_ClearStatusFlag(SPI1_DMA_TX_TC_FLAG);
    DMA_ClearStatusFlag(SPI1_DMA_RX_TC_FLAG);

    GPIO_ResetBit(SPI_CS_PORT, SPI_CS_PIN);   /* CS 拉低 */

    DMA_Enable(SPI1_DMA_RX_CH);
    DMA_Enable(SPI1_DMA_TX_CH);

    /* 等待收发完成 */
    while (DMA_ReadStatusFlag(SPI1_DMA_TX_TC_FLAG) == RESET);
    while (DMA_ReadStatusFlag(SPI1_DMA_RX_TC_FLAG) == RESET);
    while (SPI_I2S_ReadStatusFlag(SPI1, SPI_FLAG_BSY) == SET);

    DMA_Disable(SPI1_DMA_TX_CH);
    DMA_Disable(SPI1_DMA_RX_CH);

    GPIO_SetBit(SPI_CS_PORT, SPI_CS_PIN);     /* CS 拉高 */
}

/*****************************************************************************
 * @brief  DMA 多字节发送: 只发 len 字节(自动 CS)
 * @note   内部用哑接收缓冲读走 MISO, 避免 SPI RX 溢出
 *****************************************************************************/
void SPI_DMA_Send(uint8_t* txBuf, uint16_t len)
{
    SPI_DMA_Transfer(txBuf, spi_dummy_rx, len);
}

/*****************************************************************************
 * @brief  单字节全双工收发(轮询, 自动 CS)
 * @retval 接收字节
 *****************************************************************************/
uint8_t SPI_ReadWriteByte(uint8_t txData)
{
    uint8_t rxData;

    GPIO_ResetBit(SPI_CS_PORT, SPI_CS_PIN);

    while (SPI_I2S_ReadStatusFlag(SPI1, SPI_FLAG_TXBE) == RESET);
    SPI_I2S_TxData(SPI1, txData);

    while (SPI_I2S_ReadStatusFlag(SPI1, SPI_FLAG_RXBNE) == RESET);
    rxData = (uint8_t)SPI_I2S_RxData(SPI1);

    while (SPI_I2S_ReadStatusFlag(SPI1, SPI_FLAG_BSY) == SET);

    GPIO_SetBit(SPI_CS_PORT, SPI_CS_PIN);

    return rxData;
}

/* ============================ 非阻塞 DMA 传输 ============================ */
static uint8_t s_spi_async = 0;

void SPI_DMA_StartAsync(uint8_t* txBuf, uint8_t* rxBuf, uint16_t len)
{
    if (len == 0 || len > SPI_DMA_MAX_LEN) return;

    DMA_Disable(SPI1_DMA_TX_CH);
    DMA_Disable(SPI1_DMA_RX_CH);

    DMA1_Channel3->CHMADDR = (uint32_t)txBuf;
    DMA_ConfigDataNumber(SPI1_DMA_TX_CH, len);
    DMA1_Channel2->CHMADDR = (uint32_t)rxBuf;
    DMA_ConfigDataNumber(SPI1_DMA_RX_CH, len);

    DMA_ClearStatusFlag(SPI1_DMA_TX_TC_FLAG);
    DMA_ClearStatusFlag(SPI1_DMA_RX_TC_FLAG);

    GPIO_ResetBit(SPI_CS_PORT, SPI_CS_PIN);   /* CS 拉低 */

    DMA_Enable(SPI1_DMA_RX_CH);
    DMA_Enable(SPI1_DMA_TX_CH);
    s_spi_async = 1;
}

uint8_t SPI_DMA_IsDone(void)
{
    if (!s_spi_async) return 1;
    return (DMA_ReadStatusFlag(SPI1_DMA_TX_TC_FLAG) == SET &&
            DMA_ReadStatusFlag(SPI1_DMA_RX_TC_FLAG) == SET &&
            SPI_I2S_ReadStatusFlag(SPI1, SPI_FLAG_BSY) == RESET) ? 1 : 0;
}

void SPI_DMA_Finish(void)
{
    DMA_Disable(SPI1_DMA_TX_CH);
    DMA_Disable(SPI1_DMA_RX_CH);
    GPIO_SetBit(SPI_CS_PORT, SPI_CS_PIN);     /* CS 拉高 */
    s_spi_async = 0;
}
