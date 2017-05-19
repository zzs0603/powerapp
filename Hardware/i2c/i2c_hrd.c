#include "i2c_hrd.h"


#define I2C_REMAP
//#define SLAVE_10BIT_ADDRESS

#define Transmitter 0x00
#define Receiver    0x01

#define FALSE  0
#define TRUE   1

#define NODEVICEOFFSET  0xffffffff

static uint8_t Direction = Transmitter;
static uint16_t SlaveADDR;
static uint32_t TxLength,RxLength;

static uint32_t DeviceOffset = NODEVICEOFFSET;

static uint8_t* pTxBuffer1;
static uint8_t* pTxBuffer2;
static uint8_t* pRxBuffer1;
static uint8_t* pRxBuffer2;

static uint8_t  check_begin = FALSE;
uint8_t  MasterReceptionComplete = 0;
uint8_t  MasterTransitionComplete = 0;
uint8_t  SlaveRecepitionComplete = 0;
uint8_t  SlaveTransitionComplete = 0;

uint8_t  WriteComplete = 0; //to indicat target`s internal write process

//P-V operation on I2C1 or I2C2
uint8_t  PV_flag_1;
uint8_t  PV_flag_2;


I2C_STATE i2c_comm_state;

void I2C_Comm_Init(I2C_TypeDef* I2Cx, uint32_t I2C_Speed, uint16_t I2C_Addr)
{
    /*****GPIO configuration clock enable*****/
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef  I2C_InitStructure;
    NVIC_InitTypeDef NVIC_Initstructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    if(I2Cx == I2C1)
    {
#ifdef I2C_REMAP 
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
#else
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
#endif 
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    }
    else if(I2Cx == I2C2)
    {
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
    }
    
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    
    //I2C periphral configuration 
    I2C_DeInit(I2Cx);
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = I2C_Addr;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
#ifdef SLAVE_10BIT_ADDRESS
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_10bit;
#else
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
#endif 
    
    I2C_InitStructure.I2C_ClockSpeed = I2C_Speed;
    I2C_Cmd(I2Cx, ENABLE);
    I2C_Init(I2Cx, &I2C_InitStructure);
    
    //i2c NVIC config
    if(I2Cx == I2C1)
    {
        NVIC_Initstructure.NVIC_IRQChannel = I2C1_EV_IRQn;
    }
    else
    {
        NVIC_Initstructure.NVIC_IRQChannel = I2C2_EV_IRQn;
    }
    
    NVIC_Initstructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Initstructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_Initstructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_Initstructure);
    
    if(I2Cx == I2C1)
    {
        NVIC_Initstructure.NVIC_IRQChannel = I2C1_ER_IRQn;
    }
    else
    {
        NVIC_Initstructure.NVIC_IRQChannel = I2C2_ER_IRQn;
    }
    NVIC_Initstructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Initstructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_Initstructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_Initstructure);
    
}


static void Bus_Busy_Overflow_Check(I2C_TypeDef* I2Cx) //总线忙超时检测
{
      uint16_t overtime = 0;
      if(I2Cx->SR2 & 0x0002) //BUSY == 1
      {
          if(++overtime >= 0x400)
          {
              I2C_GenerateSTOP(I2Cx, ENABLE);
          }
      }
}



void I2C_Comm_MasterWrite(I2C_TypeDef* I2Cx, uint16_t slave_addr, uint32_t offset, uint8_t *pBuffer, uint32_t length)
{
    
    //总线忙检测
    //Bus_Busy_Overflow_Check(I2Cx);
    
    
    /*PV operation*/
    if(I2Cx == I2C1)
    {
        if(PV_flag_1 != 0)
           return;
        
        PV_flag_1 |= 1;
        
        if(PV_flag_1 != 1)
            return;
        
        PV_flag_1 = 3;        
        
    }
    else
    {
        if(PV_flag_2 != 0)
           return;
        
        PV_flag_2 |= 1;
        
        if(PV_flag_2 != 1)
            return;
        
        PV_flag_2 = 3;
    }
    
    /*initialize static parameter*/
    Direction = Transmitter;
    MasterTransitionComplete = 0;
    
    
    SlaveADDR = slave_addr;
    TxLength  = length;
    DeviceOffset = offset;
    if(I2Cx == I2C1)
        pTxBuffer1 = pBuffer;
    else
        pTxBuffer2 = pBuffer;
    
    i2c_comm_state = COMM_PRE;
    
    I2C_AcknowledgeConfig(I2Cx, ENABLE);
    I2C_ITConfig(I2Cx, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE);
    
    if(I2C1->CR1 & 0x200)
        I2C1->CR1 &= 0xFDFF;
    
    I2C_GenerateSTART(I2Cx, ENABLE);
}

