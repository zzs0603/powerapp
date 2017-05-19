/*
 * bsp_eeprom.h
 *
 *  Created on: 2016年3月5日
 *      Author: Administrator
 */

#ifndef BSP_BSP_EEPROM_H_
#define BSP_BSP_EEPROM_H_

// $Id$
/*
 * Header file for EEPROM functions
 *
 * ==========================================================================
 */
#include "bsp_flash.h"
/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////


#if defined (STM32F10X_LD) || defined (STM32F10X_MD)
    #define EEPROM_START_ADDRESS    ((uint32_t)0x08002000)
#elif defined (STM32F10X_HD) || defined (STM32F10X_CL)
    #define EEPROM_START_ADDRESS    ((uint32_t)0x08002000)
#endif

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////
typedef enum
{
    EEPROM_ID_SN_FIRST = 0,
    EEPROM_ID_SN_TAIL = 35,
    EEPROM_ID_SN_LEN = 36,

    EEPRIM_ID_KEY_FIRST = 37,
    EEPRIM_ID_KEY_TAIL = 68,
    EEPRIM_ID_KEY_LEN  = 69,

    EEPROM_ID_HW_VER_FIRST = 128,
    EEPROM_ID_HW_VER_TAIL = 143,
    EEPROM_ID_HW_VER_LEN = 144,

    EEPROM_ID_SW_UPG_FLAG = 145,
    EEPROM_ID_SW_LEN_LOW = 146,
    EEPROM_ID_SW_LEN_HIGH = 147,
    EEPROM_ID_SW_CRC = 148,

    EEPROM_ID_STEER_LEFT = 160,
    EEPROM_ID_STEER_RIGHT = 161,

    EEPROM_ID_PITCH_UP = 162,
    EEPROM_ID_PITCH_DOWN = 163,
    EEPROM_ID_PITCH_CENTER = 164,
    EEPROM_ID_STEER_CENTER = 165,
    EEPROM_ID_UPG_COUNT = 166,
    EEPROM_ID_UPDATE_TEST = 167,
    EEPROM_EMULATED_SIZE,       /* number of half words */
} EEPROM_ID_E;

typedef struct {
    const __IO uint16_t Id;    /* 唯一虚拟ID */
    uint16_t data;              /* 保存的数据内容 */
}eeprom_t;

extern s32 EEPROM_Init(u32 mode);
extern s32 EEPROM_Read(u16 VirtAddress);
extern s32 EEPROM_Write(u16 VirtAddress, u16 Data);

extern s32 EEPROM_SendDebugMessage(u32 mode);

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////

#endif /* BSP_BSP_EEPROM_H_ */

