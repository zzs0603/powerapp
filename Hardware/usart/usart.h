#ifndef __USART_H
#define __USART_H


#include "stdio.h"
#include "stdint.h" 



/* debug usart port & pins defined */
#define DEBUG_USART_RCC_APBxPeriph      (RCC_APB2Periph_USART1)
#define DEBUG_USART_GIPO_RCC_APBxPeriph (RCC_APB2Periph_GPIOA)
#define DEBUG_USART_GPIO_PORT           (GPIOA)
#define DEBUG_USART_TX_PIN              (GPIO_Pin_9)
#define DEBUG_USART_RX_PIN              (GPIO_Pin_10)



#define PROTOCOL_DATA_MAX_LEN    (32)
#define PROTOCOL_CMD_MAX_LEN     (8)
#define PROTOCOL_LEN_MAX_LEN     (4)
#define PROTOCOL_CRC_MAX_LEN     (4)

typedef enum
{
    USART_RX_SYNC,
    USART_RX_CMD,
    USART_RX_LEN,
    USART_RX_DATA,
    USART_RX_CRC,
    USART_RX_TAIL,
} USART_RX_STATE_E;

typedef struct
{
    uint8_t cmd[PROTOCOL_CMD_MAX_LEN];
    uint8_t data[PROTOCOL_DATA_MAX_LEN];
    uint32_t len;
    uint8_t crc[PROTOCOL_CRC_MAX_LEN];
} usart_protocol_s;

typedef struct
{
    uint8_t state;                  //????
    uint8_t count;
    usart_protocol_s packet;
} usart_rx_s;



#define DEBUG_USART                 (USART1)
#define RCV_BUFF_NUM                64

void usart_init(uint32_t bound);
void usart_rcv_analysis(void);
#endif

