#ifndef __FLW_DMA_H
#define __FLW_DMA_H
#include "stm32f10x.h"

typedef enum{
    DMA_DIRECTION_TX = 0,
    DMA_DIRECTION_RX = 1,
}DMA_Direction_E;


typedef struct
{
   uint32_t              PeriphBaseAddr;        //外设数据地址
   DMA_Channel_TypeDef  *DMA_RX_Channel;        //DMA接收通道
   DMA_Channel_TypeDef  *DMA_TX_Channel;        //
   uint32_t              DMA_RX_TCFLAG;         //
   uint32_t              DMA_TX_TCFLAG;         //
   uint8_t              *rx_buffer;             //接收内存
   uint16_t              rx_size;               //接收数据字节数
   uint8_t              *tx_buffer;             //发送内存
   uint16_t              tx_size;               //发送数据字节数
}DMA_STR;



//#define ET6037_DMA_TX_CHANNEL       (DMA1_Channel6)
//#define ET6037_DMA_RX_CHANNEL       (DMA1_Channel7)

//#define ET6037_DMA_TX_TCFLAG        (DMA1_FLAG_TC6)
//#define ET6037_DMA_RX_TCFLAG        (DMA1_FLAG_TC7)




void DMA_Config(DMA_STR *pdma_str, DMA_Direction_E dir);

#endif // for #ifndef __FLW_DMA_H
