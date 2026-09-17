#ifndef __GT24C64A_H__
#define __GT24C64A_H__
#include "apm32f10c_conf.h"
#include "i2c.h"

/*****************************************************************************
 * GT24C64A EEPROM 驱动 (GIANTEC 聚辰 64Kbit / 8KB, I2C 接口)
 * 底层依赖: i2c.h 硬件 I2C MemWrite/MemRead
 *****************************************************************************/

/* 从机 7bit 器件地址(A2A1A0 接地 = 0x50); 接法不同请修改 */
#define GT24C64A_ADDR        0x50
/* 每页字节数 */
#define GT24C64A_PAGE_SIZE   32U

/* 返回状态 */
#define GT24_OK   1
#define GT24_ERR  0

/* 初始化(内部调用 I2C_Init, 默认 700kHz) */
void GT24C64A_Init(void);

/* 等待上一次写编程完成(ACK 轮询) */
void GT24C64A_WaitWriteComplete(void);

/* 单字节写 */
uint8_t GT24C64A_WriteByte(uint16_t addr, uint8_t data);

/* 多字节写: 自动按页拆分 */
uint8_t GT24C64A_WriteBytes(uint16_t addr, uint8_t* pBuf, uint16_t len);

/* 多字节读 */
uint8_t GT24C64A_ReadBytes(uint16_t addr, uint8_t* pBuf, uint16_t len);

/* EEPROM 读写自检(经 485 打印) */
void GT24C64A_Test(void);

#endif
