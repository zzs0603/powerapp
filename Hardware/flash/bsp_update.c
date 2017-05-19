#include "bsp_update.h"
#include "bsp_eeprom.h"
#include "can.h"

static  UPDATE_MOUDLE  update_mod;

#define UPDATE_BUF_SIZE    (16)  //16Ϊ��С����ֵ
#define UPDATE_BUF_TEMP_SIZE (UPDATE_BUF_SIZE/2)

static  uint8_t    update_buf[UPDATE_BUF_SIZE];
static  uint8_t    update_buf_temp[UPDATE_BUF_TEMP_SIZE];
static  uint8_t    update_ok_flag = 0; //

static void update_init(void)
{
    memset(&update_mod, 0, sizeof(update_mod));
    init_fifo(&update_mod.fifo, update_buf, UPDATE_BUF_SIZE);     //��ʼ��buf 
    update_mod.wr_addr = BACKUP_BASE_ADDR;
    update_mod.crc = 0xffff;
    update_mod.info.flag = EEPROM_FLAG;
}

//start
static uint8_t updata_erase_back_flash(void) //��Ҫ����ʱֱ�ӽ����������������
{
    uint8_t  re = 1;
    uint32_t  offaddr;
    uint16_t  secpos_s, secnum, i;
    uint8_t suc;
    
    if(update_ok_flag == 1) //����ѽ���������δ���� ��
    {
        return 2;
    }
    
    offaddr = BACKUP_BASE_ADDR - STM32_FLASH_BASE;
    secpos_s = offaddr / STM_SECTOR_SIZE;             //����FLASH�����������ʼ��ַ
    secnum =  BACKUP_FLASH_SIZE / STM_SECTOR_SIZE;    //����FLASH���������
    
    FLASH_Unlock();   //����
    for(i = 0; i < secnum; i++) //��������������
    {
        uint16_t index = i + secpos_s; //��index������
        suc = FLASH_ErasePage(index * STM_SECTOR_SIZE + STM32_FLASH_BASE);
        if(suc != FLASH_COMPLETE)
        {
            re = 0;
            break;
        }
    }
    FLASH_Lock();     //����

    EEPROM_Write(EEPROM_ID_SW_UPG_FLAG, 0); //���������ʾ
    
    return  re;
}

//run
static uint8_t  update_process(uint8_t *pbuf, uint16_t len)
{
    uint8_t re = 1;
    
    uint8_t index = pbuf[0];
    
    if(is_fifo_full(&update_mod.fifo)) //
        return 0;
    
    if(update_mod.data_index != index) //�����������
    {
        printf("need index:%d ,rcv index: %d\r\n", update_mod.data_index, index);
        return 0;
    }
    
    update_mod.crc = crc16_updateCRC(update_mod.crc, &pbuf[1], (len - 1)); //����CRC
    
    fifo_puts(&update_mod.fifo, (uint8_t *)&pbuf[1], len - 1); //�����ݷ���fifo
    
    //�ж�FIFO�е��ֽ���
    if(fifo_used_size(&update_mod.fifo) >= UPDATE_BUF_TEMP_SIZE) //128 ����flash��д128BYTES
    {
        fifo_gets(&update_mod.fifo, update_buf_temp, UPDATE_BUF_TEMP_SIZE); 
        FLASH_Unlock();   //����
        re = STMFLASH_Write_NoCheck(update_mod.wr_addr, (uint16_t *)update_buf_temp, UPDATE_BUF_TEMP_SIZE/2); //дFLASH
        FLASH_Lock();     //����
        if(re == 0) //��д���ɹ� 
        {
            return 0;
        }
        update_mod.wr_addr += UPDATE_BUF_TEMP_SIZE; //д��ַƫ��
    }
    
    update_mod.file_size += (len - 1); //bytes
    update_mod.data_index++;
    
    return 1;
}

