/*
 * bsp_flash.c
 *
 *  Created on: 2016年3月4日
 *      Author: shench
 */

#include "bsp_flash.h"
#include "string.h"

/**
 * check special address is valid or not
 * @param address[IN]           flash address
 * @param sector_number[OUT]    sector serial number[0,127]
 * @param sector_start[OUT]     sector starting address
 * @return valid address return 1, else return 0
 */
static uint8_t flash_get_sector_info(uint32_t address, uint8_t *sector_number,
        uint32_t *sector_start /*, uint32_t *sector_size */)
{
    uint16_t sector = STM32_SECTOR_SERIAL(address);

    if (sector <= STM32_SECTOR_MAX)
    {
        /* address lies within this sector */
        *sector_number = sector;
        *sector_start  = sector * PAGE_SIZE + STM32_FLASH_BASE;
//        *sector_size   = PAGE_SIZE;
        return 1;
    }

    return 0;
}

/**
 * write data to special address
 * @param addr[IN]      flash address
 * @param data[IN]      data body
 * @param len[IN]       data length
 * @return -1   invalid flash address
 *         -2   Write crosses the flash capability
 *         -3   flash program error
 *         0    success
 */
int32_t flash_write_data(uint32_t addr, uint8_t *data, int32_t len) 
{
    uint8_t sector_num;
    uint32_t sector_start;

    if(!data || (len == 0) \
            || !flash_get_sector_info(addr, &sector_num, &sector_start))
    {
        return -1;  /* we're asking for an invalid flash address */
    }

    addr = (addr & 0x01) ? (addr - 1) : addr; //偶地址起始
    if ((uintptr_t)addr + len > STM32_FLASH_TAIL)
        return -2;/* Write crosses the flash capability */


    uint32_t temp_addr = addr, offset;
    uint16_t x = 0, hword_data;
    uint16_t numberOfhWords = len >> 1;
    FLASH_Status status;

    FLASH_Unlock();
    for(x = 0; x < numberOfhWords; x++)
    {
        offset = 2 * x;
        hword_data = (data[offset + 1] << 8) | data[offset];
        if (hword_data != *(uint16_t *)(temp_addr + offset))
        {
            status = FLASH_ProgramHalfWord(temp_addr + offset, hword_data);
            if(status != FLASH_COMPLETE)
            {
                FLASH_Lock();
                return -3;
            }
        }
    }

    if(len & 0x01) // mod
    {
        offset = 2 * x;
        hword_data = 0xFF00 | data[offset];
        if (hword_data != *(uint16_t *)(temp_addr + offset))
        {
            status = FLASH_ProgramHalfWord(temp_addr + offset, hword_data);
            if(status != FLASH_COMPLETE)
            {
                FLASH_Lock();
                return -3;
            }
        }
    }
    FLASH_Lock();
    return 0;
}

/**
 * write data to special address
 * @param addr[IN]      flash address
 * @param data[OUT]     read from flash
 * @param len[IN]       data length
 * @return -1   invalid flash address
 *         -2   read crosses the flash capability
 *         0    success
 */
int32_t flash_read_data(uint32_t addr, uint8_t *data, int32_t len) 
{
    uint8_t sector_num;
    uint32_t sector_start;

    if(!data || (len == 0) \
            || !flash_get_sector_info(addr, &sector_num, &sector_start))
    {
        return -1;  /* we're asking for an invalid flash address */
    }

    addr = (addr & 0x01) ? (addr - 1) : addr; //偶地址起始
    if ((uintptr_t)addr + len > STM32_FLASH_TAIL)
        return -2;  /* read crosses the flash capability */

    memcpy(data, (void *)addr, len);

    return 0;
}


//========================================================================
u16 STMFLASH_ReadHalfWord(u32 faddr)
{
    return *(vu16*)faddr; 
}

uint8_t STMFLASH_Write_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)    
{
    uint8_t state, re = 1;
    u16 i;
    for(i=0;i<NumToWrite;i++)
    {
        state = FLASH_ProgramHalfWord(WriteAddr,pBuffer[i]);
        if(state != FLASH_COMPLETE)
        {
            re = 0;
            break;
        }
        WriteAddr+=2;
    }
    
    return re;
}

void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead)     
{
    u16 i;
    for(i=0;i<NumToRead;i++)
    {
        pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr);
        ReadAddr+=2;
    }
}

