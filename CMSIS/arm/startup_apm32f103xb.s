;
;/**
; * @file       startup_apm32f103xb.s
; *
; * @brief      CMSIS Cortex-M3 based Core Device Startup File for Device startup_apm32f103xb.s
; *
; * @version    V1.0.0
; *
; * @date       2026-03-01
; *
; * @attention
; *
; *  Copyright (C) 2026 Geehy Semiconductor
; *
; *  You may not use this file except in compliance with the
; *  GEEHY COPYRIGHT NOTICE (GEEHY SOFTWARE PACKAGE LICENSE).
; *
; *  The program is only for reference, which is distributed in the hope
; *  that it will be useful and instructional for customers to develop
; *  their software. Unless required by applicable law or agreed to in
; *  writing, the program is distributed on an "AS IS" BASIS, WITHOUT
; *  ANY WARRANTY OR CONDITIONS OF ANY KIND, either express or implied.
; *  See the GEEHY SOFTWARE PACKAGE LICENSE for the governing permissions
; *  and limitations under the License.
; */

; <h> Stack Configuration
;  <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Stack_Size      EQU     0x00000400

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp

; <h> Heap Configuration
;   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
; </h>

Heap_Size       EQU     0x00000200

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

                PRESERVE8
                THUMB

; Vector Table Mapped to Address 0 at Reset
                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp              ; Top of Stack
                DCD     Reset_Handler             ; Reset Handler
                DCD     NMI_Handler               ; < Non Maskable Interrupt
                DCD     HardFault_Handler         ; < Hard Fault, all classes of Fault
                DCD     MemManage_Handler         ; < Memory Management Interrupt
                DCD     BusFault_Handler          ; < Bus Fault Interrupt
                DCD     UsageFault_Handler        ; < Usage Fault Interrupt
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler               ; < SV Call Interrupt
                DCD     DebugMon_Handler          ; < Debug Monitor Interrupt
                DCD     0                         ; Reserved
                DCD     PendSV_Handler            ; < Pend SV Interrupt
                DCD     SysTick_Handler           ; < System Tick Interrupt

                ; External Interrupts
                DCD     WWDT_IRQHandler           ; < Window WatchDog Interrupt
                DCD     PVD_IRQHandler            ; < PVD through EINT Line detection Interrupt
                DCD     TAMPER_IRQHandler         ; < Tamper Interrupt
                DCD     RTC_IRQHandler            ; < RTC global Interrupt
                DCD     FLASH_IRQHandler          ; < FLASH global Interrupt
                DCD     RCM_IRQHandler            ; < RCM global Interrupt
                DCD     EINT0_IRQHandler          ; < EINT Line0 Interrupt
                DCD     EINT1_IRQHandler          ; < EINT Line1 Interrupt
                DCD     EINT2_IRQHandler          ; < EINT Line2 Interrupt
                DCD     EINT3_IRQHandler          ; < EINT Line3 Interrupt
                DCD     EINT4_IRQHandler          ; < EINT Line4 Interrupt
                DCD     DMA1_Channel1_IRQHandler  ; < DMA1 Channel 1 global Interrupt
                DCD     DMA1_Channel2_IRQHandler  ; < DMA1 Channel 2 global Interrupt
                DCD     DMA1_Channel3_IRQHandler  ; < DMA1 Channel 3 global Interrupt
                DCD     DMA1_Channel4_IRQHandler  ; < DMA1 Channel 4 global Interrupt
                DCD     DMA1_Channel5_IRQHandler  ; < DMA1 Channel 5 global Interrupt
                DCD     DMA1_Channel6_IRQHandler  ; < DMA1 Channel 6 global Interrupt
                DCD     DMA1_Channel7_IRQHandler  ; < DMA1 Channel 7 global Interrupt
                DCD     ADC1_2_IRQHandler         ; < ADC1 and ADC2 global Interrupt
                DCD     USBD1_HP_CAN1_TX_IRQHandler ; < USB Device 1 High Priority or CAN1 TX Interrupts
                DCD     USBD1_LP_CAN1_RX0_IRQHandler ; < USB Device 1 Low Priority or CAN1 RX0 Interrupts
                DCD     CAN1_RX1_IRQHandler       ; < CAN1 RX1 Interrupt
                DCD     CAN1_SCE_IRQHandler       ; < CAN1 SCE Interrupt
                DCD     EINT9_5_IRQHandler        ; < External Line[9:5] Interrupts
                DCD     TMR1_BRK_IRQHandler       ; < TMR1 Break Interrupt
                DCD     TMR1_UP_IRQHandler        ; < TMR1 Update Interrupt
                DCD     TMR1_TRG_COM_IRQHandler   ; < TMR1 Trigger and Commutation Interrupt
                DCD     TMR1_CC_IRQHandler        ; < TMR1 Capture Compare Interrupt
                DCD     TMR2_IRQHandler           ; < TMR2 global Interrupt
                DCD     TMR3_IRQHandler           ; < TMR3 global Interrupt
                DCD     TMR4_IRQHandler           ; < TMR4 global Interrupt
                DCD     I2C1_EV_IRQHandler        ; < I2C1 Event Interrupt
                DCD     I2C1_ER_IRQHandler        ; < I2C1 Error Interrupt
                DCD     I2C2_EV_IRQHandler        ; < I2C2 Event Interrupt
                DCD     I2C2_ER_IRQHandler        ; < I2C2 Error Interrupt
                DCD     SPI1_IRQHandler           ; < SPI1 global Interrupt
                DCD     SPI2_IRQHandler           ; < SPI2 global Interrupt
                DCD     USART1_IRQHandler         ; < USART1 global Interrupt
                DCD     USART2_IRQHandler         ; < USART2 global Interrupt
                DCD     USART3_IRQHandler         ; < USART3 global Interrupt
                DCD     EINT15_10_IRQHandler      ; < External Line[15:10] Interrupts
                DCD     RTCAlarm_IRQHandler       ; < RTC Alarm through EINT Line Interrupt
                DCD     USBDWakeUp_IRQHandler     ; < USB Device WakeUp from suspend through EINT Line Interrupt
                DCD     FPU_IRQHandler            ; < FPU Global Interrupt
                DCD     QSPI_IRQHandler           ; < QSPI Global Interrupt
                DCD     USBD2_HP_IRQHandler       ; < USB Device 2 High Priority
                DCD     USBD2_LP_IRQHandler       ; < USB Device 2 Low Priority
                DCD     CAN2_RX1_IRQHandler       ; < CAN2 RX1 Interrupts
                DCD     CAN2_SCE_IRQHandler       ; < CAN2 SCE Interrupts

