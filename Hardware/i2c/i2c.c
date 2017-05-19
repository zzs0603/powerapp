#include "i2c.h"

#include "stdint.h"
#include "dma.h"
#include "gpio.h"
#include "stdio.h"

#define USE_TIMEOUT_USER_CALLBACK

#ifdef USE_TIMEOUT_USER_CALLBACK 
    uint8_t I2C_TIMEOUT_UserCallback(void);
#else
    #define I2C_TIMEOUT_UserCallback()  (ERROR)
#endif      /* USE_TIMEOUT_USER_CALLBACK */

#define I2C_TIMEOUT         ((uint32_t)0x1000)
#define I2C_LONG_TIMEOUT    ((uint32_t)(10 * I2C_TIMEOUT))    
#define TIMED(state, max)   {timeout = max; while(state){if(timeout-- == 0) return I2C_TIMEOUT_UserCallback();}}


uint8_t I2Cx_ReadBlock(I2C_INFO *i2c_info)
{
    uint32_t timeout;

    if(!i2c_info->dma_str.rx_buffer)
        return ERROR;
    
    /* Test on BUSY Flag */
    TIMED(I2C_GetFlagStatus(i2c_info->i2c_str.I2Cx, I2C_FLAG_BUSY), I2C_LONG_TIMEOUT); 

   /* Configure DMA Peripheral */
    DMA_Config(&i2c_info->dma_str, DMA_DIRECTION_RX); 
    
    /* Enable DMA NACK automatic generation */
    I2C_DMALastTransferCmd(i2c_info->i2c_str.I2Cx, ENABLE);

    /* Enable the I2C peripheral */
    I2C_GenerateSTART(i2c_info->i2c_str.I2Cx, ENABLE);

    /* Test on SB Flag (EV5)*/
    TIMED((I2C_GetFlagStatus(i2c_info->i2c_str.I2Cx, I2C_FLAG_SB) == RESET), I2C_TIMEOUT);

    /* Transmit the slave address and enable writing operation */
    I2C_Send7bitAddress(i2c_info->i2c_str.I2Cx, i2c_info->i2c_str.slaveAddr, I2C_Direction_Transmitter);

    /* Test on ADDR Flag (EV6)*/
    TIMED((!I2C_CheckEvent(i2c_info->i2c_str.I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)), I2C_TIMEOUT);

    I2C_SendData(i2c_info->i2c_str.I2Cx, i2c_info->i2c_str.regBegin);

    /* Test on TXE FLag (data sent)--EV8，EV8_2 */
   TIMED(((!I2C_GetFlagStatus(i2c_info->i2c_str.I2Cx, I2C_FLAG_TXE)) && (!I2C_GetFlagStatus(i2c_info->i2c_str.I2Cx, I2C_FLAG_BTF))), I2C_TIMEOUT);

    /* Send START condition a second time */  
    I2C_GenerateSTART(i2c_info->i2c_str.I2Cx, ENABLE);

    /* Test on SB Flag (EV5)*/
    TIMED((I2C_GetFlagStatus(i2c_info->i2c_str.I2Cx, I2C_FLAG_SB) == RESET), I2C_TIMEOUT);

    /* Transmit the slave address and enable writing operation */
    I2C_Send7bitAddress(i2c_info->i2c_str.I2Cx, i2c_info->i2c_str.slaveAddr, I2C_Direction_Receiver);

    /* Test on ADDR Flag (EV6)*/
    TIMED((!I2C_CheckEvent(i2c_info->i2c_str.I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)), I2C_TIMEOUT);

    /* Enable I2C DMA request */
    I2C_DMACmd(i2c_info->i2c_str.I2Cx, ENABLE);
    
    /* Enable DMA RX Channel */
    DMA_Cmd(i2c_info->dma_str.DMA_RX_Channel, ENABLE);

    /* Wait until DMA Transfer Complete */
    TIMED((!DMA_GetFlagStatus(i2c_info->dma_str.DMA_RX_TCFLAG)), I2C_LONG_TIMEOUT);

    /* Send STOP Condition */
    I2C_GenerateSTOP(i2c_info->i2c_str.I2Cx, ENABLE);

    /* Disable DMA RX Channel */
    DMA_Cmd(i2c_info->dma_str.DMA_RX_Channel, DISABLE);
    /* Disable I2C DMA request */  
    I2C_DMACmd(i2c_info->i2c_str.I2Cx, DISABLE);
    /* Clear DMA RX Transfer Complete Flag */
    DMA_ClearFlag(i2c_info->dma_str.DMA_RX_TCFLAG);
    
    
    return 0;
}


