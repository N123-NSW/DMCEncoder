/*!
 * @file        apm32f10x.h
 *
 * @brief       CMSIS Cortex-M3 Device Peripheral Access Layer Header File.
 *
 * @details     This file contains all the peripheral register's definitions, bits definitions and memory mapping
 *
 * @version     V2.0.0
 *
 * @date        2022-12-01
 *
 * @attention
 *
 *  Copyright (C) 2020-2022 Geehy Semiconductor
 *
 *  You may not use this file except in compliance with the
 *  GEEHY COPYRIGHT NOTICE (GEEHY SOFTWARE PACKAGE LICENSE).
 *
 *  The program is only for reference, which is distributed in the hope
 *  that it will be useful and instructional for customers to develop
 *  their software. Unless required by applicable law or agreed to in
 *  writing, the program is distributed on an "AS IS" BASIS, WITHOUT
 *  ANY WARRANTY OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the GEEHY SOFTWARE PACKAGE LICENSE for the governing permissions
 *  and limitations under the License.
 */

/* Define to prevent recursive inclusion */
#ifndef __APM32F10X_H
#define __APM32F10X_H

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup CMSIS
  @{
*/

/** @addtogroup APM32F10x
  * @brief Peripheral Access Layer
  @{
*/

/** @defgroup APM32F10x_StdPeripheral_Library_Version
  @{
*/

/* Uncomment the line below according to the target APM32 device used in your
   application
  */
#if !defined (APM32F103x4) && !defined (APM32F103x6) && !defined (APM32F103x8) && \
    !defined (APM32F103xB) && !defined (APM32F103xC) && !defined (APM32F103xD) && \
    !defined (APM32F103xE) && !defined (APM32F105xB) && !defined (APM32F105xC) && \
    !defined (APM32F107xB) && !defined (APM32F107xC)
    /* #define APM32F103x4 */   /*!< APM32F103T4, APM32F103C4 and APM32F103R4 Devices */
    /* #define APM32F103x6 */   /*!< APM32F103T6, APM32F103C6 and APM32F103R6 Devices */
    /* #define APM32F103x8 */   /*!< APM32F103T8, APM32F103C8 and APM32F103R8 Devices */
    /* #define APM32F103xB */   /*!< APM32F103TB, APM32F103CB, APM32F103RB and APM32F103VB Devices */
    /* #define APM32F103xC */   /*!< APM32F103CC, APM32F103RC and APM32F103VC Devices */
    /* #define APM32F103xD */   /*!< APM32F103RD, APM32F103VD and APM32F103ZD Devices */
    /* #define APM32F103xE */   /*!< APM32F103RE, APM32F103VE and APM32F103ZE Devices */
    /* #define APM32F105xB */   /*!< APM32F105RB and APM32F105VB Devices */
    /* #define APM32F105xC */   /*!< APM32F105RC and APM32F105VC Devices */
    /* #define APM32F107xB */   /*!< APM32F107RB and APM32F107VB Devices */
    /* #define APM32F107xC */   /*!< APM32F107RC and APM32F107VC Devices */
#endif

/**
 * @brief APM32F10x Standard Peripheral Library version number
 */
#define __APM32F10X_STDPERIPH_VERSION_MAIN   (0x02) /*!< [31:24] main version */
#define __APM32F10X_STDPERIPH_VERSION_SUB1   (0x00) /*!< [23:16] sub1 version */
#define __APM32F10X_STDPERIPH_VERSION_SUB2   (0x00) /*!< [15:8]  sub2 version */
#define __APM32F10X_STDPERIPH_VERSION_RC     (0x00) /*!< [7:0]  release candidate */
#define __APM32F10X_STDPERIPH_VERSION       ( (__APM32F10X_STDPERIPH_VERSION_MAIN << 24)\
                                             |(__APM32F10X_STDPERIPH_VERSION_SUB1 << 16)\
                                             |(__APM32F10X_STDPERIPH_VERSION_SUB2 << 8)\
                                             |(__APM32F10X_STDPERIPH_VERSION_RC))

/**@} end of group APM32F0xx_StdPeripheral_Library_Version */

/** @addtogroup Device_Included
  * @{
  */

#if defined(APM32F103x4)
  #include "apm32f103x4.h"
#elif defined(APM32F103x6)
  #include "apm32f103x6.h"
#elif defined(APM32F103x8)
  #include "apm32f103x8.h"
#elif defined(APM32F103xB)
  #include "apm32f103xb.h"
#elif defined(APM32F103xC)
  #include "apm32f103xc.h"
#elif defined(APM32F103xD)
  #include "apm32f103xd.h"
