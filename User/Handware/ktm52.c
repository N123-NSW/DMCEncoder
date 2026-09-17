#include "ktm52.h"
#include "spi.h"

/*****************************************************************************
 * 延时宏(可按实际芯片/应用调整)
 *****************************************************************************/
#define KTM52_MTP_WRITE_DELAY_MS   2500UL   /* 写 MTP: 芯片要求至少 2.5s */
#define KTM52_CALIB_WAIT_MS        60000UL  /* 自校正等待(转满16圈), 速度低可加长 */
#define KTM52_CALIB_WAIT_FS        20000UL  /* 自校正等待(转满16圈), 速度低可加长 */
#define KTM52_CALIB_POLL_MS        1UL      /* 轮询校准状态间隔 */

/*****************************************************************************
 * CRC8 查表
 *****************************************************************************/
static const uint8_t s_crc8Table[256] =
{
    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
    0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
    0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
    0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
    0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
    0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
    0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
    0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
    0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
    0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
    0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
    0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
    0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
    0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
    0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
    0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};

/*****************************************************************************
 * @brief  计算 CRC8
 * @param  pData: 数据指针
 * @param  len  : 数据字节数(不含末尾 CRC 字节)
 * @retval CRC8 值
 *****************************************************************************/
uint8_t KTM52_CalcCRC8(uint8_t* pData, uint8_t len)
{
    uint8_t crc = 0x00;
    uint8_t i;
    for (i = 0; i < len; i++)
    {
        crc = s_crc8Table[crc ^ pData[i]];
    }
    return crc;
}

/*****************************************************************************
 * @brief  校验 CRC: pBuf[0..len-2] 计算 CRC, 与 pBuf[len-1] 比较
 * @retval KTM52_OK / KTM52_FAIL
 *****************************************************************************/
static uint8_t KTM52_IsCRCSuccess(uint8_t* pBuf, uint8_t bufLen)
{
    return (KTM52_CalcCRC8(pBuf, bufLen - 1) == pBuf[bufLen - 1])
           ? KTM52_OK : KTM52_FAIL;
}

/*****************************************************************************
 * @brief  一次 SPI 字节流全双工收发(CS 由 SPI 底层管理)
 *****************************************************************************/
static void KTM52_SPI_TransmitReceive(uint8_t* ptx, uint8_t* prx, uint8_t size)
{
    SPI_DMA_Transfer (ptx, prx, size);
}

/*****************************************************************************
 * @brief  读角度(原始值 + 状态 + CRC)
 * @param  pAngle: 结果结构体
 * @retval CRC 校验: KTM52_OK / KTM52_FAIL
 *****************************************************************************/
uint8_t KTM52_ReadAngle(KTM52_Angle_t* pAngle)
{
    uint8_t tx[6] = {KTM52_READ_ANGLE, 0, 0, 0, 0, 0};   /* 0xA0 = 读角度命令 */
    uint8_t rx[6];
    uint8_t crcFlag;
    uint32_t data;

    KTM52_SPI_TransmitReceive(tx, rx, 6);
    crcFlag = KTM52_IsCRCSuccess(&rx[2], 4);

    data = ((uint32_t)rx[2] << 16) | ((uint32_t)rx[3] << 8) | rx[4];

    if (pAngle != NULL)
    {
        pAngle->angle = data >> 3;      /* 21 位角度 */
        pAngle->status = data & 0x07;   /* 3 位状态 */
        pAngle->crc_ok = crcFlag;
    }
    return crcFlag;
}

/* ============================ 非阻塞角度读 ============================ */
/* DMA 传输期间缓冲必须是静态(不可用栈变量) */
static uint8_t s_angle_tx[6] = {KTM52_READ_ANGLE, 0, 0, 0, 0, 0};
static uint8_t s_angle_rx[6];

void KTM52_StartAngle(void)
{
    SPI_DMA_StartAsync(s_angle_tx, s_angle_rx, 6);
}

uint8_t KTM52_FinishAngle(KTM52_Angle_t* pAngle)
{
    uint8_t crcFlag;
    uint32_t data;

    while (!SPI_DMA_IsDone());   /* 等待 DMA 完成 */
    SPI_DMA_Finish();            /* 拉高 CS */

    crcFlag = KTM52_IsCRCSuccess(&s_angle_rx[2], 4);

    data = ((uint32_t)s_angle_rx[2] << 16) | ((uint32_t)s_angle_rx[3] << 8) | s_angle_rx[4];

    if (pAngle != NULL)
    {
        pAngle->angle = data >> 3;      /* 21 位角度 */
        pAngle->status = data & 0x07;   /* 3 位状态 */
        pAngle->crc_ok = crcFlag;
    }
    return crcFlag;
}

