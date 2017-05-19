#include "bsp_update.h"
#include "bsp_eeprom.h"
#include "can.h"

static  UPDATE_MOUDLE  update_mod;

#define UPDATE_BUF_SIZE    (16)  //16为最小设置值
#define UPDATE_BUF_TEMP_SIZE (UPDATE_BUF_SIZE/2)

static  uint8_t    update_buf[UPDATE_BUF_SIZE];
static  uint8_t    update_buf_temp[UPDATE_BUF_TEMP_SIZE];
static  uint8_t    update_ok_flag = 0; //

static void update_init(void)
{
    memset(&update_mod, 0, sizeof(update_mod));
    init_fifo(&update_mod.fifo, update_buf, UPDATE_BUF_SIZE);     //初始化buf 
    update_mod.wr_addr = BACKUP_BASE_ADDR;
    update_mod.crc = 0xffff;
    update_mod.info.flag = EEPROM_FLAG;
}

//start
static uint8_t updata_erase_back_flash(void) //需要更新时直接将整个备份区域擦除
{
    uint8_t  re = 1;
    uint32_t  offaddr;
    uint16_t  secpos_s, secnum, i;
    uint8_t suc;
    
    if(update_ok_flag == 1) //如果已进行升级但未重启 则不
    {
        return 2;
    }
    
    offaddr = BACKUP_BASE_ADDR - STM32_FLASH_BASE;
    secpos_s = offaddr / STM_SECTOR_SIZE;             //备份FLASH块的扇区的起始地址
    secnum =  BACKUP_FLASH_SIZE / STM_SECTOR_SIZE;    //备份FLASH块的扇区数
    
    FLASH_Unlock();   //解锁
    for(i = 0; i < secnum; i++) //擦除整个备份区
    {
        uint16_t index = i + secpos_s; //第index个扇区
        suc = FLASH_ErasePage(index * STM_SECTOR_SIZE + STM32_FLASH_BASE);
        if(suc != FLASH_COMPLETE)
        {
            re = 0;
            break;
        }
    }
    FLASH_Lock();     //上锁

    EEPROM_Write(EEPROM_ID_SW_UPG_FLAG, 0); //清除升级标示
    
    return  re;
}

//run
static uint8_t  update_process(uint8_t *pbuf, uint16_t len)
{
    uint8_t re = 1;
    
    uint8_t index = pbuf[0];
    
    if(is_fifo_full(&update_mod.fifo)) //
        return 0;
    
    if(update_mod.data_index != index) //索引索引检测
    {
        printf("need index:%d ,rcv index: %d\r\n", update_mod.data_index, index);
        return 0;
    }
    
    update_mod.crc = crc16_updateCRC(update_mod.crc, &pbuf[1], (len - 1)); //计算CRC
    
    fifo_puts(&update_mod.fifo, (uint8_t *)&pbuf[1], len - 1); //将数据放入fifo
    
    //判断FIFO中的字节数
    if(fifo_used_size(&update_mod.fifo) >= UPDATE_BUF_TEMP_SIZE) //128 就往flash中写128BYTES
    {
        fifo_gets(&update_mod.fifo, update_buf_temp, UPDATE_BUF_TEMP_SIZE); 
        FLASH_Unlock();   //解锁
        re = STMFLASH_Write_NoCheck(update_mod.wr_addr, (uint16_t *)update_buf_temp, UPDATE_BUF_TEMP_SIZE/2); //写FLASH
        FLASH_Lock();     //上锁
        if(re == 0) //编写不成功 
        {
            return 0;
        }
        update_mod.wr_addr += UPDATE_BUF_TEMP_SIZE; //写地址偏移
    }
    
    update_mod.file_size += (len - 1); //bytes
    update_mod.data_index++;
    
    return 1;
}

