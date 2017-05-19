#include "BoardPowerSupply.h"

#include "delay.h"


static uint8_t  poweroff_flag = 0;
static uint16_t poweroff_cnt = 0;
static uint8_t  driver_board_poweron_reason = 0; //驱动板上电原因
static uint8_t  driver_board_reason_3_2_flag = 0;

static void Main_Board_Power_Supply_Init(void)
{
    //时钟使能
    RCC_APB2PeriphClockCmd(MAIN_BOARD_POWER_GPIO_CLK, ENABLE);
    //GPIO配置
    GPIO_Configuration(MAIN_BOARD_POWER_EN_PORT, MAIN_BOARD_POWER_EN_PIN, GPIO_Mode_Out_PP);
    //默认关闭电源
    MAIN_BOARD_POWER_EN = 0;
}

static void Driver_Board_Power_Supply_Init(void)
{
    //时钟使能
    RCC_APB2PeriphClockCmd(DRIVER_BOARD_POWER_GPIO_CLK, ENABLE);
    //GPIO配置
    GPIO_Configuration(DRIVER_BOARD_POWER_EN_PORT, DRIVER_BOARD_POWER_EN_PIN, GPIO_Mode_Out_PP);
    //默认关闭电源
    DRIVER_BOARD_POWER_EN = 1; 
}


void BoardPowerSupply_Init(void)
{
    Main_Board_Power_Supply_Init();
    Driver_Board_Power_Supply_Init();
}




void BoardPower_Poweroff_flag(uint8_t flag)
{
    poweroff_cnt = 0;
    poweroff_flag = flag;
}

/* add by sch 20160421 */
uint8_t Get_BoardPower_Poweroff_flag(void)
{
    return poweroff_flag;
}   

void  BoardPower_Poweroff_Ctrl(void)  
{
    if(poweroff_flag)
    {
        if(++poweroff_cnt >= 2500)
        {
            poweroff_cnt = 0; //清零
            poweroff_flag = 0; //清零
            set_CAN_Send_Flag(0);
            BoardPower_SetState(ALL_BOARD_POWER_ID, 0);
        }
    }
}


void BoardPower_SetState(uint8_t power_id, uint8_t power_state)
{
    switch(power_id)
    {
        case MAIN_BOARD_POWER_ID:
        {
            MAIN_BOARD_POWER_EN = power_state;
        }break;
        
        case DRIVER_BOARD_POWER_ID:
        {
            DRIVER_BOARD_POWER_EN = power_state;
        }break;
        
        case ALL_BOARD_POWER_ID:
        {
            MAIN_BOARD_POWER_EN = power_state;
            DRIVER_BOARD_POWER_EN = power_state;
        }break;
    }
}

uint8_t BoardPower_GetState(void)
{
    uint8_t re = 0;

    if(MAIN_BOARD_POWER_EN == 1)
    {
      re = 1;
    }
    else
    {
      re = 0;
    }

    return re;
}

//
void set_driver_board_poweron_reason(uint8_t reason)
{
    driver_board_poweron_reason = reason;
}

uint8_t get_driver_board_poweron_reason(void)
{
    return driver_board_poweron_reason;
}

void set_driver_board_reason_3_2_flag(uint8_t flag)
{
    driver_board_reason_3_2_flag = flag;
}

uint8_t get_driver_board_reason_3_2_flag(void)
{
    return driver_board_reason_3_2_flag;
}