/*****************************************************************************
 * @brief  读寄存器(带回读 CRC 校验)
 *****************************************************************************/
uint8_t KTM52_ReadReg(uint8_t addr, uint8_t* pRegValue)
{
    uint8_t tx[4] = {KTM52_READ_REG | (addr >> 4), addr << 4, 0, 0};
    uint8_t rx[4];
    uint8_t crcFlag;

    KTM52_SPI_TransmitReceive(tx, rx, 4);
    crcFlag = KTM52_IsCRCSuccess(&rx[2], 2);

    if (pRegValue != NULL)
    {
        *pRegValue = rx[2];
    }
    return crcFlag;
}

/*****************************************************************************
 * @brief  直接读寄存器值(不校验 CRC), 返回寄存器内容
 *****************************************************************************/
uint8_t KTM52_ReadRegRaw(uint8_t addr)
{
    uint8_t tx[4] = {KTM52_READ_REG | (addr >> 4), addr << 4, 0, 0};
    uint8_t rx[4];

    KTM52_SPI_TransmitReceive(tx, rx, 4);
    return rx[2];
}

/*****************************************************************************
 * @brief  写寄存器(写入后读回校验)
 * @retval KTM52_OK / KTM52_FAIL
 *****************************************************************************/
uint8_t KTM52_WriteReg(uint8_t addr, uint8_t data)
{
    uint8_t tx[3] = {KTM52_WRITE_REG | (addr >> 4), addr << 4, data};
    uint8_t rx[3];
    uint8_t readBack = 0;
    uint8_t crcFlag;

    KTM52_SPI_TransmitReceive(tx, rx, 3);
    crcFlag = KTM52_ReadReg(addr, &readBack);

    if ((crcFlag == KTM52_OK) && (readBack == data))
    {
        return KTM52_OK;
    }
    return KTM52_FAIL;
}

/*****************************************************************************
 * @brief  解锁寄存器写保护
 * @retval KTM52_OK(解锁成功) / KTM52_FAIL
 *****************************************************************************/
uint8_t KTM52_UnlockReg(void)
{
    KTM52_WriteReg(KTM52_REG_PROTECT, 0xAA);
    return (KTM52_ReadRegRaw(KTM52_REG_PROTECT) == 0xAA) ? KTM52_OK : KTM52_FAIL;
}

/*****************************************************************************
 * @brief  写寄存器内容到 MTP(非易失), 需延时较长时间
 * @retval KTM52_OK / KTM52_FAIL
 *****************************************************************************/
uint8_t KTM52_WriteMTP(void)
{
    uint8_t tx[3] = {KTM52_WRITE_MTP, 0, 0};
    uint8_t rx[3];

    KTM52_SPI_TransmitReceive(tx, rx, 3);

    Delay_ms(KTM52_MTP_WRITE_DELAY_MS);   /* MTP 写入需 >=2.5s */

    return (rx[2] == 0x55) ? KTM52_OK : KTM52_FAIL;
}

/*****************************************************************************
 * @brief  慢速自校正(闭环自校正)
 * @retval 校准完成状态(2 或 3 表示成功)
 *****************************************************************************/
uint8_t KTM52_SlowSelfCorrection(void)
{
    uint8_t status;

    KTM52_WriteReg(0x5e, 0xac);    /* 慢速校准模式 */
    KTM52_WriteReg(0x5c, 0x00);    /* 线性校准阈值降到最低 */
    KTM52_WriteReg(0x52, 0x11);    /* 一级线性校准使能 */
    KTM52_WriteReg(0x52, 0x51);    /* 一级线性校准补偿参数更新 */
    Delay_ms(KTM52_CALIB_WAIT_MS);
    KTM52_WriteReg(0x5c, 0x90);    /* 线性校准阈值拉到最高 */

    KTM52_WriteReg(0x17, 0x01);    /* 非线性校准启动 */
    do
    {
        status = KTM52_ReadRegRaw(0x16);
        Delay_ms(10);
    } while ((status != 2) && (status != 3));

    KTM52_WriteReg(0x16, 0x00);    /* 校准成功后寄存器复位 */

    return status;
}

