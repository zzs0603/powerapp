#ifndef _UPDATE_H_
#define _UPDATE_H_


#include  <stdio.h>
#include  <string.h>
#include  "flwfifo.h"


#include "bsp_flash.h"
#include "crc.h"

#if      defined (STM32F10X_MD)          //中容量的
#define  CHIP_FLASH_SIZE          (64*1024)
#define  STM_SECTOR_SIZE          (1024)
#elif    defined (STM32F10X_HD) || defined (STM32F10X_CL) //大容量的
#define  CHIP_FLASH_SIZE          (512*1024)
#define  STM_SECTOR_SIZE          (2048)
#endif


#define  IAP_BASE_ADDR            (STM32_FLASH_BASE)
#define  IAP_FLASH_SIZE           (8*1024)                                   //8K

#define  EEPROM1_BASE_ADDR        (IAP_BASE_ADDR+IAP_FLASH_SIZE)
#define  EEPROM1_FLASH_SIZE       (4*1024)                                   //4K
#define  EEPROM2_BASE_ADDR        (EEPROM1_BASE_ADDR+EEPROM1_FLASH_SIZE)
#define  EEPROM2_FLASH_SIZE       (4*1024)                                   //4K

#define  APP_BASE_ADDR            (EEPROM2_BASE_ADDR+EEPROM2_FLASH_SIZE)
#define  APP_FLASH_SIZE           ((CHIP_FLASH_SIZE-IAP_FLASH_SIZE-EEPROM1_FLASH_SIZE-EEPROM2_FLASH_SIZE)/2)   //248K

#define  BACKUP_BASE_ADDR         (APP_BASE_ADDR+APP_FLASH_SIZE)
#define  BACKUP_FLASH_SIZE        (APP_FLASH_SIZE)                                                             //248K

#define  EEPROM_FLAG             0x5a5a



typedef struct UPDATE_Packets
{
    uint16_t CAN_Id;
    uint16_t len;
    uint8_t data[8];
}UPDATE_Packet_S;

#define UPDATE_FIFO_NUMS    8
#define UPDATE_FIFO_MASK       (UPDATE_FIFO_NUMS - 1)

typedef struct UPDATE_Fifo
{
    uint16_t wr; 
    uint16_t rd; 
    uint16_t num; 
    uint8_t mask; 
    UPDATE_Packet_S packets[UPDATE_FIFO_NUMS]; 
}UPDATE_Fifo_S;


typedef struct
{
    uint16_t   flag;
    uint32_t   file_len;     //更新文件的长度
    uint16_t   file_crc;     //更新文件的CRC16
}EEPROM_INFO;


typedef struct
{
    UPDATE_Fifo_S  rx_fifo;   //从can接收到的数据包fifo
}UPDATE_PACKET;



typedef struct
{
    uint8_t        data_index;     //索引
    uint32_t       wr_addr;        //
    flwfifo_s      fifo;           //从数据包中截取的程序文件数据流fifo
    uint32_t       file_size;      //字节数
    uint16_t        crc;            //CRC16校验码
    EEPROM_INFO    info;           //eeprom
}UPDATE_MOUDLE;


void app_update_init(void) ;
void app_update_rcv_packets(uint16_t const CAN_Id, uint8_t const *msg, uint16_t len);
void app_update_process(void);


#endif
