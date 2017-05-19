#include "stdint.h"

#include "dma.h"

#define DMA_CLK     (RCC_AHBPeriph_DMA1)




void DMA_Config( DMA_STR *pdma_str,  DMA_Direction_E dir)
{
    DMA_InitTypeDef DMA_InitStructure;

    RCC_AHBPeriphClockCmd(DMA_CLK, ENABLE);
   
    /* Initialize the DMA_PeripheralInc member */
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    /* Initialize the DMA_MemoryInc member */
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    /* Initialize the DMA_PeripheralDataSize member */
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    /* Initialize the DMA_MemoryDataSize member */
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    /* Initialize the DMA_Mode member */
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    /* Initialize the DMA_Priority member */
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    /* Initialize the DMA_M2M member */
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    
    /* If using DMA for Reception */
    if (dir == DMA_DIRECTION_RX)
    {
        /* Initialize the DMA_DIR member */
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
        
          /* Initialize the DMA_MemoryBaseAddr member */
        DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)pdma_str->rx_buffer;
        DMA_InitStructure.DMA_BufferSize = pdma_str->rx_size;        
        
        /* Initialize the DMA_PeripheralBaseAddr member */
        DMA_InitStructure.DMA_PeripheralBaseAddr = pdma_str->PeriphBaseAddr;
        DMA_DeInit(pdma_str->DMA_RX_Channel);
        DMA_Init(pdma_str->DMA_RX_Channel, &DMA_InitStructure);
        DMA_ClearFlag(pdma_str->DMA_RX_TCFLAG);
    }
    /* If using DMA for Transmission */
    else if(dir == DMA_DIRECTION_TX)
    { 
        /* Initialize the DMA_DIR member */
        DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;

        /* Initialize the DMA_MemoryBaseAddr member */
        DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)pdma_str->tx_buffer;
        DMA_InitStructure.DMA_BufferSize = pdma_str->tx_size;        
        
        /* Initialize the DMA_PeripheralBaseAddr member */
        DMA_InitStructure.DMA_PeripheralBaseAddr = pdma_str->PeriphBaseAddr;
        DMA_DeInit(pdma_str->DMA_TX_Channel);
        DMA_Init(pdma_str->DMA_TX_Channel, &DMA_InitStructure);
        DMA_ClearFlag(pdma_str->DMA_TX_TCFLAG);
    }
    
}