uint8_t I2Cx_WriteBlock(I2C_INFO  *i2c_info)
{
     uint32_t timeout;

    if(!i2c_info->dma_str.tx_buffer || (i2c_info->dma_str.tx_size == 0))
        return ERROR;

    /* Test on BUSY Flag */
    TIMED(I2C_GetFlagStatus(i2c_info->i2c_str.I2Cx, I2C_FLAG_BUSY), I2C_LONG_TIMEOUT); 

    /* Configure DMA Peripheral */
    DMA_Config(&i2c_info->dma_str, DMA_DIRECTION_TX);

    /* Enable the I2C peripheral */
    I2C_GenerateSTART(i2c_info->i2c_str.I2Cx, ENABLE);
    /* Test on SB Flag (EV5)*/
    TIMED((I2C_GetFlagStatus(i2c_info->i2c_str.I2Cx, I2C_FLAG_SB) == RESET), I2C_TIMEOUT);

    /* Transmit the slave address and enable writing operation */
    I2C_Send7bitAddress(i2c_info->i2c_str.I2Cx, i2c_info->i2c_str.slaveAddr, I2C_Direction_Transmitter);
    /* Test on ADDR Flag (EV6)*/
    TIMED((!I2C_CheckEvent(i2c_info->i2c_str.I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)), I2C_TIMEOUT);

    I2C_SendData(i2c_info->i2c_str.I2Cx, i2c_info->i2c_str.regBegin);
    /* Test on TXE FLag (data sent)--EV8，EV8_2 */
    TIMED(((!I2C_GetFlagStatus(i2c_info->i2c_str.I2Cx, I2C_FLAG_TXE)) && (!I2C_GetFlagStatus(i2c_info->i2c_str.I2Cx, I2C_FLAG_BTF))), I2C_TIMEOUT);

    /* Enable I2C DMA request */
    I2C_DMACmd(i2c_info->i2c_str.I2Cx, ENABLE);
    /* Enable DMA TX Channel */
    DMA_Cmd(i2c_info->dma_str.DMA_TX_Channel, ENABLE);

    /* Wait until DMA Transfer Complete */
    TIMED((!DMA_GetFlagStatus(i2c_info->dma_str.DMA_TX_TCFLAG)), I2C_LONG_TIMEOUT);
    TIMED(!(I2C_GetFlagStatus(i2c_info->i2c_str.I2Cx, I2C_FLAG_BTF)), I2C_LONG_TIMEOUT);

     /* Send STOP Condition */
    I2C_GenerateSTOP(i2c_info->i2c_str.I2Cx, ENABLE);
     /* Disable DMA TX Channel */
    DMA_Cmd(i2c_info->dma_str.DMA_TX_Channel, DISABLE);
    /* Disable I2C DMA request */  
    I2C_DMACmd(i2c_info->i2c_str.I2Cx, DISABLE);
    /* Clear DMA RX Transfer Complete Flag */
    DMA_ClearFlag(i2c_info->dma_str.DMA_TX_TCFLAG);
    
    
    return SUCCESS;
}

uint8_t I2C_TIMEOUT_UserCallback()
{
    //I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_GenerateSTOP(I2C2, ENABLE);
    return ERROR;
}






