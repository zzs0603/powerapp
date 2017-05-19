#ifndef _FLASHTEST_H
#define _FLASHTEST_H

#include "stmflash.h"
#include "CRC16.h"


#define DATA_ADDR_STATR  0X08010000 //64k  

typedef struct
{
    uint16_t val1;
    uint16_t val2;
    uint16_t val3;
    uint16_t val4;
}FLASH_DATA;

typedef struct
{
    FLASH_DATA data; //数据域
    CRCCODE crc16;   //校验位
}FLASH_RECODE;





uint8_t flash_write_test(void);


#endif

