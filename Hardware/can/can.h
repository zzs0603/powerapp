#ifndef _FLW_CAN_H
#define _FLW_CAN_H
#include "stdint.h"

#define BOARD_DEFINE_POWER_MANAGE     //定义功能板类型::电源管理

#define CANx    (CAN1)
/**< CAN的GPIO定义*/
#define CAN_CLK                         (RCC_APB1Periph_CAN1)
#define CAN_GPIO_CLK                    (RCC_APB2Periph_GPIOA)
#define CAN_GPIO_PORT                   (GPIOA)
#define CAN_GPIO_RX_PIN                 (GPIO_Pin_11)
#define CAN_GPIO_TX_PIN                 (GPIO_Pin_12)

typedef enum{
    CAN_BAUDRATE_100K,      /* 100Kbps */
    CAN_BAUDRATE_125K,      /* 125Kbps */
    CAN_BAUDRATE_250K,      /* 250Kbps */
    CAN_BAUDRATE_500K,      /* 500Kbps */
    CAN_BAUDRATE_800k,      /* 800Kbps */
    CAN_BAUDRATE_1M,        /* 1Mbps */
}CAN_BAUDRATE_E;

//typedef enum{
//    OBSTACLE_TOP,
//    OBSTACLE_LEFT,
//    OBSTACLE_RIGHT,
//    OBSTACLE_BACK,
//}Obstacle_Type_E;

//typedef enum{
//    FALL_LEFT,
//    FALL_RIGHT,
//    FALL_BACK,
//}Fall_Type_E;

//typedef enum
//{
//    MOTION_ALARM_OBSTACLE,  /* 障碍告警(0顶部,1左侧,2右侧,3背部) */
//    MOTION_ALARM_FALL,      /* 跌落告警(0左侧,1右侧,2背部) */
//}Motion_Alarm_E;

//typedef enum{
//    MOTOR_CTL_STOP,                 /* 停止运动(0) */
//    MOTOR_CTL_FORWARD = 0x02,       /* 前进指定距离(4--距离,2--速度) */
//    MOTOR_CTL_BACK = 0x04,          /* 后退指定距离(4--距离,2--速度) */
//    MOTOR_CTL_TURN = 0x06,          /* 转向(4--角度) */
//    MOTOR_CTL_SPEED = 0x08,         /* 差速控制(2--速度,2--速度) */
//    MOTOR_CTL_FORWARD_ACK = 0x10,   /* 前进指令应答(4--已前进距离) */
//    MOTOR_CTL_BACK_ACK = 0x11,      /* 后退指令应答(4--已后退距离) */
//    MOTOR_CTL_TURN_ACK = 0x12,      /* 转向应答(4--转向角度) */
//    MOTOR_CTL_SPEED_ACK = 0x13,     /* 差速应答(4--距离,4--距离) */
//}Motor_Ctl_E;

//typedef enum{
//    SERVO_CTL_STOP,
//    SERVO_CTL_INC_TURN = 0x01,      /* 增量转动(2--水平角度,2--竖直角度) */
//    SERVO_CTL_ABS_TURN = 0x02,      /* 绝对转动(2--水平角度,2--竖直角度) */
//    SERVO_CTL_ZERO = 0x08,          /* 位置归中 */
//    SERVO_CTL_TURN_ACK = 0x10,      /* 转动应答(2--水平角度,2--竖直角度) */
//}Servo_Ctl_E;

typedef enum{
    SEARCH_CHARGER_STOP,
    SEARCH_CHARGER_BEGIN,
}Search_Charger_En_E;

typedef enum{
    SEARCH_RESULT_FAIL,
    SEARCH_RESULT_OK,
}Search_Charger_Result_E;

typedef enum{
    SEARCH_CHARGER_ENABLE,          /* 寻找充电座使能(1--0停止,1启动) */
    SEARCH_CHARGER_RESULT,          /* 寻找充电座反馈(1--0未找到,1找到) */
}Auto_Search_Charger_E;

