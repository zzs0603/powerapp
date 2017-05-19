#include "iic.h"
#include "delay.h"

#define TM1  12
#define TM2  6

//��ʼ��IIC
void IIC_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE );//ʹ��GPIOBʱ��
       
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD ;   //�������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB,GPIO_Pin_10|GPIO_Pin_11); //PB10,PB11 �����
}
//����IIC��ʼ�ź�
void IIC_Start(void)
{
    SDA_OUT();     //sda�����
    IIC_SDA=1;      
    delay_us(TM1);
    IIC_SCL=1;
    delay_us(TM1);
    IIC_SDA=0;    //START:when CLK is high,DATA change form high to low 
    delay_us(TM1);
    IIC_SCL=0;    //ǯסI2C���ߣ�׼�����ͻ�������� 
    delay_us(TM1);
}
//����IICֹͣ�ź�
void IIC_Stop(void)
{
    SDA_OUT();    //sda�����
    IIC_SCL=0;
    delay_us(TM1);
    IIC_SDA=0;    //STOP:when CLK is high DATA change form low to high
    delay_us(TM1);
    IIC_SCL=1;
    delay_us(TM1);
    IIC_SDA=1;    //����I2C���߽����ź�
    delay_us(TM1);
}
//�ȴ�Ӧ���źŵ���
//����ֵ��1������Ӧ��ʧ��
//        0������Ӧ��ɹ�
u8 IIC_Wait_Ack(void)
{
    u8 ucErrTime=0;
    SDA_IN();      //SDA����Ϊ����  
    IIC_SDA=1; //delay_us(1);   
    IIC_SCL=1; //delay_us(1); 
    while(READ_SDA)
    {
        delay_us(10);
        ucErrTime++;
        if(ucErrTime>250)
        {
            IIC_Stop();
            return 1;
        }
    }
    delay_us(TM1);
    IIC_SCL=0;//ʱ�����0    
    return 0;  
}
//����ACKӦ��
void IIC_Ack(void)
{
    IIC_SCL=0;
    SDA_OUT();
    IIC_SDA=0;
    delay_us(TM2);
    IIC_SCL=1;
    delay_us(TM2);
    IIC_SCL=0;
}
//������ACKӦ��
void IIC_NAck(void)
{
    IIC_SCL=0;
    SDA_OUT();
    IIC_SDA=1;
    delay_us(TM2);
    IIC_SCL=1;
    delay_us(TM2);
    IIC_SCL=0;
}  

//���شӻ�����Ӧ��
//1����Ӧ��
//0����Ӧ��
u8 IIC_Send_Byte(u8 txd)
{                        
    u8 t;   
    SDA_OUT(); //��Ϊ���
    
    IIC_SCL=0;//����ʱ�ӿ�ʼ���ݴ���
    
    for(t=0;t<8;t++)
    {
        if((txd&0x80)>>7)
            IIC_SDA=1;
        else
            IIC_SDA=0;
        delay_us(TM2);
        IIC_SCL=1;
        delay_us(TM2);
        txd<<=1;
        IIC_SCL=0;
        delay_us(TM2);
    }
    
    return IIC_Wait_Ack();
}
//��1���ֽڣ�ack=1ʱ������ACK��ack=0������nACK   
u8 IIC_Read_Byte(unsigned char ack)
{
    unsigned char i,receive=0;
    SDA_IN();    //SDA����Ϊ����
    
    for(i=0;i<8;i++)
    {
        IIC_SCL=0;
        delay_us(TM2);
        IIC_SCL=1;
        delay_us(TM2);
        receive<<=1;
        if(READ_SDA) receive++;
    }
    if (ack == 0)
        IIC_NAck();//����nACK
    else
        IIC_Ack(); //����ACK
    delay_us(250);
    return receive;
}

//-----------------------------------------
void iic_writeBlock(unsigned char SlaveAddress, unsigned int numBytes, unsigned char multi, void* TxData)
{
  unsigned int  i;
  unsigned char *temp;

  temp = (unsigned char *)TxData;           // Initialize array pointer
  IIC_Start();                              // Send Start condition
  IIC_Send_Byte(0xAA);    // [ADDR] + R/W bit = 0
  
  for (i = 0; i < numBytes; i++)
  {
    IIC_Send_Byte(*(temp));                 // Send data and ack
    temp++;                                 // Increment pointer to next element
  }
  
  if(multi == 0)                           // Need STOP condition?
  {
    IIC_Stop();                 // Yes, send STOP condition
  }
  
  //delay_us(3);                                 // Quick delay
}


void iic_readBlock(unsigned char SlaveAddress, unsigned int numBytes, void* RxData)
{
  unsigned int  i;
  unsigned char* temp;

  temp = (unsigned char *)RxData;           // Initialize array pointer
 
  IIC_Start();                  // Send Start condition
  IIC_Send_Byte(0xAB);          //��
    
  delay_us(250);
    
  for (i = 0; i < numBytes; i++)
  {
    if (i == (numBytes - 1))
      *(temp) = IIC_Read_Byte(NACK);// Read last 8-bit data with no ACK
    else
      *(temp) = IIC_Read_Byte(ACK);// Read 8-bit data & then send ACK
    temp++;                                 // Increment pointer to next element
  }
  
  IIC_Stop();                   // Send Stop condition
}






















