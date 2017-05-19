#include "gpio.h"
#include "can.h"

#include "FlwFIFO.h"
#include "usart.h"
#include "TouchKey.h"
#include "FuelGauge.h"
#include "BoardPowerSupply.h"
#include "bsp_update.h"
#include "fw_version.h"
#include "delay.h"

#define CAN_STD_ID          (0xF0)
#define CAN_EXT_ID          (0x03)
static CanTxMsg TxMessage = {CAN_STD_ID, CAN_EXT_ID, CAN_ID_STD,\
    CAN_RTR_DATA, 1, { 0 } };
static CanRxMsg RxMsg;

//CAN 发送FIFO
static CAN_Fifo_S CAN_Fifo = {0, 0, CAN_FIFO_NUMS, CAN_FIFO_MASK};
const uint8_t can_frame_len = 8;

static uint8_t can_send_en_flag = 0;

/**< CAN的标识符屏蔽列表,只有在这个表内定义的标识符
* 才会通过CAN过滤器,后续需求变更需要确认这张表定义是否正确
* */
CAN_ID_E CANFilterTable[CAN_ID_MASK_COUNTERS] = { CAN_ID_WAKE_OR_SLEEP, CAN_ID_FUN_TEST, CAN_ID_UPDATE,\
                                                  CAN_ID_REQUEST_VERSION, CAN_ID_REQUEST_STATE,\
                                                  CAN_ID_POWEROFF_ACK, CAN_ID_DRIVERBOARD_POWERON_REASON};


const uint16_t filter_scale = CAN_FilterScale_16bit;    //16位宽模式
const uint16_t filter_mode = CAN_FilterMode_IdMask;     //标识符屏蔽模式
const uint16_t filter_mask = 0xFC00;

/*
 * @param baudrate -- CAN_BAUDRATE_100K,CAN_BAUDRATE_125K,CAN_BAUDRATE_250K,
 *      CAN_BAUDRATE_500K, CAN_BAUDRATE_800K,CAN_BAUDRATE_1M
 * @Param mode -- CAN_Mode_Normal,CAN_Mode_LoopBack,
 *      CAN_Mode_Silent,CAN_Mode_Silent_LoopBack
 * */
static void CAN_Base_Init(CAN_BAUDRATE_E baudrate, uint8_t mode)
{
    CAN_InitTypeDef     CAN_InitStructure;
    NVIC_InitTypeDef    NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(CAN_GPIO_CLK, ENABLE);
    RCC_APB1PeriphClockCmd(CAN_CLK, ENABLE);

#ifdef BOARD_DEFINE_MOTION
    GPIO_PinRemapConfig(GPIO_Remap1_CAN1, ENABLE);
#endif
    GPIO_Configuration(CAN_GPIO_PORT, CAN_GPIO_RX_PIN, GPIO_Mode_IPU);
    GPIO_Configuration(CAN_GPIO_PORT, CAN_GPIO_TX_PIN, GPIO_Mode_AF_PP);

    //NVIC initialize
    NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* CAN register init */
    CAN_DeInit(CANx);
    CAN_StructInit(&CAN_InitStructure);
    /* CAN cell init */
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = DISABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = mode;

    //优先选择采样率87.5%
    //baudrate = 36Mhz / ((1 + BS1 + BS2) * PSC);
    switch(baudrate)
    {
    case CAN_BAUDRATE_100K:
        CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
        CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;
        CAN_InitStructure.CAN_BS2 = CAN_BS2_1tq;
        CAN_InitStructure.CAN_Prescaler = 45;
        break;
    case CAN_BAUDRATE_125K:
        CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
        CAN_InitStructure.CAN_BS1 = CAN_BS1_13tq;
        CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;
        CAN_InitStructure.CAN_Prescaler = 18;
        break;
    case CAN_BAUDRATE_500K:
        CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
        CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;
        CAN_InitStructure.CAN_BS2 = CAN_BS2_1tq;
        CAN_InitStructure.CAN_Prescaler = 9;
        break;
    case CAN_BAUDRATE_800k:
        CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
        CAN_InitStructure.CAN_BS1 = CAN_BS1_12tq;
        CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;
        CAN_InitStructure.CAN_Prescaler = 3;
        break;
    case CAN_BAUDRATE_1M:
        CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
        CAN_InitStructure.CAN_BS1 = CAN_BS1_15tq;
        CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;
        CAN_InitStructure.CAN_Prescaler = 2;
        break;
    default:    //default baudrate 250KHZ
        CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
        CAN_InitStructure.CAN_BS1 = CAN_BS1_13tq;
        CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;
        CAN_InitStructure.CAN_Prescaler = 9;
        break;
    }
    CAN_Init(CANx, &CAN_InitStructure);
}

