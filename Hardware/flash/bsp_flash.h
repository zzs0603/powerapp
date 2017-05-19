/*
 * bsp_flash.h
 *
 *  Created on: 2016?3?4?
 *      Author: shench
 */

#ifndef BSP_BSP_FLASH_H_
#define BSP_BSP_FLASH_H_

#include "stm32f10x_flash.h"

#define STM32_FLASH_BASE    0x08000000      //STM32 flash 


#if defined (STM32F10X_LD) || defined (STM32F10X_MD)
    #define STM32_FLASH_SIZE    ((uint32_t)0x10000)    //64 kilobytes
    #define PAGE_SIZE           (uint16_t)0x400  /* Page size = 1KByte */
    #define SHIT_WIDTH          10
#elif defined (STM32F10X_HD) || defined (STM32F10X_CL)
    #define STM32_FLASH_SIZE    ((uint32_t)0x80000)    //512 kilobytes
    #define PAGE_SIZE           (uint16_t)0x800  /* Page size = 2KByte */
    #define SHIT_WIDTH          11
#endif

// sector_number = offset / 2k, range is [0,127]
#define STM32_SECTOR_MAX            (STM32_FLASH_SIZE >> SHIT_WIDTH) //???
#define STM32_SECTOR_SERIAL(addr)   (((addr) - STM32_FLASH_BASE) >> SHIT_WIDTH)
#define STM32_FLASH_TAIL            (STM32_FLASH_BASE + STM32_FLASH_SIZE)
#define HALF_WORD_SIZE              (PAGE_SIZE >> 1)

int32_t flash_read_data(uint32_t addr, uint8_t *data, int32_t len);
int32_t flash_write_data(uint32_t addr, uint8_t *data, int32_t len);

uint8_t STMFLASH_Write_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);

#endif /* BSP_BSP_FLASH_H_ */



