void I2C_Comm_MasterRead(I2C_TypeDef* I2Cx, uint16_t slave_addr, uint32_t offset, uint8_t *pBuffer, uint32_t length)
{
    //总线忙检测
    //Bus_Busy_Overflow_Check(I2Cx);
      
     if(I2Cx == I2C1)
     {
         if(PV_flag_1 != 0)
             return;
         PV_flag_1 |= 1;
         if(PV_flag_1 != 1)
             return;
         PV_flag_1 = 3;
     }
     else
     {
         if(PV_flag_2 != 0)
             return;
         PV_flag_2 |= 1;
         if(PV_flag_2 != 1)
             return;
         PV_flag_2 = 3;
     }
     
     /*initialize static parameter*/
     Direction = Receiver;
     MasterReceptionComplete = 0;
     
     SlaveADDR = slave_addr;
     RxLength = length;
     DeviceOffset = offset;
     if(I2Cx == I2C1)
        pRxBuffer1 = pBuffer;
     else
        pRxBuffer2 = pBuffer;
     
     i2c_comm_state = COMM_PRE;
     
     I2C_AcknowledgeConfig(I2Cx, ENABLE);
     I2C_ITConfig(I2Cx, I2C_IT_EVT | I2C_IT_ERR, ENABLE);
     if(I2C1->CR1 & 0x200)
         I2C1->CR1 &= 0xFDFF;
     I2C_GenerateSTART(I2Cx, ENABLE);
}

