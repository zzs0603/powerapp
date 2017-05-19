#include "usart.h"
#include "gpio.h"
#include "flwFifo.h"
#include <string.h>
#include "bsp_eeprom.h"
#include "crc.h"
#include "stdio.h"

static flwfifo_s usart_rcv_fifo;
static uint8_t rcvBuf[RCV_BUFF_NUM] = {0};
//

static usart_rx_s usart_rx = { .state = USART_RX_SYNC, .count = 0,};

/* 加入以下代码,支持printf函数,而不需要选择use MicroLIB */
/************ BEGIN **************/
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
    int handle; 
}; 
FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
    x = x; 
} 

//重定义fputc函数 
int fputc(int ch, FILE *f)
{     
    while((DEBUG_USART->SR & 0X40) == 0);	//循环发送,直到发送完毕     
    DEBUG_USART->DR = (u8) ch;      
    return ch;
}
/************ END **************/

static void usart_device_init(uint32_t baudrate)
{
    //  GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    //时钟使能
    RCC_APB2PeriphClockCmd(DEBUG_USART_RCC_APBxPeriph, ENABLE);
    RCC_APB2PeriphClockCmd(DEBUG_USART_GIPO_RCC_APBxPeriph, ENABLE);

    //恢复串口默认设置
    USART_DeInit(DEBUG_USART);

    GPIO_Configuration(DEBUG_USART_GPIO_PORT, DEBUG_USART_TX_PIN, GPIO_Mode_AF_PP);
    GPIO_Configuration(DEBUG_USART_GPIO_PORT, DEBUG_USART_RX_PIN, GPIO_Mode_IN_FLOATING);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;       //抢占优先级3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;              //子优先级3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                 //IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);                                 //初始化NVIC寄存器

    //USART 初始化设置
    USART_InitStructure.USART_BaudRate = baudrate;                  //波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;     //8字节数据位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;          //一位停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;             //无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;     //无硬件数据流控制
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; //收发模式
    USART_Init(DEBUG_USART, &USART_InitStructure);

    USART_ITConfig(DEBUG_USART, USART_IT_RXNE, ENABLE);                 //使能串口接收中断
    USART_Cmd(DEBUG_USART, ENABLE);                                     //使能串口 
}


static void usart_variable_init(void)
{
    init_fifo(&usart_rcv_fifo, rcvBuf, RCV_BUFF_NUM);
}


void usart_init(uint32_t baudrate)
{
     usart_variable_init();
     usart_device_init(baudrate);
}





void USART1_IRQHandler(void)                
{
    int Res;

    if(USART_GetITStatus(DEBUG_USART, USART_IT_RXNE) != RESET)
    {
        Res = USART_ReceiveData(DEBUG_USART);
        //printf("%#x", Res);
        fifo_put_c(&usart_rcv_fifo, Res);
    } 
} 

static void hw_write(const char *data, uint16_t len, char *crc)
{
    uint8_t buf[5], i;

    sprintf((char*)buf, "%04X", crc16_updateCRC(0xFFFF, (uint8_t *)data, len));

    if(strncmp(crc, (char *)buf, 4) == 0)
    {
        for(i = 0; i < len; i++)
            EEPROM_Write(i + EEPROM_ID_HW_VER_FIRST, data[i] - '0');

        EEPROM_Write(EEPROM_ID_HW_VER_LEN, len);
    }
}

static void hw_read(void)
{
    uint8_t buf[32] = {'$', 'H', 'W', 'A', 'K', ','};
    int16_t len, i, index = 6;
    uint16_t crc = 0xffff;

    // length
    len = EEPROM_Read(EEPROM_ID_HW_VER_LEN); 
    if(len < 0 || len >= PROTOCOL_DATA_MAX_LEN)
        len = 0;

    i = sprintf((char *)buf + index, "%d", len);
    index += i;
    buf[index] = ',';
    index++;
    //data
    for(i = 0; i < len; i++)
    {
        buf[i + index] = EEPROM_Read(i + EEPROM_ID_HW_VER_FIRST) + '0';
        crc = crc16_updateByte(crc, buf[i + index]);
    }
    index += len;
    buf[index] = ',';
    index++;
    //crc
    i = sprintf((char *)buf + index, "%04X", crc);
    index += i;
    buf[index] = '@';
    index++;
    buf[index] = '\0';
    printf("%s\r\n", buf);
}


static void usart_protocol_parse(usart_rx_s *rx)
{
    uint8_t cmd_len = 0;

    cmd_len = strlen((char *)rx->packet.cmd);
    switch(cmd_len)
    {
    case 4:
        if(strcmp((char *)rx->packet.cmd, "HWWR") == 0)
        {
            hw_write((char *)rx->packet.data, rx->packet.len, (char *)rx->packet.crc);
        }
        else if(strcmp((char *)rx->packet.cmd, "HWRD") == 0)
        {
            hw_read();
        }
        break;
    default:
        break;
    }
}


static void usart_protocol_rcv(uint8_t ch)
{
    switch(ch)
    {
    case '$':
        usart_rx.state = USART_RX_CMD;
        usart_rx.count = 0;
        usart_rx.packet.data[0] = '\0';
        usart_rx.packet.len = 0;
        usart_rx.packet.crc[0] = '\0';
        usart_rx.packet.cmd[0] = '\0';
        break;
    case ',':
        if( (usart_rx.state > USART_RX_SYNC) \
                && (usart_rx.state != USART_RX_TAIL) )
        {
            usart_rx.state++;
            usart_rx.count = 0;
        }
        break;
    case '@':
        if(usart_rx.state > USART_RX_SYNC)
        {
            usart_rx.state = USART_RX_SYNC;  
            usart_protocol_parse(&usart_rx);
        }
        break;
    default:
        switch(usart_rx.state)
        {
        case USART_RX_CMD:
            if(usart_rx.count < PROTOCOL_CMD_MAX_LEN - 1)
            {
                usart_rx.packet.cmd[usart_rx.count++] = ch;
                usart_rx.packet.cmd[usart_rx.count] = '\0';
            }
            break;
        case USART_RX_LEN:
            if(usart_rx.count < PROTOCOL_LEN_MAX_LEN \
                    && (ch >= '0' && ch <= '9'))
            {
                usart_rx.packet.len = usart_rx.packet.len * 10 + ch - '0';
                usart_rx.count++;
            }
            break;
        case USART_RX_DATA:
            if(usart_rx.count < PROTOCOL_DATA_MAX_LEN - 1)
            {
                usart_rx.packet.data[usart_rx.count++] = ch;
                usart_rx.packet.data[usart_rx.count] = '\0';
            }
            break;
        case USART_RX_CRC:
            if(usart_rx.count < PROTOCOL_CRC_MAX_LEN)
            {
                usart_rx.packet.crc[usart_rx.count++] = ch;
            }
            break;
        case USART_RX_TAIL:     // wait for '@'
            break;
        default:
            break;
        }
        break;
    }
}

void usart_rcv_analysis(void)
{
    uint8_t ch;
    uint16_t len ;
    
    len = fifo_used_size(&usart_rcv_fifo);
    if(len == 0)
       return;
    
    while(len--)
    {
        ch = fifo_get_c(&usart_rcv_fifo);
        usart_protocol_rcv(ch);
    }
}


