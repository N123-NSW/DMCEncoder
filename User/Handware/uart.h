#ifndef __USRT_H__
#define __USRT_H__
#include "apm32f10c_conf.h"
#include "stdio.h"
#include <string.h>

#define BufSize                  64U
extern uint8_t rx_data;
extern uint8_t RX_DATA[32];
extern uint8_t DMA_USART1_TxBuf[BufSize];
/* USART1 RX Buf */
extern uint8_t DMA_USART1_RxBuf[BufSize];
extern volatile uint16_t RX_LEN;

void uart1_Init(uint32_t baudRate);
void UART1_DMASendByte(uint16_t number);
void Encoder_ResetRx(void);


#endif
