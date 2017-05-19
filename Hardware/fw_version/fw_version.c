#include "fw_version.h"
#include "bsp_eeprom.h"



uint8_t version_view(uint8_t *version_table, uint8_t len)
{
    int32_t temp;
    if(len < 8)
       return 0;
    
    for(uint8_t i = 0; i < 3; i++) //读取硬件版本
    {
        temp = EEPROM_Read(EEPROM_ID_HW_VER_FIRST + i); 
        if(temp < 0) //如果读出的硬件版本号不对 则自己写入正确版本号
        {
            EEPROM_Write(EEPROM_ID_HW_VER_FIRST, VM);
            EEPROM_Write(EEPROM_ID_HW_VER_FIRST+1, HW);
            EEPROM_Write(EEPROM_ID_HW_VER_FIRST+2, SW);
            version_table[0] = VM;
            version_table[1] = HW;
            version_table[2] = SW;
            break;
        }
        version_table[i] = temp & 0xff;
    }
    
    version_table[3] = VM;
    version_table[4] = HW;
    version_table[5] = SW;
    version_table[6] = REPE & 0xff;
    version_table[7] = (REPE >> 8) & 0xff;
    
    return 1;
}



