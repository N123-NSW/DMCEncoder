#ifndef __KTM52_H__
#define __KTM52_H__
#include "apm32f10c_conf.h"
#include "Rcm.h"   /* 使用 Delay_ms */

/*****************************************************************************
 * KTM5220 角度传感器驱动(SPI 8bit 字节流协议)
 * 底层依赖: spi.h 提供的 SPI_DMA_Transfer()
 *****************************************************************************/

/* SPI 传输缓冲区大小(读角度需要 6 字节) */
#define KTM52_SPI_BUF_SIZE   8U

/* ===== 协议命令字 ===== */
#define KTM52_READ_ANGLE    0xA0   /* 读角度(一次性读多字节) */
#define KTM52_READ_REG      0x30   /* 读寄存器 */
#define KTM52_WRITE_REG     0x60   /* 写寄存器 */
#define KTM52_WRITE_MTP     0xC0   /* 写寄存器到 MTP(非易失) */

/* ===== 返回状态 ===== */
#define KTM52_OK            1      /* 操作成功 / CRC 校验通过 */
#define KTM52_FAIL          0      /* 操作失败 / CRC 校验失败 */

/* ===== 关键寄存器地址 ===== */
#define KTM52_REG_PROTECT   0x1A   /* 寄存器写保护(0xAA 解锁) */
#define KTM52_REG_CAL_STAT  0x16   /* 校准状态 */
#define KTM52_REG_CAL_START 0x17   /* 校准启动 */

/*****************************************************************************
 * 读角度结果(高封装, 方便中转站直接使用)
 *****************************************************************************/
typedef struct
{
    uint32_t angle;      /* 角度原始值 */
    uint8_t  status;     /* 状态位(bit0~2) */
    uint8_t  crc_ok;     /* CRC8 校验: KTM52_OK / KTM52_FAIL */
} KTM52_Angle_t;

/*****************************************************************************
 * 对外功能接口
 *****************************************************************************/

/* 读角度, 结果写入 pAngle */
uint8_t KTM52_ReadAngle(KTM52_Angle_t* pAngle);

/* 非阻塞角度读: Start 启动 DMA 读(立即返回), Finish 等待完成并解析 */
void KTM52_StartAngle(void);
uint8_t KTM52_FinishAngle(KTM52_Angle_t* pAngle);

/* 读寄存器: 地址 8bit, 值写回 *pRegValue, 返回 CRC 结果 */
uint8_t KTM52_ReadReg(uint8_t addr, uint8_t* pRegValue);

/* 写寄存器: 写入后读回校验, 成功返回 KTM52_OK */
uint8_t KTM52_WriteReg(uint8_t addr, uint8_t data);

/* 直接读寄存器值(不校验 CRC), 返回寄存器内容 */
uint8_t KTM52_ReadRegRaw(uint8_t addr);

/* 解锁寄存器写保护(写入 0xAA 到保护寄存器), 成功返回 KTM52_OK */
uint8_t KTM52_UnlockReg(void);

/* 把当前寄存器内容写入 MTP(非易失, 需较长时间), 成功返回 KTM52_OK */
uint8_t KTM52_WriteMTP(void);

/* 慢速自校正(闭环/开环自校正均可用) */
uint8_t KTM52_SlowSelfCorrection(void);

/* 快速自校正(可多次执行迭代) */
uint8_t KTM52_FastSelfCorrection(void);

/* 软件置零: 向 ZERO_SET(0x12) 写 1, 芯片自动将当前机械角映射为零点 */
uint8_t KTM52_SetZero(void);

/* 读 64 点 Golden 查找表(0x80~0xFF), 64 个 uint16_t */
uint8_t KTM52_ReadGoldenTable(uint16_t* pGolden);

/* 写 64 点 Golden 查找表(0x80~0xFF), 64 个 uint16_t, 需先解锁 */
uint8_t KTM52_WriteGoldenTable(uint16_t* pGolden);

/* CRC8 计算接口(供上层/测试复用) */
uint8_t KTM52_CalcCRC8(uint8_t* pData, uint8_t len);

#endif