void i2c1_evt_isr()
{
    switch( I2C_GetLastEvent(I2C1) )
    {
        case I2C_EVENT_MASTER_MODE_SELECT: //EV5 //已发送起始条件
            if(!check_begin)
                i2c_comm_state = COMM_IN_PROCESS;
            
            if(Direction == Receiver)
            {
                if(DeviceOffset != NODEVICEOFFSET)
                   I2C_Send7bitAddress(I2C1, SlaveADDR, I2C_Direction_Transmitter); //先写寄存器地址 
                else
                   I2C_Send7bitAddress(I2C1, SlaveADDR, I2C_Direction_Receiver); //再接收
            }
            else
            {
                //Send Slave Address for write
                I2C_Send7bitAddress(I2C1, SlaveADDR, I2C_Direction_Transmitter);
            }
            I2C_ITConfig(I2C1, I2C_IT_BUF, ENABLE);
            break;
            
             //主机接收事件
            case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED: //EV6 //从机地址已发送
                 if(RxLength == 1)
                 {
                     I2C_AcknowledgeConfig(I2C1, DISABLE);
                     I2C_GenerateSTOP(I2C1, ENABLE);
                 }
            break;
                 
                
            case I2C_EVENT_MASTER_BYTE_RECEIVED: //EV7 已接收到一个BYTE
                *pRxBuffer1++ = I2C_ReceiveData(I2C1);
                 RxLength--;
                 //Disable ack and send i2c1 stop condition before receiving the last data
                 
                  if(RxLength == 1)
                  {
                      I2C_AcknowledgeConfig(I2C1, DISABLE);
                      I2C_GenerateSTOP(I2C1, ENABLE);
                  }
                  
                  if(RxLength == 0)
                  {
                     MasterReceptionComplete = 1;
                     i2c_comm_state = COMM_DONE;
                      PV_flag_1 = 0;
                  }
            break;
                  
               //主机发送事件
            case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED://EV6 发送第一个字节
                 if(check_begin)
                 {
                     check_begin = FALSE;
                     WriteComplete = 1;
                     i2c_comm_state = COMM_DONE;
                     I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
                     I2C_GenerateSTOP(I2C1, ENABLE);
                     PV_flag_1 = 0;
                     break;
                 }
                 
                 if(DeviceOffset != NODEVICEOFFSET)
                     I2C_SendData(I2C1, DeviceOffset);
                 else
                 {
                     I2C_SendData(I2C1, *pTxBuffer1++);
                     TxLength--;
                 }
            break;
                 
            case I2C_EVENT_MASTER_BYTE_TRANSMITTING: //EV8
                if(Direction == Receiver)
                {
                    DeviceOffset = NODEVICEOFFSET;
                    I2C_ITConfig(I2C1, I2C_IT_BUF, DISABLE);
                    while((I2C1->CR1 & 0x200) == 0x200);
                    I2C_GenerateSTART(I2C1, ENABLE);
                    break;
                }
                
                if(TxLength > 0)
                {
                    I2C_SendData(I2C1, *pTxBuffer1++);
                    TxLength--;
                }
                else if(TxLength == 0)
                {
                    I2C_ITConfig(I2C1, I2C_IT_BUF, DISABLE);
                    I2C_GenerateSTOP(I2C1, ENABLE);
                }
            break;
                
            case I2C_EVENT_MASTER_BYTE_TRANSMITTED: //EV8-2
                if(Direction == Transmitter)
                {
                    MasterTransitionComplete = 1;
                    I2C_ITConfig(I2C1, I2C_IT_EVT | I2C_IT_BUF, DISABLE);
                    //ENABLE AF and SB and ADDR interrupt
                    I2C_ITConfig(I2C1, I2C_IT_EVT |I2C_IT_ERR, ENABLE);
                    
                    check_begin = TRUE;
                    i2c_comm_state = CHECK_IN_PROCESS;
                    if(I2C1->CR1 & 0x200)
                        I2C1->CR1 &= 0xFDFF;
                    I2C_GenerateSTART(I2C1, ENABLE);
                }
             break;
                
                
            default:
            break;
    }
}


void i2c1_err_isr()
{
    if(I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))
    {
        if(check_begin)
            I2C_GenerateSTART(I2C1, ENABLE);
        else if(I2C1->SR2 & 0x01)
        {
            I2C_GenerateSTOP(I2C1, ENABLE);
            i2c_comm_state = COMM_EXIT;
            PV_flag_1 = 0;
        }
        
        I2C_ClearFlag(I2C1, I2C_FLAG_AF);
    }
    
    if(I2C_GetFlagStatus(I2C1, I2C_FLAG_BERR))
    {
        if(I2C1->SR2 & 0x01)
        {
            I2C_GenerateSTOP(I2C1, ENABLE);
            i2c_comm_state = COMM_EXIT;
            PV_flag_1 = 0;
        }
        
        I2C_ClearFlag(I2C1, I2C_FLAG_BERR);
    }
}



