#include "bq34z100.h"
#include "delay.h"
#define I2CSLAVEADDR 0xAA

unsigned char RxData[4];
signed short vol, cur, soc;

static u8 bq34z100_Read(unsigned char cmd, unsigned int bytes)
{
  unsigned char tx[1];

  tx[0] = cmd;

  iic_writeBlock(I2CSLAVEADDR, 1, 1, tx); //��дcmd��Ҫ��ȡ�ļĴ�����ַ
  delay_us(100);  
  iic_readBlock(I2CSLAVEADDR, bytes, RxData);
  delay_us(100);  
    
    return 0;
}

static void bq34z100_cmdWrite(unsigned char cmd, unsigned char data)
{
    unsigned char tx[2];

    tx[0] = cmd;
    tx[1] = data;
    
    iic_writeBlock(I2CSLAVEADDR, 2, 0, tx);
    delay_ms(1); 
}


void bq34z100_run(signed short* pvol, signed short* pcur, signed short* psoc) 
{
    
    bq34z100_Read(0x08, 2); //��ѹ
    *pvol = (signed short)(((RxData[1] << 8) & 0xff00) + (RxData[0] & 0x00ff));
    
    bq34z100_Read(0x10, 2); //����
    *pcur = (signed short)(((RxData[1] << 8) & 0xff00) + (RxData[0] & 0x00ff));
    
    bq34z100_Read(0x02, 1); //����
    *psoc = RxData[0];
}

signed short bq34z100_getvol(void)
{
    return vol;
}

signed short bq34z100_getcur(void)
{
    return cur;
}

signed short bq34z100_getsoc(void)
{
    return soc;
}


void bq34z100_Init(void)
{
    IIC_Init(); //iic��ʼ��
    bq34z100_cmdWrite(0x21, 0x00); //�����������㷨
    delay_ms(100); 
}