__Vectors_End

__Vectors_Size  EQU  __Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY

; Reset handler routine
Reset_Handler   PROC
                EXPORT  Reset_Handler             [WEAK]
        IMPORT  __main
        IMPORT  SystemInit
                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP

NMI_Handler     PROC
                EXPORT  NMI_Handler               [WEAK]
                B       .
                ENDP
HardFault_Handler     PROC
                EXPORT  HardFault_Handler         [WEAK]
                B       .
                ENDP
MemManage_Handler     PROC
                EXPORT  MemManage_Handler         [WEAK]
                B       .
                ENDP
BusFault_Handler     PROC
                EXPORT  BusFault_Handler          [WEAK]
                B       .
                ENDP
UsageFault_Handler     PROC
                EXPORT  UsageFault_Handler        [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler               [WEAK]
                B       .
                ENDP
DebugMon_Handler     PROC
                EXPORT  DebugMon_Handler          [WEAK]
                B       .
                ENDP
PendSV_Handler     PROC
                EXPORT  PendSV_Handler            [WEAK]
                B       .
                ENDP
SysTick_Handler     PROC
                EXPORT  SysTick_Handler           [WEAK]
                B       .
                ENDP

Default_Handler PROC
                EXPORT  WWDT_IRQHandler           [WEAK]
                EXPORT  PVD_IRQHandler            [WEAK]
                EXPORT  TAMPER_IRQHandler         [WEAK]
                EXPORT  RTC_IRQHandler            [WEAK]
                EXPORT  FLASH_IRQHandler          [WEAK]
                EXPORT  RCM_IRQHandler            [WEAK]
                EXPORT  EINT0_IRQHandler          [WEAK]
                EXPORT  EINT1_IRQHandler          [WEAK]
                EXPORT  EINT2_IRQHandler          [WEAK]
                EXPORT  EINT3_IRQHandler          [WEAK]
                EXPORT  EINT4_IRQHandler          [WEAK]
                EXPORT  DMA1_Channel1_IRQHandler  [WEAK]
                EXPORT  DMA1_Channel2_IRQHandler  [WEAK]
                EXPORT  DMA1_Channel3_IRQHandler  [WEAK]
                EXPORT  DMA1_Channel4_IRQHandler  [WEAK]
                EXPORT  DMA1_Channel5_IRQHandler  [WEAK]
                EXPORT  DMA1_Channel6_IRQHandler  [WEAK]
                EXPORT  DMA1_Channel7_IRQHandler  [WEAK]
                EXPORT  ADC1_2_IRQHandler         [WEAK]
                EXPORT  USBD1_HP_CAN1_TX_IRQHandler [WEAK]
                EXPORT  USBD1_LP_CAN1_RX0_IRQHandler [WEAK]
                EXPORT  CAN1_RX1_IRQHandler       [WEAK]
                EXPORT  CAN1_SCE_IRQHandler       [WEAK]
                EXPORT  EINT9_5_IRQHandler        [WEAK]
                EXPORT  TMR1_BRK_IRQHandler       [WEAK]
                EXPORT  TMR1_UP_IRQHandler        [WEAK]
                EXPORT  TMR1_TRG_COM_IRQHandler   [WEAK]
                EXPORT  TMR1_CC_IRQHandler        [WEAK]
                EXPORT  TMR2_IRQHandler           [WEAK]
                EXPORT  TMR3_IRQHandler           [WEAK]
                EXPORT  TMR4_IRQHandler           [WEAK]
                EXPORT  I2C1_EV_IRQHandler        [WEAK]
                EXPORT  I2C1_ER_IRQHandler        [WEAK]
                EXPORT  I2C2_EV_IRQHandler        [WEAK]
                EXPORT  I2C2_ER_IRQHandler        [WEAK]
                EXPORT  SPI1_IRQHandler           [WEAK]
                EXPORT  SPI2_IRQHandler           [WEAK]
                EXPORT  USART1_IRQHandler         [WEAK]
                EXPORT  USART2_IRQHandler         [WEAK]
                EXPORT  USART3_IRQHandler         [WEAK]
                EXPORT  EINT15_10_IRQHandler      [WEAK]
                EXPORT  RTCAlarm_IRQHandler       [WEAK]
                EXPORT  USBDWakeUp_IRQHandler     [WEAK]
                EXPORT  FPU_IRQHandler            [WEAK]
                EXPORT  QSPI_IRQHandler           [WEAK]
                EXPORT  USBD2_HP_IRQHandler       [WEAK]
                EXPORT  USBD2_LP_IRQHandler       [WEAK]
                EXPORT  CAN2_RX1_IRQHandler       [WEAK]
                EXPORT  CAN2_SCE_IRQHandler       [WEAK]
WWDT_IRQHandler
PVD_IRQHandler
TAMPER_IRQHandler
RTC_IRQHandler
FLASH_IRQHandler
RCM_IRQHandler
EINT0_IRQHandler
EINT1_IRQHandler
EINT2_IRQHandler
EINT3_IRQHandler
EINT4_IRQHandler
DMA1_Channel1_IRQHandler
DMA1_Channel2_IRQHandler
DMA1_Channel3_IRQHandler
DMA1_Channel4_IRQHandler
DMA1_Channel5_IRQHandler
DMA1_Channel6_IRQHandler
DMA1_Channel7_IRQHandler
ADC1_2_IRQHandler
USBD1_HP_CAN1_TX_IRQHandler
USBD1_LP_CAN1_RX0_IRQHandler
CAN1_RX1_IRQHandler
CAN1_SCE_IRQHandler
EINT9_5_IRQHandler
TMR1_BRK_IRQHandler
TMR1_UP_IRQHandler
TMR1_TRG_COM_IRQHandler
TMR1_CC_IRQHandler
TMR2_IRQHandler
TMR3_IRQHandler
TMR4_IRQHandler
I2C1_EV_IRQHandler
I2C1_ER_IRQHandler
I2C2_EV_IRQHandler
I2C2_ER_IRQHandler
SPI1_IRQHandler
SPI2_IRQHandler
USART1_IRQHandler
USART2_IRQHandler
USART3_IRQHandler
EINT15_10_IRQHandler
RTCAlarm_IRQHandler
USBDWakeUp_IRQHandler
FPU_IRQHandler
QSPI_IRQHandler
USBD2_HP_IRQHandler
USBD2_LP_IRQHandler
CAN2_RX1_IRQHandler
CAN2_SCE_IRQHandler
                B       .
                ENDP

                ALIGN

;*******************************************************************************
; User Stack and Heap initialization
;*******************************************************************************
                 IF      :DEF:__MICROLIB

                 EXPORT  __initial_sp
                 EXPORT  __heap_base
                 EXPORT  __heap_limit

                 ELSE

                 IMPORT  __use_two_region_memory
                 EXPORT  __user_initial_stackheap

__user_initial_stackheap

                 LDR     R0, =  Heap_Mem
                 LDR     R1, =(Stack_Mem + Stack_Size)
                 LDR     R2, = (Heap_Mem +  Heap_Size)
                 LDR     R3, = Stack_Mem
                 BX      LR

                 ALIGN

                 ENDIF

                 END

;************************ (C) COPYRIGHT Geehy Semiconductor Co.,Ltd *****END OF FILE*****