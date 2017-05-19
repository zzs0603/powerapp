#ifndef __FLW_I2C_H
#define __FLW_I2C_H

#include "stm32f10x.h"
#include "dma.h"



typedef struct
{
    I2C_TypeDef *I2Cx;  //I2Cx
    uint8_t slaveAddr;  //�豸��ַ
    uint8_t regBegin;   //�Ĵ�����ַ
}I2C_STR;

typedef struct
{
   I2C_STR  i2c_str;
   DMA_STR  dma_str;
}I2C_INFO;



uint8_t I2Cx_ReadBlock(I2C_INFO   *i2c_info);
uint8_t I2Cx_WriteBlock(I2C_INFO  *i2c_info);

#endif // for #ifndef _FLW_I2C_H
