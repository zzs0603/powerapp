#include "gpio.h"
#include "timer.h"
#include "stdio.h"
#include "string.h"

#include "TouchKey.h"
#include "BoardPowerSupply.h"
#include "can.h"
#include "Led.h"
#include "FuelGauge.h"
#include "delay.h"
#include "beep.h"

KEY_STR key_str;
static uint8_t   poweroff_overtime_flag = 0;
static uint16_t  poweroff_overtime_cnt = 0;
static uint16_t  poweroff_force_timeout = 0;
//static uint8_t  key_poweroff_flag = 0;
//static uint16_t key_poweroff_cnt = 0;
static uint8_t   poweron_handshake_overtime_flag = 0;
static uint16_t  poweron_handshake_overtime = 0;

uint8_t low_power_hold_key_flag = 0;

static void TOUCH_KEY_Init(void)
{
    /* 时钟使能 */
    RCC_APB2PeriphClockCmd(TOUCH_KEY_GPIO_CLK, ENABLE);
    /* GPIO 配置 */
    GPIO_Configuration(TOUCH_KEY_GPIO_PORT, TOUCH_KEY_GPIO_PIN, GPIO_Mode_IPD);   
}
    


void TouchKey_Init(void)
{
     //Touch_Pad_Led_Init();
     led_init();
     led_brightness_set(LED_HIGH); //低亮
     TOUCH_KEY_Init();
     memset(&key_str, 0, sizeof(key_str));
}


static uint8_t Get_Key_State(void)
{
       if(TOUCH_KEY == KEY_DOWN) //按键按下
       {
           key_str.timeflow++; //按键计时
           
           if(key_str.timeflow >= HOLDDOWN_TIME && key_str.state != HOLDDOWN) //长按
           {
               key_str.state = HOLDDOWN;
               key_str.timeflow = 0;
               return key_str.state; //返回一次长按事件
           }
           
           if(key_str.state == DOUBCLICK_TEMP)//双击第二次
           {
               key_str.state = DOUBCLICK; //确定为一次双击事件
               key_str.doubclk_timeflow = 0;
               key_str.timeflow = 0;
               return key_str.state;  //返回一次双击事件
           }
           
           return 0;
       }
       else //否则按键是抬起的
       {
           if(key_str.state == HOLDDOWN || key_str.state == DOUBCLICK || key_str.state == CLICK) //状态解除
           {
               key_str.timeflow = 0;
               key_str.doubclk_timeflow = 0;
               key_str.state = 0;        //清除状态
               return 0;
           }
           
           if(key_str.timeflow >= DEATHTIME && key_str.timeflow < DOUBCLICK_TIME && key_str.state != DOUBCLICK_TEMP)//双击首次
           {
               key_str.state = DOUBCLICK_TEMP; //进入中间状态
               key_str.timeflow = 0;
           }
           
           if(key_str.state == DOUBCLICK_TEMP)//进入双击的中间状态 
           {
               if(++key_str.doubclk_timeflow >= DOUBCLICK_TIME)//进入中间状态后，但按键抬起时间超时 则算单击
               {
                   key_str.state = CLICK;   //单击
                   key_str.doubclk_timeflow = 0;
                   key_str.timeflow = 0;
                   return key_str.state;
               }
           }
           
           if(key_str.timeflow >= DEATHTIME && key_str.timeflow < HOLDDOWN_TIME) //点击
           {
               key_str.state = CLICK; //短按
               key_str.timeflow = 0;
               return key_str.state; //返回一次单击事件
           }
           
           return 0;
       }
}

#define KEY_LED_HOLD_TIM   (80)
int16_t key_led_overflow = KEY_LED_HOLD_TIM;

static void key_led_process(uint8_t key)        //当按键按下时LED高亮
{
    if(key) //如果按键按下则高亮
    {
        key_led_overflow = KEY_LED_HOLD_TIM;
        led_brightness_set(LED_HIGH);
    }
    
    if( --key_led_overflow <= 0) //释放后延迟1S 低亮
    {
        key_led_overflow  = 0;
        led_brightness_set(LED_LOW); //低亮
    }
}