/** \brief 设置CAN的过滤器规则
 *
 * \param mode,标识符屏蔽模式,分为标识符列表或掩码模式
 * \param scale,过滤器的位宽,分16bits 和 32bits 两种
 * \return 无
 *
 */
static void CAN_Filter_Config(uint8_t mode, uint8_t scale)
{
    uint8_t i;
    CAN_FilterInitTypeDef   CAN_FilterInitStructure;

    CAN_FilterInitStructure.CAN_FilterMode = mode;      //标识符屏蔽模式
    CAN_FilterInitStructure.CAN_FilterScale = scale;    //1个16位宽过滤器
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;

    if(filter_scale)
    { //32bits scale
        for(i = 0; i < CAN_ID_MASK_COUNTERS; i++)
        {
            CAN_FilterInitStructure.CAN_FilterNumber = i;
            //设置期望ID,移位至高6位
            CAN_FilterInitStructure.CAN_FilterIdHigh = CANFilterTable[i] << 5;
            CAN_FilterInitStructure.CAN_FilterIdLow = 0;
            //设置关心的bits
            CAN_FilterInitStructure.CAN_FilterMaskIdHigh = filter_mask;
            CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0;
            CAN_FilterInit(&CAN_FilterInitStructure);
        }
    }
    else
    { //16bits scale
         for(i = 0; i < CAN_ID_MASK_COUNTERS; i += 2)
        {
            CAN_FilterInitStructure.CAN_FilterNumber = i >> 1;
            //设置期望ID,移位至高6位
            CAN_FilterInitStructure.CAN_FilterIdLow = CANFilterTable[i] << 5;
            CAN_FilterInitStructure.CAN_FilterMaskIdLow = filter_mask;

            //设置关心的bits
            if(i < CAN_ID_MASK_COUNTERS - 1)
            {
                CAN_FilterInitStructure.CAN_FilterIdHigh = \
                                CANFilterTable[i + 1] << 5;
                CAN_FilterInitStructure.CAN_FilterMaskIdHigh = filter_mask;
            }
            else
            {
                CAN_FilterInitStructure.CAN_FilterIdHigh = \
                                CANFilterTable[i] << 5;
                CAN_FilterInitStructure.CAN_FilterMaskIdHigh = filter_mask;
            }

            CAN_FilterInit(&CAN_FilterInitStructure);
        }

    }
}

void FlwCAN_Init(CAN_BAUDRATE_E baudrate, uint8_t mode)
{
    /* set baudrate and mode(normal or loopback) */
    CAN_Base_Init(baudrate, mode);

        /* CAN filter init */
    CAN_Filter_Config(filter_mode, filter_scale);
    
    /* Transmit */
    CAN_ITConfig(CANx, CAN_IT_FMP0, ENABLE);
}

/* CAN 接收接口,在中断内调用,存入FIFO */
void CAN_Protocol_Packet(uint16_t const CAN_Id, uint8_t const *data, uint16_t len)
{
    uint16_t i, index;
    CAN_Packet_S *packet;

    if(is_fifo_full(&CAN_Fifo))
        return;

    index = CAN_Fifo.wr & CAN_Fifo.mask;
    packet = &CAN_Fifo.packets[index];
    packet->CAN_Id = CAN_Id;
    packet->len = len;
    for(i = 0; i < len; i++)
        packet->data[i] = data[i];
    CAN_Fifo.wr++;
}