typedef enum{
    CHARGE_IDLE,            /* 未充电 */
    CHARGE_WORKING,         /* 充电中 */
    CHARGE_COMPLETE,        /* 充电完成 */
}Charge_State_E;

typedef enum
{
    HANDSHAKE,   //握手测试CAN通讯
    PCBLED,     //灯板检测
    PCBDRIVER,  //驱动板
    PCBENCODER, //码盘
    PCBPOWER,   //电源板
    PCBTOUCH,   //触摸板
}Test_State_E;


typedef enum
{
 MAIN_DRV_POWERON = 1,
 CHARGESTATE = 2,
}PCBPOWER_State_E;

typedef enum
{
    UPDATE_INIT = 0,
    UPDATE_RUN,
    UPDATE_END,
}FIRMWARE_UPDATE_E;  //固件升级过程控制

typedef enum
{
    BOARD_POWER = 3,
}BOARD_INDEX_E;


typedef enum
{
    POWEROFF_ACK = 3,
}POWEROFF_ACK_E;

typedef enum
{
/* 电源管理板CAN ID定义 */
#ifdef BOARD_DEFINE_POWER_MANAGE
    CAN_ID_WAKE_OR_SLEEP = 0x240, /* 休眠与唤醒 */
    CAN_ID_FUN_TEST = 0x300,      /*测试*/
    CAN_ID_UPDATE = 0x540,        /*固件升级*/
    CAN_ID_REQUEST_VERSION = 0x4c0, /*版本号查询*/
    CAN_ID_REQUEST_STATE = 0x600, /*状态查询*/
    CAN_ID_POWEROFF_ACK = 0x640,  /*主控板关机事件应答*/
    CAN_ID_DRIVERBOARD_POWERON_REASON = 0x680, /*驱动板上电原因*/
    CAN_ID_MASK_COUNTERS = 7,
    /* used for send */
    CAN_ID_CHARGE_STATE = 0x1C0,   /* 电池充电状态 */
    CAN_ID_BATTERY_STATE = 0x420,  /* 电池电量及状态 */
    CAN_ID_VERSION_ACK = 0x4e3,    /*版本应答*/
    CAN_ID_STATE_ACK = 0x623,      /*开机状态应答*/
    CAN_ID_CLICK_EVENT = 0x640,    /*单击事件*/
    CAN_ID_DOUBCLK_EVENT = 0x641,  /*双击事件*/
    CAN_ID_POWERON_EVENT = 0x642,  /*长按开机事件*/
    CAN_ID_POWEROFF_EVENT = 0x643, /*长按关机事件*/
    CAN_ID_DEBUG_EVENT = 0x644,    /*长按调试事件*/
#endif
} CAN_ID_E;

typedef struct CAN_Packet{
    uint16_t CAN_Id;
    uint16_t len;
    uint8_t data[8];
}CAN_Packet_S;

#define CAN_FIFO_NUMS       (8)
#define CAN_FIFO_MASK       (CAN_FIFO_NUMS - 1)

typedef struct CAN_Fifo{
    uint16_t wr;        /* 写指针 */
    uint16_t rd;        /* 读指针 */
    uint16_t num;       /* 缓存总数 */
    uint8_t mask;       /* 掩码 */
    CAN_Packet_S packets[CAN_FIFO_NUMS];  /* 缓存 */
}CAN_Fifo_S;

void FlwCAN_Init(CAN_BAUDRATE_E baudrate, uint8_t mode);

void CAN_Send_Message(uint16_t const CAN_Id, uint8_t const *msg, uint16_t len);
//void CAN_Protocol_Packet(uint16_t const CAN_Id, uint8_t const *data, uint16_t len);
//void CAN_Packet_Send(void);
void CAN_Protocol_Process(void);
uint8_t CAN_Send_Flag_State(void);
void set_CAN_Send_Flag(uint8_t flag);

#endif // for #ifndef _FLW_CAN_H


