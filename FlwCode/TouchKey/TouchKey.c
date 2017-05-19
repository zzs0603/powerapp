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
    /* ʱ��ʹ�� */
    RCC_APB2PeriphClockCmd(TOUCH_KEY_GPIO_CLK, ENABLE);
    /* GPIO ���� */
    GPIO_Configuration(TOUCH_KEY_GPIO_PORT, TOUCH_KEY_GPIO_PIN, GPIO_Mode_IPD);   
}
    


void TouchKey_Init(void)
{
     //Touch_Pad_Led_Init();
     led_init();
     led_brightness_set(LED_HIGH); //����
     TOUCH_KEY_Init();
     memset(&key_str, 0, sizeof(key_str));
}


static uint8_t Get_Key_State(void)
{
       if(TOUCH_KEY == KEY_DOWN) //��������
       {
           key_str.timeflow++; //������ʱ
           
           if(key_str.timeflow >= HOLDDOWN_TIME && key_str.state != HOLDDOWN) //����
           {
               key_str.state = HOLDDOWN;
               key_str.timeflow = 0;
               return key_str.state; //����һ�γ����¼�
           }
           
           if(key_str.state == DOUBCLICK_TEMP)//˫���ڶ���
           {
               key_str.state = DOUBCLICK; //ȷ��Ϊһ��˫���¼�
               key_str.doubclk_timeflow = 0;
               key_str.timeflow = 0;
               return key_str.state;  //����һ��˫���¼�
           }
           
           return 0;
       }
       else //���򰴼���̧���
       {
           if(key_str.state == HOLDDOWN || key_str.state == DOUBCLICK || key_str.state == CLICK) //״̬���
           {
               key_str.timeflow = 0;
               key_str.doubclk_timeflow = 0;
               key_str.state = 0;        //���״̬
               return 0;
           }
           
           if(key_str.timeflow >= DEATHTIME && key_str.timeflow < DOUBCLICK_TIME && key_str.state != DOUBCLICK_TEMP)//˫���״�
           {
               key_str.state = DOUBCLICK_TEMP; //�����м�״̬
               key_str.timeflow = 0;
           }
           
           if(key_str.state == DOUBCLICK_TEMP)//����˫�����м�״̬ 
           {
               if(++key_str.doubclk_timeflow >= DOUBCLICK_TIME)//�����м�״̬�󣬵�����̧��ʱ�䳬ʱ ���㵥��
               {
                   key_str.state = CLICK;   //����
                   key_str.doubclk_timeflow = 0;
                   key_str.timeflow = 0;
                   return key_str.state;
               }
           }
           
           if(key_str.timeflow >= DEATHTIME && key_str.timeflow < HOLDDOWN_TIME) //���
           {
               key_str.state = CLICK; //�̰�
               key_str.timeflow = 0;
               return key_str.state; //����һ�ε����¼�
           }
           
           return 0;
       }
}

#define KEY_LED_HOLD_TIM   (80)
int16_t key_led_overflow = KEY_LED_HOLD_TIM;

static void key_led_process(uint8_t key)        //����������ʱLED����
{
    if(key) //����������������
    {
        key_led_overflow = KEY_LED_HOLD_TIM;
        led_brightness_set(LED_HIGH);
    }
    
    if( --key_led_overflow <= 0) //�ͷź��ӳ�1S ����
    {
        key_led_overflow  = 0;
        led_brightness_set(LED_LOW); //����
    }
}

//�����ػ�ʱ,������Ӧ�ػ�����ĳ�ʱ����
static void touchkey_poweroff_overflow_process(void)
{
     if(poweroff_overtime_flag) //�ػ������ѷ���
     {
         if(BoardPower_GetState() == 0)//˵������Ӧ�ػ����˳���ʱ�ж�״̬
         {
             poweroff_overtime_flag = 0;
             poweroff_overtime_cnt = 0;
         }
         else
         {
             if(++poweroff_overtime_cnt >= 3000)//��ʱ�ϱ��ػ���Ϣ
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
             if(++poweroff_force_timeout >= 20000) //20�볬ʱ
             {
                 if(BoardPower_GetState() != 0 && Get_BoardPower_Poweroff_flag() == 0)
                 {
                     poweroff_overtime_flag = 0;
                     poweroff_overtime_cnt = 0;
                     poweroff_force_timeout = 0;
                     set_CAN_Send_Flag(0);
                     BoardPower_SetState(ALL_BOARD_POWER_ID, 0);//ǿ�ƶϵ�
                 }
             }
         }
     }
}



//�ϵ����ֳ�ʱ����
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
       if(key_str.check_flag_en == 0)//�����ڲ���״̬�������ܽ�����������
       {
           if(CAN_Send_Flag_State() == 2)//�����ذ�����֮��
              CAN_Send_Message(CAN_ID_CLICK_EVENT, &buf, 1); //���͵����¼�
       }
       else if(key_str.check_flag_en == 1)//==================����״̬====================
       {
            CAN_Send_Message(CAN_ID_FUN_TEST + PCBTOUCH, &buf, 1); //�ɹ���ȡ����״̬
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
          
          if(BoardPower_GetState() && (CAN_Send_Flag_State() == 2))//�ػ� 
          {
             buf = Get_SOC();
             CAN_Send_Message(CAN_ID_POWEROFF_EVENT, &buf, 1);
             poweroff_overtime_flag = 1;  //�����ж����ذ���Ӧ�ػ��Ƿ�ʱ
          }
          else
          {
              if(BoardPower_GetState()==0 )//����
              {
                 if(Get_SOC() > 3 )  //��������
                 {
                    FlwCAN_Init(CAN_BAUDRATE_125K, CAN_Mode_Normal);
                    poweron_handshake_overtime_flag = 1; //�ϵ����ֳ�ʱ��־
                    poweron_handshake_overtime = 0;
                    set_driver_board_poweron_reason(4); //�����ϵ�
                    BoardPower_SetState(ALL_BOARD_POWER_ID, 1); //ֱ���ϵ�
                    
                    if(GetAdapterState() == ADAPTER_STATE_TRUE) //�����Դ������������Driver��϶��е磬���跢�������¼�
                    {
                       CAN_Send_Message(CAN_ID_POWERON_EVENT, &buf, 1);//�ϵ�
                    }
                 }
                 else//�������С��3
                 {
                     if(GetAdapterState() == ADAPTER_STATE_TRUE) //�ڳ������
                     {
                         BoardPower_SetState(ALL_BOARD_POWER_ID, 1); //ֱ���ϵ� //ADD 16.8.8
                         CAN_Send_Message(CAN_ID_POWERON_EVENT, &buf, 1);//�ϵ�
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
        if(CAN_Send_Flag_State() == 2)//�����ذ�����֮��
           CAN_Send_Message(CAN_ID_DOUBCLK_EVENT, &buf, 1); //���͵����¼�
    }
    
    key_led_process(TOUCH_KEY); //touch��
    
    poweron_handshake_overtime_process();
    
    touchkey_poweroff_overflow_process();//�����ػ�ʱ,������Ӧ�ػ�����ĳ�ʱ����
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





