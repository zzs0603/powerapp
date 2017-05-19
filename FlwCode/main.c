#include "delay.h"
#include "timer.h"
#include "stdio.h"
#include "usart.h"
#include "can.h"
#include "gpio.h"
#include "i2c.h"
#include "Touchkey.h"
#include "FuelGauge.h"
#include "BoardPowerSupply.h"
#include "bsp_eeprom.h"
#include "bsp_update.h"
#include "fw_version.h"

#include "bq34z100.h"

#include "beep.h"

int main(void)
{
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x4000); //设置中断向量表
    delay_init();//延时函数初始化
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);//设置NVIC中断分组2:2位抢占优先级，2位响应优先级

    usart_init(115200);//串口初始化

    FlwCAN_Init(CAN_BAUDRATE_125K, CAN_Mode_Normal);//初始化CAN
 
    TouchKey_Init(); //触摸开关捕获定时器初始化
   
    FuelGauge_Init(); //电量计初始化
    
    driver_board_poweron_reason_ctrl(); 
    
    SYSTIM_BASE_Init();//系统时基初始化

    BoardPowerSupply_Init();   //电源开关管理初始化
    
    EEPROM_Init(0);

    app_update_init();    //固件升级初始化
    
    beep_init();
    
    printf("build_time: %s-%s\r\n", __DATE__, __TIME__);
    printf("version: %d-%d-%d-%d\r\n", VM, HW, SW, REPE);
    printf("Init OK.\r\n");

    while(1)
    {
      if(SYSTIM_BASE_FlagGet(SYSTIM_BASE_1S))
      {
          SYSTIM_BASE_FlagClear(SYSTIM_BASE_1S); //清除定时标志
          i2c_Report_Battery_State_With_Uart();
      }
      
      if(SYSTIM_BASE_FlagGet(SYSTIM_BASE_1MS)) //1ms定时到
      {
          SYSTIM_BASE_FlagClear(SYSTIM_BASE_1MS);
          driver_board_poweron_reason_ctrl(); 
          driver_board_poweroff_reason_ctrl();
          Key_Process();
          BoardPower_Poweroff_Ctrl();
      }
      usart_rcv_analysis();
    }//end of while(1)
}