//stop
static uint8_t update_end_process(uint8_t *pbuf, uint16_t len) //����ʱ����2��BYTE��CRC16У���� 
{
    uint16_t fifo_use_len, crc16_temp;
    uint32_t flash_addr;
    uint8_t re = 0;
    
    //����CRC16�ж�
    crc16_temp = ((uint16_t)pbuf[1] << 8) + pbuf[0];
    if(update_mod.crc != crc16_temp) //���CRC16���ɹ�
    {
        printf("need crc: %x , rcv crc: %x \r\n", update_mod.crc, crc16_temp);
        return 0;
    }
    printf("crc ok \r\n");
    //��FIFO��ʣ�������д��FLASH
    fifo_use_len = fifo_used_size(&update_mod.fifo);
    if(fifo_use_len > 0) //fifo�л��в���UPDATE_BUF_TEMP_SIZE���ȵ�������Ҫ����
    {
        fifo_gets(&update_mod.fifo, update_buf_temp, fifo_use_len); //��ȡ����
        if((fifo_use_len % 2) != 0) //��֤��2�ı���
        {
            fifo_use_len += 1;
        }
        FLASH_Unlock();   //����
        re = STMFLASH_Write_NoCheck(update_mod.wr_addr, (uint16_t *)update_buf_temp, fifo_use_len/2);
        FLASH_Lock();     //����
        
        if(re == 0)
        {
            return 0;
        }
    }
    
    //�������ļ�У��
    crc16_temp = 0xffff;
    flash_addr = BACKUP_BASE_ADDR; //��ʼ��ַ
    crc16_temp = crc16_updateCRC(crc16_temp, (uint8_t *)flash_addr, update_mod.file_size);
   
    if(update_mod.crc != crc16_temp)
    {
        return 0; //У����������
    }

    //дEEPROM �ø��±�־ д�����ݳ���
    update_mod.info.flag = EEPROM_FLAG;
    update_mod.info.file_len = update_mod.file_size;
    update_mod.info.file_crc = update_mod.crc;
   
    //��¼�ļ�����
    EEPROM_Write(EEPROM_ID_SW_LEN_LOW, (uint16_t)update_mod.info.file_len);
    EEPROM_Write(EEPROM_ID_SW_LEN_HIGH, (uint16_t)(update_mod.info.file_len >> 16));
    if(EEPROM_Read(EEPROM_ID_SW_LEN_LOW) != (uint16_t)update_mod.info.file_len)//��У��
    {
        return 0;
    }
    if(EEPROM_Read(EEPROM_ID_SW_LEN_HIGH) != ((uint16_t)(update_mod.info.file_len >> 16)))
    {
        return 0;
    }

    //��¼�ļ�CRC16
    EEPROM_Write(EEPROM_ID_SW_CRC, update_mod.info.file_crc);
    if(EEPROM_Read(EEPROM_ID_SW_CRC) != update_mod.info.file_crc)
    {
        return 0;
    }

    //��λ���±�־
    EEPROM_Write(EEPROM_ID_SW_UPG_FLAG, update_mod.info.flag);
    if(EEPROM_Read(EEPROM_ID_SW_UPG_FLAG) != update_mod.info.flag)
    {
        return 0;
    }
    
    update_ok_flag = 1; //�������ı�ʾ
    
    //����update_test��ʾ
    {
        uint16_t id = EEPROM_Read(EEPROM_ID_UPDATE_TEST);
        id++;
        EEPROM_Write(EEPROM_ID_UPDATE_TEST, id);
    }
    return 1;
}
//===================================================================
static  UPDATE_PACKET  update_packets;  //��can���յĹ��ڳ�����µ����ݰ�

//�������ݰ���ʼ��
void app_update_init(void)  
{
    //fifo init
    update_packets.rx_fifo.num = UPDATE_FIFO_NUMS;
    update_packets.rx_fifo.mask = UPDATE_FIFO_NUMS - 1;
    update_packets.rx_fifo.rd = update_packets.rx_fifo.wr = 0;
}

//�������ݰ�����
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


//����������
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


