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
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x4000); //�����ж�������
    delay_init();//��ʱ������ʼ��
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);//����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�

    usart_init(115200);//���ڳ�ʼ��

    FlwCAN_Init(CAN_BAUDRATE_125K, CAN_Mode_Normal);//��ʼ��CAN
 
    TouchKey_Init(); //�������ز���ʱ����ʼ��
   
    FuelGauge_Init(); //�����Ƴ�ʼ��
    
    driver_board_poweron_reason_ctrl(); 
    
    SYSTIM_BASE_Init();//ϵͳʱ����ʼ��

    BoardPowerSupply_Init();   //��Դ���ع����ʼ��
    
    EEPROM_Init(0);

    app_update_init();    //�̼�������ʼ��
    
    beep_init();
    
    printf("build_time: %s-%s\r\n", __DATE__, __TIME__);
    printf("version: %d-%d-%d-%d\r\n", VM, HW, SW, REPE);
    printf("Init OK.\r\n");

    while(1)
    {
      if(SYSTIM_BASE_FlagGet(SYSTIM_BASE_1S))
      {
          SYSTIM_BASE_FlagClear(SYSTIM_BASE_1S); //�����ʱ��־
          i2c_Report_Battery_State_With_Uart();
      }
      
      if(SYSTIM_BASE_FlagGet(SYSTIM_BASE_1MS)) //1ms��ʱ��
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