//stop
static uint8_t update_end_process(uint8_t *pbuf, uint16_t len) //结束时发送2个BYTE的CRC16校验码 
{
    uint16_t fifo_use_len, crc16_temp;
    uint32_t flash_addr;
    uint8_t re = 0;
    
    //传输CRC16判断
    crc16_temp = ((uint16_t)pbuf[1] << 8) + pbuf[0];
    if(update_mod.crc != crc16_temp) //如果CRC16不成功
    {
        printf("need crc: %x , rcv crc: %x \r\n", update_mod.crc, crc16_temp);
        return 0;
    }
    printf("crc ok \r\n");
    //将FIFO中剩余的数据写入FLASH
    fifo_use_len = fifo_used_size(&update_mod.fifo);
    if(fifo_use_len > 0) //fifo中还有不够UPDATE_BUF_TEMP_SIZE长度的数据需要保存
    {
        fifo_gets(&update_mod.fifo, update_buf_temp, fifo_use_len); //获取数据
        if((fifo_use_len % 2) != 0) //保证是2的倍数
        {
            fifo_use_len += 1;
        }
        FLASH_Unlock();   //解锁
        re = STMFLASH_Write_NoCheck(update_mod.wr_addr, (uint16_t *)update_buf_temp, fifo_use_len/2);
        FLASH_Lock();     //上锁
        
        if(re == 0)
        {
            return 0;
        }
    }
    
    //读更新文件校验
    crc16_temp = 0xffff;
    flash_addr = BACKUP_BASE_ADDR; //起始地址
    crc16_temp = crc16_updateCRC(crc16_temp, (uint8_t *)flash_addr, update_mod.file_size);
   
    if(update_mod.crc != crc16_temp)
    {
        return 0; //校验数据有误
    }

    //写EEPROM 置更新标志 写入数据长度
    update_mod.info.flag = EEPROM_FLAG;
    update_mod.info.file_len = update_mod.file_size;
    update_mod.info.file_crc = update_mod.crc;
   
    //记录文件长度
    EEPROM_Write(EEPROM_ID_SW_LEN_LOW, (uint16_t)update_mod.info.file_len);
    EEPROM_Write(EEPROM_ID_SW_LEN_HIGH, (uint16_t)(update_mod.info.file_len >> 16));
    if(EEPROM_Read(EEPROM_ID_SW_LEN_LOW) != (uint16_t)update_mod.info.file_len)//读校验
    {
        return 0;
    }
    if(EEPROM_Read(EEPROM_ID_SW_LEN_HIGH) != ((uint16_t)(update_mod.info.file_len >> 16)))
    {
        return 0;
    }

    //记录文件CRC16
    EEPROM_Write(EEPROM_ID_SW_CRC, update_mod.info.file_crc);
    if(EEPROM_Read(EEPROM_ID_SW_CRC) != update_mod.info.file_crc)
    {
        return 0;
    }

    //置位更新标志
    EEPROM_Write(EEPROM_ID_SW_UPG_FLAG, update_mod.info.flag);
    if(EEPROM_Read(EEPROM_ID_SW_UPG_FLAG) != update_mod.info.flag)
    {
        return 0;
    }
    
    update_ok_flag = 1; //升级过的标示
    
    //更新update_test表示
    {
        uint16_t id = EEPROM_Read(EEPROM_ID_UPDATE_TEST);
        id++;
        EEPROM_Write(EEPROM_ID_UPDATE_TEST, id);
    }
    return 1;
}
//===================================================================
static  UPDATE_PACKET  update_packets;  //从can接收的关于程序更新的数据包

//更新数据包初始化
void app_update_init(void)  
{
    //fifo init
    update_packets.rx_fifo.num = UPDATE_FIFO_NUMS;
    update_packets.rx_fifo.mask = UPDATE_FIFO_NUMS - 1;
    update_packets.rx_fifo.rd = update_packets.rx_fifo.wr = 0;
}

//跟新数据包接收
void app_update_rcv_packets(uint16_t const CAN_Id, uint8_t const *msg, uint16_t len) 
{
    uint16_t i, index;
    UPDATE_Packet_S *packet;

    if(is_fifo_full(&update_packets.rx_fifo))
        return;
    
    index = update_packets.rx_fifo.wr & update_packets.rx_fifo.mask;

    packet = &update_packets.rx_fifo.packets[index];
    packet->CAN_Id = CAN_Id;
    packet->len = len;

    for(i = 0; i < len; i++)
        packet->data[i] = msg[i];

    update_packets.rx_fifo.wr++;
}


//更新任务处理
void app_update_process(void)  
{
    u8                 index, slaveId, res;
    UPDATE_Packet_S    packet;
    //u16                masterId;

    while(1)
    {
        if(is_fifo_empty(&update_packets.rx_fifo))
                return;
        
        index = update_packets.rx_fifo.rd & update_packets.rx_fifo.mask;
        packet = update_packets.rx_fifo.packets[index];
        update_packets.rx_fifo.rd++;

        //masterId = packet.CAN_Id & 0x7E0;
        slaveId = packet.CAN_Id & 0x1F;     

        if(slaveId == UPDATE_INIT)    //
        {
            update_init();                   //
            res = updata_erase_back_flash();  //
            CAN_Send_Message(CAN_ID_UPDATE + UPDATE_INIT, &res, 1); //
        }
        else if(slaveId == UPDATE_RUN) //
        {
            res = update_process(packet.data, packet.len);  //
            if(res == 0) //
            {
                CAN_Send_Message(CAN_ID_UPDATE + UPDATE_RUN, &res, 1); //
            }
        }
        else if(slaveId == UPDATE_END)  //
        {
            res = update_end_process(packet.data, packet.len);
            CAN_Send_Message(CAN_ID_UPDATE + UPDATE_END, &res, 1); 
        }
    }
}


