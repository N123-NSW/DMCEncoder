#ifndef __485_H__
#define __485_H__
#include "apm32f10c_conf.h"
#include "uart.h"

/*****************************************************************************
 * 485 收发方向控制 + 编码器(Encoder)应用协议层
 *   - CF(Command Field) + 可选 ADF/EDF/DF + CRC
 *   - CRC-8 多项式 x8+1: 对所有数据字节异或
 *
 * 帧格式 (每个数据字: 1起始位+8数据位+1停止位, 低位在前):
 *   ID0 (0x02) 读单圈位置   	Req=[CF]              	Resp=[CF+SF+DF0+DF1+DF2+CRC]
 *   ID2 (0x92) 读 ID         Req=[CF]              Resp=[CF+SF+DF0+CRC]
 *   ID3 (0x1A) 读所有信息    Req=[CF]              Resp=[CF+SF+DF0..DF7+CRC]
 *   ID4 (0x03) 快速自校准	  Req=[CF]              Resp=[CF+CorrectionSatus]
 *   ID5 (0x04) 慢校准		    Req=[CF]              Resp=[CF+CorrectionSatus]
 *   ID6 (0x32) 写 E2PROM     Req=[CF+ADF+EDF+CRC]  Resp=[CF+ADF+EDF+CRC]
 *   IDD (0xEA) 读 E2PROM     Req=[CF+ADF+CRC]      Resp=[CF+ADF+EDF+CRC]
 *   IDC (0xBA) 故障复位      Req=[CF]      				Resp=[CF+SF+DF0+DF1+DF2+CRC]
 *****************************************************************************/

/* 485 收发方向 */
#define RS485_MODE_TX		0
#define RS485_MODE_RX		1

/* 485 底层: 方向控制 + DMA 发送 */
void Encoder_SetDirection(uint16_t dir);
void Encoder_485_Senddata(uint16_t number);

/* ====== CF 命令码 ====== */
#define ENC_CF_ID0  0x02   /* 读单圈位置 */
#define ENC_CF_ID2  0x92   /* 读编码器 ID */
#define ENC_CF_ID3  0x1A   /* 读所有信息 */
#define ENC_CF_ID4  0x03   /* 快速自校准 */
#define ENC_CF_ID5  0x04   /* 慢自校准 */
#define ENC_CF_ID6  0x32   /* 写 E2PROM */
#define ENC_CF_IDD  0xEA   /* 读 E2PROM */
#define ENC_CF_ID7  0x62   /* 多圈位置归零*/
#define ENC_CF_ID8  0xC2   /* 单圈位置归零 */
#define ENC_CF_IDC  0xBA   /* 故障复位 */
#define ENC_CF_CTRL_OFF 0x00 /* 关闭校准/寄存器/golden 操作 */
#define ENC_CF_CTRL_ON  0x01 /* 开启校准/寄存器/golden 操作 */
#define ENC_CF_ID9  0x13   /* 读 KTM52 寄存器 */
#define ENC_CF_IDA  0x12   /* 写 KTM52 寄存器 */
#define ENC_CF_IDB  0x14   /* 读 Golden 值(低字节+高字节) */
#define ENC_CF_IDE  0x15   /* 写 Golden 值(低字节+高字节) */
#define ENC_CF_ID1C 0x1C   /* 读 Golden 全部(64点=128字节) */
#define ENC_CF_ID1D 0x1D   /* 写 Golden 全部(64点=128字节) */

/* ====== EEPROM 页模型 (匹配 TAMAGAWA T-format 手册) ====== */
#define ENC_EEPROM_PAGE_SIZE       127u       /* 每页 127 字节(地址 0~126) */
#define ENC_EEPROM_PAGE_COUNT      8u         /* 有效数据页 0~7 */
#define ENC_EEPROM_PAGE_REG_ADDR   127u       /* 地址 127 = 页寄存器(页切换) */

typedef struct
{
    uint32_t abs_pos;       /* 21bit 单圈绝对位置  */
    uint8_t  abs_crc_ok;    /* KTM52 CRC 校验结果 */

    uint8_t  sf;            /* Status Field 字节 */
    uint8_t  almc;          /* 故障字节 */
    uint8_t  enid;          /* 编码器 ID (默认 0x11) */

    uint8_t  zero_pending;         /* 后台归零待执行标志 */
    uint8_t  alarm_reset_pending;  /* 后台故障复位待执行标志 */
    uint8_t  op_enable;            /* 操作使能: 1=允许校准/寄存器/golden, 0=禁止 */
} Encoder_t;

/* 全局编码器上下文 */
extern Encoder_t g_encoder;
/* Golden 批量写接收缓冲(129字节), uart.c 接收 + ProcessRx 处理共用 */
extern uint8_t s_golden_buf[256];

/* 编码器应用层: 初始化(SPI/I2C + KTM52 解锁) */
void Encoder_Init(void);
/* 主循环: 持续非阻塞读角度并锁存快照 + 后台回写 EEPROM 脏字节 */
void Encoder_AnglePoll(void);
/* 处理 485 收到的一帧 */
void Encoder_ProcessRx(uint8_t* pBuf, uint16_t len);
/* 中断内处理收到的一帧: ID0 零拷贝立即回包, 其余交主循环 */
void Encoder_HandleFrameISR(uint8_t* pBuf, uint16_t len);
/* 返回预组装响应缓冲指针(供 uart.c 中断直接访问) */
uint8_t* Encoder_GetTxID0(void);
uint8_t* Encoder_GetTxID3(void);
/* CRC-8 (x8+1) */
uint8_t Encoder_CalcCRC8(uint8_t* pData, uint16_t len);

/* ====== EEPROM 页模型接口 ====== */
/* 上电加载 EEPROM 全部内容到 RAM 镜像 */
void Encoder_EEPROM_Load(void);

#endif