void i2c2_evt_isr()
{
    switch( I2C_GetLastEvent(I2C2) )
    {
        case I2C_EVENT_MASTER_MODE_SELECT: //EV5
            if(!check_begin)
                i2c_comm_state = COMM_IN_PROCESS;
            
            if(Direction == Receiver)
            {
                if(DeviceOffset != NODEVICEOFFSET)
                   I2C_Send7bitAddress(I2C2, SlaveADDR, I2C_Direction_Transmitter);
                else
                   I2C_Send7bitAddress(I2C2, SlaveADDR, I2C_Direction_Receiver);
            }
            else
            {
                //Send Slave Address for write
                I2C_Send7bitAddress(I2C2, SlaveADDR, I2C_Direction_Transmitter);
            }
            I2C_ITConfig(I2C2, I2C_IT_BUF, ENABLE);
            break;
            
             //主机接收事件
            case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED: //EV6
                 if(RxLength == 1)
                 {
                     I2C_AcknowledgeConfig(I2C2, DISABLE);
                     I2C_GenerateSTOP(I2C2, ENABLE);
                 }
            break;
                 
                
            case I2C_EVENT_MASTER_BYTE_RECEIVED: //EV7
                *pRxBuffer2++ = I2C_ReceiveData(I2C2);
                 RxLength--;
                 //Disable ack and send i2c1 stop condition before receiving the last data
                 
                  if(RxLength == 1)
                  {
                      I2C_AcknowledgeConfig(I2C2, DISABLE);
                      I2C_GenerateSTOP(I2C2, ENABLE);          //发送STOP时还会产生一次EV7
                  }
                  
                  if(RxLength == 0)
                  {
                     MasterReceptionComplete = 1;
                     i2c_comm_state = COMM_DONE;
                      PV_flag_2 = 0;
                  }
            break;
                  
               //主机发送事件 发送的第一个BYTE事件
            case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED://EV8
                 if(check_begin)
                 {
                     check_begin = FALSE;
                     WriteComplete = 1;
                     i2c_comm_state = COMM_DONE;
                     I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
                     I2C_GenerateSTOP(I2C2, ENABLE);
                     PV_flag_2 = 0;
                     break;
                 }
                 
                 if(DeviceOffset != NODEVICEOFFSET)
                     I2C_SendData(I2C2, DeviceOffset); //寄存器地址
                 else
                 {
                     I2C_SendData(I2C2, *pTxBuffer2++);
                     TxLength--;
                 }
            break;
                 
                 //发送后续BYTE事件
            case I2C_EVENT_MASTER_BYTE_TRANSMITTING: //EV8
                if(Direction == Receiver) //如果是接收
                {
                    DeviceOffset = NODEVICEOFFSET;  
                    I2C_ITConfig(I2C2, I2C_IT_BUF, DISABLE);
                    while((I2C2->CR1 & 0x200) == 0x200);
                    I2C_GenerateSTART(I2C2, ENABLE);
                    break;
                }
                
                if(TxLength > 0)
                {
                    I2C_SendData(I2C2, *pTxBuffer2++);
                    TxLength--;
                }
                else if(TxLength == 0)
                {
                    I2C_ITConfig(I2C2, I2C_IT_BUF, DISABLE);
                    //I2C_GenerateSTOP(I2C2, ENABLE);
                }
            break;
                
            case I2C_EVENT_MASTER_BYTE_TRANSMITTED: //EV8-2
                if(Direction == Transmitter)
                {
                    MasterTransitionComplete = 1;
                    I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF, DISABLE);
                    //ENABLE AF and SB and ADDR interrupt
                    I2C_ITConfig(I2C2, I2C_IT_EVT |I2C_IT_ERR, ENABLE);
                    check_begin = TRUE;
                    i2c_comm_state = CHECK_IN_PROCESS;
                    if(I2C2->CR1 & 0x200)
                        I2C2->CR1 &= 0xFDFF;
                    I2C_GenerateSTART(I2C2, ENABLE);
                    
                }
             break;
                
                
            default:
                I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, DISABLE);
                I2C_GenerateSTOP(I2C2, ENABLE);
            break;
    }
}


void  i2c2_err_isr()
{
    if(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))
    {
        if(check_begin)
            I2C_GenerateSTART(I2C2, ENABLE);
        else if(I2C2->SR2 & 0x01)
        {
            I2C_GenerateSTOP(I2C2, ENABLE);
            i2c_comm_state = COMM_EXIT;
            PV_flag_2 = 0;
        }
        I2C_ClearFlag(I2C2, I2C_FLAG_AF);
    }
    
    if(I2C_GetFlagStatus(I2C2, I2C_FLAG_BERR))
    {
        if(I2C2->SR1 & 0x01)
        {
            I2C_GenerateSTOP(I2C2, ENABLE);
            i2c_comm_state = COMM_EXIT;
            PV_flag_2 = 0;
        }
        
        I2C_ClearFlag(I2C2, I2C_FLAG_BERR);
    }
}