/* CAN1 接收中断 */
void USB_LP_CAN1_RX0_IRQHandler(void)
{
    if(CAN_GetITStatus(CANx, CAN_IT_FMP0))
    {
        CAN_Receive(CANx, 0, &RxMsg);
        
        CAN_Protocol_Packet(RxMsg.StdId, RxMsg.Data, RxMsg.DLC); //收入缓存
        CAN_Protocol_Process();
        app_update_process(); 
        
        CAN_ClearITPendingBit(CANx, CAN_IT_FMP0);
    }
}

static int CAN_Transmit_Frame(uint8_t const *msg, uint8_t len)
{
    uint8_t i, mbox;
    uint16_t timeout = 0x400;

    if(len > can_frame_len)
        len = can_frame_len;

    for(i = 0; i < len; i++)
        TxMessage.Data[i] = msg[i];
    mbox = CAN_Transmit(CANx, &TxMessage);
    /* 等待发送完成 */
    while(CAN_TransmitStatus(CANx, mbox) != CAN_TxStatus_Ok)
    {
        if(timeout-- <= 0)
            return -1;
    }
    return 0;
}

void CAN_Send_Message(uint16_t const CAN_Id, uint8_t const *msg, \
                             uint16_t len)
{
    uint16_t packets, remains = len;
    uint8_t i = 0;

    if(can_send_en_flag == 0 && CAN_Id != CAN_ID_STATE_ACK \
                             && CAN_Id != CAN_ID_POWERON_EVENT\
                             && CAN_Id != (CAN_ID_FUN_TEST + HANDSHAKE)\
                             && CAN_Id != (CAN_ID_FUN_TEST + PCBPOWER)\
                             && CAN_Id != (CAN_ID_FUN_TEST + PCBTOUCH))//CAN发送标示未使能
        return;
    
    /* 计算8字节CAN帧数 */
    packets = ((len + can_frame_len - 1) >> 3);

    TxMessage.StdId = CAN_Id;
    if(packets > 0)
    {
        /* 发送packets个8字节CAN帧 */
        for(i = 0; i < packets - 1; i++)
        {
            TxMessage.DLC =  can_frame_len;
            if(CAN_Transmit_Frame(msg + (i * can_frame_len), \
                                  can_frame_len) < 0)
                return;
            remains -= can_frame_len;
        }
        /* 发送剩余部分 */
        if(remains > 0)
        {
           TxMessage.DLC = remains;
           if(CAN_Transmit_Frame(msg + (i * can_frame_len), remains) < 0)
               return;
        }
    }
    else
    {
        TxMessage.DLC = remains;
        if(CAN_Transmit_Frame(msg + (i * can_frame_len), remains) < 0)
           return;
    }
}

///* CAN 发送接口,取FIFO */
//void CAN_Packet_Send(void)
//{
//    uint16_t index;
//    CAN_Packet_S *packet;

//    if(is_fifo_empty(&CAN_Fifo))
//        return;

//    index = CAN_Fifo.rd & CAN_Fifo.mask;
//    packet = &CAN_Fifo.packets[index];
//    CAN_Send_Message(packet->CAN_Id, packet->data, packet->len);
//    CAN_Fifo.rd++;
//}