/*****************************************************************************
 * @brief  快速自校正(可多次执行迭代)
 * @retval 校准完成状态(2 或 3 表示成功)
 *****************************************************************************/
uint8_t KTM52_FastSelfCorrection(void)
{
    uint8_t status;

    KTM52_WriteReg(0x5e, 0x8e);    /* 快速校准模式 */

    KTM52_WriteReg(0x5c, 0x00);    /* 线性校准阈值降到最低 */
    KTM52_WriteReg(0x52, 0x11);    /* 一级线性校准使能 */
    KTM52_WriteReg(0x52, 0x51);    /* 一级线性校准补偿参数更新 */
    Delay_ms(KTM52_CALIB_WAIT_FS); /* 转满 16 圈所需时间 */
    KTM52_WriteReg(0x5c, 0x90);    /* 线性校准阈值拉到最高 */

    /* 第一次非线性校准 */
    KTM52_WriteReg(0x17, 0x01);
    do
    {
        status = KTM52_ReadRegRaw(0x16);
        Delay_ms(10);
    } while ((status != 2) && (status != 3));
    KTM52_WriteReg(0x17, 0x00);

    /* 第二次非线性校准 */
    KTM52_WriteReg(0x17, 0x01);
    do
    {
        status = KTM52_ReadRegRaw(0x16);
        Delay_ms(10);
    } while ((status != 2) && (status != 3));
    KTM52_WriteReg(0x17, 0x00);

    return status;
}

/*****************************************************************************
 * @brief  软件置零: 向 ZERO_SET(0x12) 写 1, 芯片自动将当前机械角映射为零点
 * @note   完成后该位自动清零; 仅修改 RAM 寄存器, 不会写入 MTP
 * @retval KTM52_OK / KTM52_FAIL
 *****************************************************************************/
uint8_t KTM52_SetZero(void)
{
    /* 写 1 触发置零, 完成后位自动清零 */
    return KTM52_WriteReg(0x12, 0x01);
}

/*****************************************************************************
 * @brief  读 64 点 Golden 查找表(寄存器 0x80~0xFF, 64 个 16 位参数)
 * @param  pGolden: 64 个 uint16_t 的数组指针, 输出 64 个 Golden 值
 * @note   数据格式: 低字节在前(偶地址 0x80+2N), 高字节在后(奇地址 0x81+2N)
 * @retval KTM52_OK / KTM52_FAIL
 *****************************************************************************/
uint8_t KTM52_ReadGoldenTable(uint16_t* pGolden)
{
    uint8_t i;

    if (pGolden == NULL) return KTM52_FAIL;

    for (i = 0; i < 64; i++)
    {
        uint8_t low  = KTM52_ReadRegRaw(0x80 + 2 * i);  /* GLD_N 低字节 */
        uint8_t high = KTM52_ReadRegRaw(0x81 + 2 * i);  /* GLD_N 高字节 */
        pGolden[i] = (uint16_t)(((uint16_t)high << 8) | low);
    }
    return KTM52_OK;
}

/*****************************************************************************
 * @brief  写 64 点 Golden 查找表(寄存器 0x80~0xFF, 64 个 16 位参数)
 * @param  pGolden: 64 个 uint16_t 的数组指针, 输入 64 个待写 Golden 值
 * @note   数据格式: 低字节在前(偶地址 0x80+2N), 高字节在后(奇地址 0x81+2N)
 *         写入需先解锁(0x1A 写 0xAA)
 * @retval KTM52_OK / KTM52_FAIL
 *****************************************************************************/
uint8_t KTM52_WriteGoldenTable(uint16_t* pGolden)
{
    uint8_t i;

    if (pGolden == NULL) return KTM52_FAIL;

    /*写前必须先解锁 */
    if (KTM52_UnlockReg() != KTM52_OK) return KTM52_FAIL;

    for (i = 0; i < 64; i++)
    {
        uint8_t low  = (uint8_t)(pGolden[i] & 0xFF);
        uint8_t high = (uint8_t)((pGolden[i] >> 8) & 0xFF);

        /* 先写低字节, 再写高字节(同一寄存器的两个地址) */
        if (KTM52_WriteReg(0x80 + 2 * i, low) != KTM52_OK) return KTM52_FAIL;
        if (KTM52_WriteReg(0x81 + 2 * i, high) != KTM52_OK) return KTM52_FAIL;
    }
    return KTM52_OK;
}