//长按关机时,主机响应关机命令的超时处理
static void touchkey_poweroff_overflow_process(void)
{
     if(poweroff_overtime_flag) //关机命令已发送
     {
         if(BoardPower_GetState() == 0)//说明已响应关机，退出超时判断状态
         {
             poweroff_overtime_flag = 0;
             poweroff_overtime_cnt = 0;
         }
         else
         {
             if(++poweroff_overtime_cnt >= 3000)//超时上报关机信息
             {
                 uint8_t buf = Get_SOC();
                 poweroff_overtime_cnt = 0;
                 CLI();
                 if(BoardPower_GetState() != 0 && Get_BoardPower_Poweroff_flag() == 0)
                 {
                   CAN_Send_Message(CAN_ID_POWEROFF_EVENT, &buf, 1);
                 }
                 SEI();
             }
             if(++poweroff_force_timeout >= 20000) //20秒超时
             {
                 if(BoardPower_GetState() != 0 && Get_BoardPower_Poweroff_flag() == 0)
                 {
                     poweroff_overtime_flag = 0;
                     poweroff_overtime_cnt = 0;
                     poweroff_force_timeout = 0;
                     set_CAN_Send_Flag(0);
                     BoardPower_SetState(ALL_BOARD_POWER_ID, 0);//强制断电
                 }
             }
         }
     }
}



//上电握手超时处理
static void poweron_handshake_overtime_process(void)
{
    if(poweron_handshake_overtime_flag)
    {
        if(++poweron_handshake_overtime >= 60000)
        {
           poweron_handshake_overtime = 0;
           poweron_handshake_overtime_flag = 0;
           set_CAN_Send_Flag(2);
        }
    }
}




void Key_Process(void)
{
    uint8_t key_val = 0;
    uint8_t buf = 0;
    key_val = Get_Key_State();
    
    if(key_val == CLICK)
    {
       if(key_str.check_flag_en == 0)//不处于测试状态单击可能进行其它操作
       {
           if(CAN_Send_Flag_State() == 2)//与主控板握手之后
              CAN_Send_Message(CAN_ID_CLICK_EVENT, &buf, 1); //发送单击事件
       }
       else if(key_str.check_flag_en == 1)//==================测试状态====================
       {
            CAN_Send_Message(CAN_ID_FUN_TEST + PCBTOUCH, &buf, 1); //成功获取触摸状态
       }//=================================================
    }
    else if(key_val == HOLDDOWN)
    {
          unsigned int beep_count = 0;
          //TOUCH_LED_EN = ~TOUCH_LED_EN; //
          BEEP_ON;
          delay_ms(500);
          BEEP_OFF;
        
          if(GetPowerSwitchState() == POWERSWITCH_STATE_OFF) 
             return;
          
          if(BoardPower_GetState() && (CAN_Send_Flag_State() == 2))//关机 
          {
             buf = Get_SOC();
             CAN_Send_Message(CAN_ID_POWEROFF_EVENT, &buf, 1);
             poweroff_overtime_flag = 1;  //用于判断主控板响应关机是否超时
          }
          else
          {
              if(BoardPower_GetState()==0 )//开机
              {
                 if(Get_SOC() > 3 )  //正常开机
                 {
                    FlwCAN_Init(CAN_BAUDRATE_125K, CAN_Mode_Normal);
                    poweron_handshake_overtime_flag = 1; //上电握手超时标志
                    poweron_handshake_overtime = 0;
                    set_driver_board_poweron_reason(4); //正常上电
                    BoardPower_SetState(ALL_BOARD_POWER_ID, 1); //直接上电
                    
                    if(GetAdapterState() == ADAPTER_STATE_TRUE) //如果电源适配器接入则Driver板肯定有电，则需发送如下事件
                    {
                       CAN_Send_Message(CAN_ID_POWERON_EVENT, &buf, 1);//上电
                    }
                 }
                 else//否则电量小于3
                 {
                     if(GetAdapterState() == ADAPTER_STATE_TRUE) //在充电做上
                     {
                         BoardPower_SetState(ALL_BOARD_POWER_ID, 1); //直接上电 //ADD 16.8.8
                         CAN_Send_Message(CAN_ID_POWERON_EVENT, &buf, 1);//上电
                     }
                     else
                     {
                         low_power_hold_key_flag = 1;
                         BoardPower_SetState(DRIVER_BOARD_POWER_ID, 1);
                     }
                 }
             }
          }
    }
    else if(key_val == DOUBCLICK)
    {
        if(CAN_Send_Flag_State() == 2)//与主控板握手之后
           CAN_Send_Message(CAN_ID_DOUBCLK_EVENT, &buf, 1); //发送单击事件
    }
    
    key_led_process(TOUCH_KEY); //touch灯
    
    poweron_handshake_overtime_process();
    
    touchkey_poweroff_overflow_process();//长按关机时,主机响应关机命令的超时处理
}




void set_low_power_hold_key_flag(uint8_t flag)
{
    low_power_hold_key_flag = flag;
}

uint8_t get_low_power_hold_key_flag(void)
{
    return low_power_hold_key_flag;
}



//
void TouchKey_Check_Flag(uint8_t val)
{
    key_str.check_flag_en = val;
}