static int CAN_Protocol_Parse(uint32_t CAN_Id, uint8_t const *msg, uint16_t len)
{
    uint16_t master_id = CAN_Id & 0x7E0;
    uint8_t slave_id = CAN_Id & 0x1F;

    switch(master_id)
    {
        case CAN_ID_WAKE_OR_SLEEP:
            printf("Wake or Sleep\n");
            break;
        
        case CAN_ID_FUN_TEST: //工装测试
        {
            if(slave_id == HANDSHAKE)    //CAN通讯握手
            {
                if(msg[0] == 0x5a) //如果收到握手
                {   
                    uint8_t buf = 0x5a;
                    BoardPower_SetState(ALL_BOARD_POWER_ID, 0);
                    CAN_Send_Message(CAN_ID_FUN_TEST + HANDSHAKE, &buf, 1);
                }
            }
            else if(slave_id == PCBPOWER) //电源板测试命令
            {
                if(msg[0] == MAIN_DRV_POWERON) //将POWER 打开
                {
                    BoardPower_SetState(ALL_BOARD_POWER_ID, 1);
                }
                
                if(msg[0] == CHARGESTATE)// 1,检测电量计 充电状态
                {
                   uint8_t val[2];
                   val[0] = CHARGESTATE;
                   val[1] = FuelGauge_Check();
                   CAN_Send_Message(CAN_ID_FUN_TEST + PCBPOWER, val, 2); // 0或1 表示电量计是否为OK
                }
            }
            else if(slave_id == PCBTOUCH) //触摸板测试命令
            {
                  if(msg[0] == 1)
                  {
                      TouchKey_Check_Flag(1);
                  }
                  else
                  {
                      TouchKey_Check_Flag(0);
                  }
            }
        }break;
        
        case CAN_ID_UPDATE://固件升级
        {
             app_update_rcv_packets(CAN_Id, msg, len); //将CAN数据直接交给更新模块的缓存中
        }break;
        
        case CAN_ID_REQUEST_VERSION: //版本请求
        {
            if(slave_id == BOARD_POWER) 
            {
                uint8_t version[8], re;
                re = version_view(version, 8);
                if(re == 0) //读取版本失败
                {
                    CAN_Send_Message(CAN_ID_VERSION_ACK, version, 1);
                }
                else
                {
                    CAN_Send_Message(CAN_ID_VERSION_ACK, version, 8);
                }
            }
        }break;
        
        case CAN_ID_REQUEST_STATE: //开机状态请求
        {
                uint8_t version[8], re, buf;
                re = version_view(version, 8);
                if(re == 0) //读取版本失败
                {
                    can_send_en_flag = 0;
                    buf = 0;
                    CAN_Send_Message(CAN_ID_STATE_ACK, &buf, 1);
                }
                else
                {
                    can_send_en_flag = 2;
                    buf = 1;
                    CAN_Send_Message(CAN_ID_STATE_ACK, &buf, 1);
                    Report_Battery_State_Init(); //上传电量状态
                }
        }break;
        
        
        case CAN_ID_POWEROFF_ACK:
        {
            if(slave_id == POWEROFF_ACK) //关闭主机电源
            {
               if(BoardPower_GetState()) //
               {
                  //BoardPower_SetState(ALL_BOARD_POWER_ID, 0); //关闭主控板电源
                   BoardPower_Poweroff_flag(1); //设置关闭电源标示
               }
            }
        }break;
        
        case CAN_ID_DRIVERBOARD_POWERON_REASON: //驱动板上电原因
        {
            uint8_t re[3];
            if(msg[0] == 0x00) //查询
            {
              can_send_en_flag = 1; //上电握手
              re[0] = get_driver_board_poweron_reason();
              if(get_low_power_hold_key_flag())
              {
                  set_low_power_hold_key_flag(0);
                  re[0] = 4;
              }
                
              re[1] = Get_SOC();
              if(GetAdapterState())
              {
                  re[2] = 1;
              }
              else
              {
                  re[2] = 0;
              }
              CAN_Send_Message(CAN_ID_DRIVERBOARD_POWERON_REASON, re, 3);
            }
            else if(msg[0] == 0xf2) //关闭驱动板电源
            {
                can_send_en_flag = 0;
                //set_driver_board_reason_3_2_flag(1);
                BoardPower_SetState(DRIVER_BOARD_POWER_ID, 0);
            }
        }break;
        
        default:
            break;
    }
    return 0;
}



void CAN_Protocol_Process(void)
{
    uint16_t index;
    CAN_Packet_S *packet;

    if(is_fifo_empty(&CAN_Fifo))
    return;

    index = CAN_Fifo.rd & CAN_Fifo.mask;
    packet = &CAN_Fifo.packets[index];
    CAN_Protocol_Parse(packet->CAN_Id, packet->data, packet->len);
    CAN_Fifo.rd++;
}


uint8_t CAN_Send_Flag_State(void)
{
    return can_send_en_flag;
}

void set_CAN_Send_Flag(uint8_t flag)
{
    can_send_en_flag = flag;
}
