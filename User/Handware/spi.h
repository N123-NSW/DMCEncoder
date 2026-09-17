#ifndef __SPI_H__
#define __SPI_H__
#include "apm32f10c_conf.h"

/*****************************************************************************
 * SPI1 底层(8bit, 主机, SPI 模式3, MSB first)
 * 支持 DMA 多字节收发/发送, 以及单字节收发
 * CS(PA4) 由本层自动管理: 每帧拉低一次, 完成后拉高
 *****************************************************************************/

/* CS 引脚 */
#define SPI_CS_PORT   GPIOA
#define SPI_CS_PIN    GPIO_PIN_4

/* SPI DMA 单次最大长度 */
#define SPI_DMA_MAX_LEN  32U

/* 初始化 SPI1(8bit, 模式3) 及 TX/RX DMA */
void SPI_Init(void);

/* DMA 多字节全双工收发: 同时发 len 字节并收 len 字节(自动 CS) */
void SPI_DMA_Transfer(uint8_t* txBuf, uint8_t* rxBuf, uint16_t len);

/* DMA 多字节发送: 只发 len 字节(自动 CS) */
void SPI_DMA_Send(uint8_t* txBuf, uint16_t len);

/* 单字节全双工收发(自动 CS), 返回接收字节 */
uint8_t SPI_ReadWriteByte(uint8_t txData);

/* 非阻塞 DMA 全双工收发: 启动后立即返回, 配合 IsDone/Finish 使用 */
void SPI_DMA_StartAsync(uint8_t* txBuf, uint8_t* rxBuf, uint16_t len);
uint8_t SPI_DMA_IsDone(void);
void SPI_DMA_Finish(void);

#endif
