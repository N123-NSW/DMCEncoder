#include "gt24c64a.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "uart.h"
#include "485.h"   /* 测试函数经 485 打印用 */

/* 默认 I2C 速率: 700kHz (GT24C64A 支持 ≤1MHz) */
#define GT24C64A_I2C_SPEED  700000UL

/*****************************************************************************
 * @brief  初始化: I2C_Init(硬件 I2C1, 700kHz)
 *****************************************************************************/
void GT24C64A_Init(void)
{
    I2C_Init(GT24C64A_I2C_SPEED);
}

/*****************************************************************************
 * @brief  等待上一次写编程完成(ACK 轮询)
 *****************************************************************************/
void GT24C64A_WaitWriteComplete(void)
{
    while (I2C_IsDeviceReady(GT24C64A_ADDR) != I2C_OK)
    {
        /* 器件忙, 继续轮询 */
    }
}

/*****************************************************************************
 * @brief  单字节写
 *****************************************************************************/
uint8_t GT24C64A_WriteByte(uint16_t addr, uint8_t data)
{
    if (I2C_MemWrite(GT24C64A_ADDR, addr, &data, 1) != I2C_OK)
    {
        return GT24_ERR;
    }
    GT24C64A_WaitWriteComplete();
    return GT24_OK;
}

/*****************************************************************************
 * @brief  多字节写, 自动按页拆分(每页 GT24C64A_PAGE_SIZE 字节), 长度不限
 *****************************************************************************/
uint8_t GT24C64A_WriteBytes(uint16_t addr, uint8_t* pBuf, uint16_t len)
{
    while (len > 0)
    {
        uint16_t pageLeft = GT24C64A_PAGE_SIZE - (addr % GT24C64A_PAGE_SIZE);
        uint16_t chunk = (len < pageLeft) ? len : pageLeft;

        if (I2C_MemWrite(GT24C64A_ADDR, addr, pBuf, chunk) != I2C_OK)
        {
            return GT24_ERR;
        }
        GT24C64A_WaitWriteComplete();

        addr += chunk;
        pBuf += chunk;
        len -= chunk;
    }
    return GT24_OK;
}

/*****************************************************************************
 * @brief  多字节读(随机读), 顺序递增, 长度不限
 *****************************************************************************/
uint8_t GT24C64A_ReadBytes(uint16_t addr, uint8_t* pBuf, uint16_t len)
{
    if (len == 0) return GT24_OK;
    if (I2C_MemRead(GT24C64A_ADDR, addr, pBuf, len) != I2C_OK)
    {
        return GT24_ERR;
    }
    return GT24_OK;
}

/* ============================ EEPROM 自检函数 ============================ */

/* 测试打印辅助: 格式化到 485 发送缓冲并发出 */
static void GT24C64A_TestPrint(const char* fmt, ...)
{
    int n;
    va_list args;

    va_start(args, fmt);
    n = vsnprintf((char*)DMA_USART1_TxBuf, BufSize, fmt, args);
    va_end(args);

    if (n > 0)
    {
        if (n >= (int)BufSize) n = BufSize - 1;
        Encoder_485_Senddata((uint16_t)n);
    }
}

/*****************************************************************************
 * @brief  EEPROM 读写自检, 经 485 打印结果
 *****************************************************************************/
void GT24C64A_Test(void)
{
    uint8_t wbuf[40];
    uint8_t rbuf[40] = {0};
    uint8_t i;
    uint8_t byteOk, pageOk;

    GT24C64A_TestPrint("=== GT24C64A Test ===\r\n");

    /* 1) 单字节写 + 读回 */
    GT24C64A_WriteByte(0x0000, 0xAB);
    GT24C64A_ReadBytes(0x0000, rbuf, 1);
    byteOk = (rbuf[0] == 0xAB);
    GT24C64A_TestPrint("ByteW 0x00=AB -> read=%02X %s\r\n", rbuf[0],
                       byteOk ? "OK" : "FAIL");

    /* 2) 跨页多字节写 + 读回 */
    for (i = 0; i < 40; i++)
    {
        wbuf[i] = (uint8_t)i;
    }
    pageOk = GT24C64A_WriteBytes(0x0010, wbuf, 40);
    memset(rbuf, 0, sizeof(rbuf));
    if (GT24C64A_ReadBytes(0x0010, rbuf, 40) != GT24_OK)
    {
        pageOk = 0;
    }
    else
    {
        for (i = 0; i < 40; i++)
        {
            if (rbuf[i] != wbuf[i])
            {
                pageOk = 0;
                break;
            }
        }
    }
    GT24C64A_TestPrint("PageWrite/Read @0x10 len40 %s\r\n",
                       pageOk ? "OK" : "FAIL");

    /* 3) 打印读回前 12 字节 */
    GT24C64A_TestPrint("Rd: ");
    for (i = 0; i < 12; i++)
    {
        GT24C64A_TestPrint("%02X ", rbuf[i]);
    }
    GT24C64A_TestPrint("\r\n");

    GT24C64A_TestPrint("=== Test End ===\r\n");
}
