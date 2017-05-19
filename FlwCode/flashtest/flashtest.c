#include "flashtest.h"


FLASH_RECODE f_data;


uint8_t flash_write_test(void)
{
     uint8_t re = 0;
     
     f_data.data.val1 = 4;
     f_data.data.val2 = 3;
     f_data.data.val3 = 2;
     f_data.data.val4 = 1;
     
    //CRC计算
    {
        f_data.crc16.Crch = 0xff;
        f_data.crc16.Crcl = 0xff;
        
        crc16block((char*)&f_data.data, sizeof(f_data.data), &f_data.crc16);
    }
    
    //写FLASH
    STMFLASH_Write(DATA_ADDR_STATR, (uint16_t *)&f_data, sizeof(f_data)/2);
    
    //校验
    {
        FLASH_RECODE check_temp;
        CRCCODE crctemp;
        
        STMFLASH_Read(DATA_ADDR_STATR,  (uint16_t *)&check_temp, sizeof(check_temp)/2); //从flash读数据
        
        crctemp.Crch = 0xff;
        crctemp.Crcl = 0xff;
        
        crc16block((char *)&check_temp.data, sizeof(check_temp.data), &crctemp); //计算CRC
        
        if(crctemp.Crch == check_temp.crc16.Crch && crctemp.Crcl == check_temp.crc16.Crcl) //与读出的CRC进行比较
        {
            re = 1;
        }
    }
    
    return re;
}