#elif defined(APM32F103xE)
  #include "apm32f103xe.h"
#elif defined(APM32F105xB)
  #include "apm32f105xb.h"
#elif defined(APM32F105xC)
  #include "apm32f105xc.h"
#elif defined(APM32F107xB)
  #include "apm32f107xb.h"
#elif defined(APM32F107xC)
  #include "apm32f107xc.h"
#else
 #error "Please select first the target APM32F10x device used in your application (in apm32f10x.h file)"
#endif

/**
  * @}
  */

#if defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050) /* ARM Compiler V6 */
  #ifndef __weak
    #define __weak  __attribute__((weak))
  #endif
  #ifndef __packed
    #define __packed  __attribute__((packed))
  #endif
#elif defined ( __GNUC__ ) && !defined (__CC_ARM) /* GNU Compiler */
  #ifndef __weak
    #define __weak   __attribute__((weak))
  #endif /* __weak */
  #ifndef __packed
    #define __packed __attribute__((__packed__))
  #endif /* __packed */
#endif /* __GNUC__ */

/** @defgroup Exported_Types
  @{
*/

typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef const int32_t sc32;
typedef const int16_t sc16;
typedef const int8_t sc8;

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef __I int32_t vsc32;
typedef __I int16_t vsc16;
typedef __I int8_t vsc8;

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef const uint32_t uc32;
typedef const uint16_t uc16;
typedef const uint8_t uc8;

typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

typedef __I uint32_t vuc32;
typedef __I uint16_t vuc16;
typedef __I uint8_t vuc8;

#ifndef __IM
#define __IM   __I
#endif
#ifndef __OM
#define __OM   __O
#endif
#ifndef __IOM
#define __IOM  __IO
#endif

enum {BIT_RESET, BIT_SET};
enum {RESET, SET};
enum {DISABLE, ENABLE};
enum {ERROR, SUCCESS};

#ifndef NULL
#define NULL   ((void *)0)
#endif

#if defined (__CC_ARM )
#pragma anon_unions
#endif

/**@} end of group Exported_types */

/** @defgroup Exported_Macros
  @{
*/

/* Define one bit mask */
#define BIT0                    ((uint32_t)0x00000001)
#define BIT1                    ((uint32_t)0x00000002)
#define BIT2                    ((uint32_t)0x00000004)
#define BIT3                    ((uint32_t)0x00000008)
#define BIT4                    ((uint32_t)0x00000010)
#define BIT5                    ((uint32_t)0x00000020)
#define BIT6                    ((uint32_t)0x00000040)
#define BIT7                    ((uint32_t)0x00000080)
#define BIT8                    ((uint32_t)0x00000100)
#define BIT9                    ((uint32_t)0x00000200)
#define BIT10                   ((uint32_t)0x00000400)
#define BIT11                   ((uint32_t)0x00000800)
#define BIT12                   ((uint32_t)0x00001000)
#define BIT13                   ((uint32_t)0x00002000)
#define BIT14                   ((uint32_t)0x00004000)
#define BIT15                   ((uint32_t)0x00008000)
#define BIT16                   ((uint32_t)0x00010000)
#define BIT17                   ((uint32_t)0x00020000)
#define BIT18                   ((uint32_t)0x00040000)
#define BIT19                   ((uint32_t)0x00080000)
#define BIT20                   ((uint32_t)0x00100000)
#define BIT21                   ((uint32_t)0x00200000)
#define BIT22                   ((uint32_t)0x00400000)
#define BIT23                   ((uint32_t)0x00800000)
#define BIT24                   ((uint32_t)0x01000000)
#define BIT25                   ((uint32_t)0x02000000)
#define BIT26                   ((uint32_t)0x04000000)
#define BIT27                   ((uint32_t)0x08000000)
#define BIT28                   ((uint32_t)0x10000000)
#define BIT29                   ((uint32_t)0x20000000)
#define BIT30                   ((uint32_t)0x40000000)
#define BIT31                   ((uint32_t)0x80000000)

#define SET_BIT(REG, BIT)       ((REG) |= (BIT))

#define CLEAR_BIT(REG, BIT)     ((REG) &= ~(BIT))

#define READ_BIT(REG, BIT)      ((REG) & (BIT))

#define CLEAR_REG(REG)          ((REG) = (0x0))

#define WRITE_REG(REG, VAL)     ((REG) = (VAL))

#define READ_REG(REG)           ((REG))

#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

#define UNUSED(X)               (void)X

/**@} end of group Exported_Macros*/
/**@} end of group APM32F10x */
/**@} end of group CMSIS */

#ifdef __cplusplus
}
#endif

#endif /* __APM32F10X_H */
