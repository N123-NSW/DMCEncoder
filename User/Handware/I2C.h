#ifndef __I2C_H__
#define __I2C_H__
#include "apm32f10c_conf.h"

/*****************************************************************************
 * I2C1 硬件主机底层驱动(APM32 I2C1, 速率可配)
 * 引脚: PB6 = SCL, PB7 = SDA (复用开漏)
 * 采用事件轮询(阻塞)实现完整主机会话:
 *   I2C_MemWrite / I2C_MemRead 适合 EEPROM 等"器件地址+内存地址"访问
 *   I2C_IsDeviceReady 用于写后 ACK 轮询等待
 *****************************************************************************/

/* 返回状态 */
#define I2C_OK    1
#define I2C_NACK  0

/* 初始化 I2C1 为硬件主机 (PB6/PB7), 速率 = clockSpeed Hz(默认 700000) */
void I2C_Init(uint32_t clockSpeed);

/* 写多字节到器件内存地址: devAddr=7bit, memAddr=16bit 字地址 */
uint8_t I2C_MemWrite(uint8_t devAddr, uint16_t memAddr, uint8_t* pBuf, uint16_t len);

/* 从器件内存地址读多字节: devAddr=7bit, memAddr=16bit 字地址 */
uint8_t I2C_MemRead(uint8_t devAddr, uint16_t memAddr, uint8_t* pBuf, uint16_t len);

/* 检测器件是否就绪(ACK 轮询): 器件忙时无 ACK 返回 I2C_NACK */
uint8_t I2C_IsDeviceReady(uint8_t devAddr);

#endif
