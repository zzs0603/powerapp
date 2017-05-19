#ifndef __FLW_DMA_H
#define __FLW_DMA_H
#include "stm32f10x.h"

typedef enum{
    DMA_DIRECTION_TX = 0,
    DMA_DIRECTION_RX = 1,
}DMA_Direction_E;


typedef struct
{
   uint32_t              PeriphBaseAddr;        //�������ݵ�ַ
   DMA_Channel_TypeDef  *DMA_RX_Channel;        //DMA����ͨ��
   DMA_Channel_TypeDef  *DMA_TX_Channel;        //
   uint32_t              DMA_RX_TCFLAG;         //
   uint32_t              DMA_TX_TCFLAG;         //
   uint8_t              *rx_buffer;             //�����ڴ�
   uint16_t              rx_size;               //���������ֽ���
   uint8_t              *tx_buffer;             //�����ڴ�
   uint16_t              tx_size;               //���������ֽ���
}DMA_STR;



//#define ET6037_DMA_TX_CHANNEL       (DMA1_Channel6)
//#define ET6037_DMA_RX_CHANNEL       (DMA1_Channel7)

//#define ET6037_DMA_TX_TCFLAG        (DMA1_FLAG_TC6)
//#define ET6037_DMA_RX_TCFLAG        (DMA1_FLAG_TC7)




void DMA_Config(DMA_STR *pdma_str, DMA_Direction_E dir);

#endif // for #ifndef __FLW_DMA_H
