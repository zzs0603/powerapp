#include "delay.h"
#include "gpio.h"
#include "can.h"
#include "usart.h"
#include "FuelGauge.h"
#include "BoardPowerSupply.h"

#include "drv_i2c.h"
#include "bq34z100.h"
#include "adc.h"

#define CHARGE_DEATH_AREA  50  //����������

#define BUFF_LEN 4


uint8_t tx[BUFF_LEN];
uint8_t rx[BUFF_LEN];


#define CURRENT_MAX_VAL
#define CURRENT_MIN_VAL

#define VOL_MAX_VAL
#define VOL_MIN_VAL

#define SOC_MAX_VAL    100
#define SOC_MIN_VAL    0

#define I2C_HARD  1
#define I2C_SOFT  2
#define I2C_TYPE  I2C_SOFT //ѡ��I2C����ģʽΪ�ܽ�ģ��


uint8_t  cur_charge_state = 0; //��ǰ���״̬ 
uint8_t  pre_charge_state = 0; //֮ǰ�ĳ��״̬

uint8_t noerr = 0;
signed short current = 0, voltage = 0, SOC = 0, pre_soc = 0;




static void ChargeState_Init(void)//���״̬�ܽų�ʼ��
{
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    //ʱ��ʹ��
    RCC_APB2PeriphClockCmd(CHARGE_STATE_GPIO_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    //GPIO����
    GPIO_Configuration(CHARGE_STATE_PORT, CHARGE_STATE_PIN  | POWERSWITCH_STATE_PIN, GPIO_Mode_IN_FLOATING); //��������
    GPIO_Configuration(ADAPTER_STATE_PORT, ADAPTER_STATE_PIN, GPIO_Mode_IN_FLOATING); //��������
    
    
    
    //PA6 �ⲿ�ж� 
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource6);
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line6;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; //�����ػ��½����ж�
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    //GPIOB.14 �ⲿ�ж��ź�
//    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);
//    
//    EXTI_InitStructure.EXTI_Line = EXTI_Line14;
//    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
//    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; //�����ػ��½����ж�
//    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
//    EXTI_Init(&EXTI_InitStructure);
//    
//    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);
       
       
    //GPIOB.1 �ⲿ�ж��ź�
//    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);
//    
//    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
//    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
//    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //�½����ж�
//    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
//    EXTI_Init(&EXTI_InitStructure);
//    
//    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);

      
}


//��������������״̬
uint8_t GetAdapterState(void)
{
      return  ADAPTER_STATE;
}

//���س��״̬ �������������������Ч
uint8_t GetChargeState(void)
{
       return CHARGE_STATE;
}

//���ص�Դ����״̬ �������������������Ч
uint8_t GetPowerSwitchState(void)
{
     return POWERSWITCH_STATE;
}



void FuelGauge_Init_hard(void)
{
    ChargeState_Init(); //���״̬�ܽų�ʼ��
    
    i2cInit(BATTERY_I2C);
    
    delay_ms(600);//�ӳ�һ��ʱ��
    
    //reset
    tx[0] = 0x41;
    tx[1] = 0x00;
    i2cWriteBuffer(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x00, 2, tx);
    delay_ms(1000);//�ӳ�һ��ʱ�䣬�ȴ�BQ34Z100�ȶ�
    
    //ʹ�ܵ�����IT�㷨
      tx[0] = 0x21;
      tx[1] = 0x00;
      i2cWriteBuffer(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x00, 2, tx);
      delay_ms(50);//�ӳ�һ��ʱ�䣬�ȴ�BQ34Z100�ȶ�
    
//������ֵ
    i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x02, 2, rx); //��ȡ����ֵ
    SOC = rx[0];
    
    if(GetAdapterState() == ADAPTER_STATE_TRUE) //���ϳ����
    {
        cur_charge_state = 1;
    }
    else //δ���ӳ����
    {
        cur_charge_state = 0; //δ���ӳ����
    }
}

void FuelGauge_Init_soft(void) //ʹ�ùܽ�ģ��I2C
{
    ChargeState_Init(); //���״̬�ܽų�ʼ��
    
    bq34z100_Init(); //��ʼ��
    bq34z100_run(&voltage, &current, &SOC); //��ȡ������ѹ����
    
    if(GetAdapterState() == ADAPTER_STATE_TRUE) //���ϳ����
    {
        cur_charge_state = 1;
    }
    else //δ���ӳ����
    {
        cur_charge_state = 0; //δ���ӳ����
    }
    
    adc_init(); //
}

void FuelGauge_Init(void)
{
#if (I2C_TYPE == I2C_SOFT)
  FuelGauge_Init_soft();
#elif (I2C_TYPE == I2C_HARD)
  FuelGauge_Init_hard();
#endif
}


//�����״̬�����ı�ʱ �ϱ�һ�γ��״̬
void Report_Charge_State_With_Can(void)
{
      if(GetAdapterState() == ADAPTER_STATE_TRUE) //���ϳ����
      {
           cur_charge_state = 1;
      }
      else //δ���ӳ����
      {
           cur_charge_state = 0; //δ���ӳ����
      }

       if(pre_charge_state != cur_charge_state) //������״̬�����ı����ϱ�һ�γ��״̬
       {
           tx[0] = cur_charge_state; //���״̬(����0��ʾδ���ӳ������1��ʾ���ڳ��  2��ʾ��������)
           tx[1] = pre_charge_state;
           CAN_Send_Message(CAN_ID_CHARGE_STATE, &tx[0], 2); //���ͳ��״̬
           
           /*printf("can send cur_charge_state %d  pre_charge_state %d \r\n", cur_charge_state, pre_charge_state);*/
           pre_charge_state = cur_charge_state;
       }
}

//PA6�ⲿ�ж� ���״̬�����ı�
void EXTI9_5_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line6) != RESET)
    {
        Report_Charge_State_With_Can(); //״̬�����ı�
        EXTI_ClearITPendingBit(EXTI_Line6);
    }
}

//PB14�ⲿ�ж� ���״̬�����ı�
//void EXTI15_10_IRQHandler(void)
//{
//    if(EXTI_GetITStatus(EXTI_Line14) != RESET)
//    {
//        Report_Charge_State_With_Can(); //״̬�����ı�
//        EXTI_ClearITPendingBit(EXTI_Line14);
//    }
//}

//PB1�ⲿ�ж� 
//void EXTI1_IRQHandler(void)
//{
//    if(EXTI_GetITStatus(EXTI_Line1) != RESET)
//    {
//        Report_Charge_State_With_Can(); //״̬�����ı�
//        EXTI_ClearITPendingBit(EXTI_Line1);
//    }
//}

//ͨ���������������ж�
//static void get_chargestate_through_current(void)
//{
//    static uint8_t cur_chargestate = 0;
//    static uint8_t pre_chargestate = 0;
//    
//    if(current > 0) //ֻ�ܼӿ�LED�Ƶ�ָʾ
//    {
//        cur_chargestate = 1; //�ڳ��
//    }
//    else
//    {
//        cur_chargestate = 0;
//    }
//    
//    if(pre_chargestate != cur_chargestate)
//    {
//        tx[0] = SOC;   //��ص���SOC
//        tx[1] = cur_chargestate;  //���״̬(����0��ʾδ���ӳ������1��ʾ���ڳ��  2��ʾ��������)
//        CAN_Send_Message(CAN_ID_BATTERY_STATE, &tx[0], 2);
//        pre_chargestate = cur_chargestate;
//        CLI();
//        cur_charge_state = cur_chargestate;
//        SEI();
//    }
//}



void Report_Battery_State_With_Can(void)
{
      //��CAN FIFO����ӷ�������
      static uint16_t report_time = 0;

      if(report_time++ >= 8)
      {
          report_time = 0;

          tx[0] = SOC;   //��ص���SOC
          tx[1] = cur_charge_state;  //���״̬(����0��ʾδ���ӳ������1��ʾ���ڳ��  2��ʾ��������)
          CAN_Send_Message(CAN_ID_BATTERY_STATE, &tx[0], 2); //���͵��������״̬ ����0��ʾδ���ӳ������1��ʾ���ڳ��  2��ʾ��������
          /*printf("can send SOC %d cur_charge_state %d\r\n", SOC, cur_charge_state);*/
      }
}

//�����ذ巢����һ��0x600�����źź��������ص���ֵ
void Report_Battery_State_Init(void)
{
    tx[0] = SOC;   //��ص���SOC
    tx[1] = cur_charge_state;  //���״̬(����0��ʾδ���ӳ������1��ʾ���ڳ��  2��ʾ��������)
    CAN_Send_Message(CAN_ID_BATTERY_STATE, &tx[0], 2); //���͵��������״̬ ����0��ʾδ���ӳ������1��ʾ���ڳ��  2��ʾ��������
}

//��õ�ǰ����ֵ
uint8_t Get_SOC(void)
{
    return SOC;
}






void i2c_hrad_Report_Battery_State_With_Uart(void)
{
      uint16_t errcount = i2cGetErrorCounter(BATTERY_I2C); //
      if(errcount > 0)
      {
        printf("i2c err count: %d\n", errcount);
        i2c_reset_errcount(BATTERY_I2C); 
        FuelGauge_Init();
      }
    
      noerr = i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x10, 2, rx); //��ȡ����ֵ
      if(noerr)
      {
        current = (signed short)(((rx[1] << 8) & 0xff00) + (rx[0] & 0x00ff));
        //printf("Current():%dmA. \r\n", current);
      }
    
      noerr = i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x08, 2, rx); //��ȡ��ѹֵ
      if(noerr)
      {
        voltage = (signed short)(((rx[1] << 8) & 0xff00) + (rx[0] & 0x00ff));
        /*printf("voltage():%dmV. \r\n", voltage);*/
        
        if(voltage <= 8000) //��������Ƶ�ѹֵС�ڵ���8V�� ����������
        {
            //reset
           FuelGauge_Init();
        }
      }
      
      
      noerr = i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x02, 2, rx); //��ȡ����ֵ
      if(noerr)
      {
         SOC = rx[0];
         if(SOC > SOC_MAX_VAL || SOC < SOC_MIN_VAL) //��Χ�ж�
         {
             FuelGauge_Init();
         }
         
         //�������С��3ʱ���͵͵����ػ�
         if(SOC <= 3) //�������Ϊ0���͹ػ�ָ��
         {
             // add a check poweroff flag condition by sch --20160421
             //������ذ��е�,����û�д��ڹػ��� �������ذ����ֳɹ�
             if(BoardPower_GetState() && (Get_BoardPower_Poweroff_flag() == 0) \
                                      && CAN_Send_Flag_State()
                                      && GetAdapterState() == ADAPTER_STATE_FASLE) //ADD 16.8.8
             {
                 static uint16_t soc_poweroff_cnt = 0;
                 if(++soc_poweroff_cnt >= 3)//3��
                 {
                     uint8_t buf = SOC;
                     soc_poweroff_cnt = 0;
                     CAN_Send_Message(CAN_ID_POWEROFF_EVENT, &buf, 1); //���͹ػ�ָ��
                 }
             }
         }
         //printf("StateOfCharge(): %dperc. \r\n", SOC);
         printf("%d\r\n", SOC);
      }

      if(current == 0 && voltage == 0 && SOC == 0) //����У��
      {
          FuelGauge_Init();
      }
      
       if(GetAdapterState() == ADAPTER_STATE_TRUE)//�������������
       {
           if(GetPowerSwitchState() == POWERSWITCH_STATE_OFF || SOC <= 1)//�����ܿ���Ϊ�ر�״̬
           {
              Report_Battery_State_Init();
           }
       }
       
       Report_Battery_State_With_Can(); //ÿ20���ϴ�һ��״̬
}



typedef struct
{
  u32  soc_per;
  float vol;
}BAT_TABLE;

#define BATT_H_VOL (12500/4.0)
#define BATT_L_VOL (9800/4.0)
//���ݵ�ص�ǰ��ѹ�������ֵ
#define COEFFICIENT 0.75
#define VOL_DELTA 10
BAT_TABLE bat_table[] =
{
    {100, 4200*COEFFICIENT},
    {90, 4080*COEFFICIENT},
    {80, 4000*COEFFICIENT},
    {70, 3930*COEFFICIENT},
    {60, 3870*COEFFICIENT},
    {50, 3820*COEFFICIENT},
    {40, 3790*COEFFICIENT},
    {30, 3770*COEFFICIENT},
    {20, 3730*COEFFICIENT},
    {15, 3700*COEFFICIENT},
    {10, 3680*COEFFICIENT},
    {0,  3300*COEFFICIENT},
};




signed short  batt_soc_estimation()
{
    unsigned int ad_vbat1 = 0,i = 0;
    float ad_vbat2 = 0, h_vol = 0, l_vol = 0, h_per = 0, l_per = 0;
    unsigned short batt_soc = 0;
    
    ad_vbat1 = Get_Adc(1); //��ȡ����ֵ
    
    ad_vbat2 = ((float)ad_vbat1 * 3300) / 4096;
    
    if(ad_vbat2 > 4200*COEFFICIENT)
        ad_vbat2 = 4200*COEFFICIENT - VOL_DELTA;
    else if(ad_vbat2 < 3300*COEFFICIENT)
        ad_vbat2 = 3300*COEFFICIENT + VOL_DELTA;
    
    for(i = 0; i < 11; i++)
    {
        if( (ad_vbat2 < bat_table[i].vol) && (ad_vbat2 > (bat_table[i+1].vol - VOL_DELTA)) )
        {
            h_vol = bat_table[i].vol;
            l_vol = bat_table[i+1].vol;
            h_per = bat_table[i].soc_per;
            l_per = bat_table[i+1].soc_per; 
            break;
        }
    }
    
    if(i > 11)
    {
        h_vol = BATT_H_VOL;
        l_vol = BATT_L_VOL;
        h_per = 100;
        l_per = 0; 
    }
    
    batt_soc = (unsigned short )((ad_vbat2 - l_vol) /(h_vol - l_vol) * (h_per - l_per) + l_per) ;
    return batt_soc;
}


void i2c_soft_Report_Battery_State_With_Uart(void) //ʹ�ùܽ�ģ��I2C
{
    bq34z100_run(&voltage, &current, &SOC); //��ȡ������ѹ����
    
    
    //SOC = batt_soc_estimation();
    //printf("S-%d\r\n", SOC);
    //�˴��ж϶�ȡֵ��Χ ���������������������ֵ
    if(voltage < 8000 || voltage > 13000) //�����ص�ѹֵ
    {
        SOC = batt_soc_estimation();
    }
    
    printf("%d\r\n", SOC);
    //�������С��3ʱ���͵͵����ػ�
     if(SOC <= 3) //�������Ϊ0���͹ػ�ָ��
     {
         // add a check poweroff flag condition by sch --20160421
         //������ذ��е�,����û�д��ڹػ��� �������ذ����ֳɹ�
         if(BoardPower_GetState() && (Get_BoardPower_Poweroff_flag() == 0) \
                                  && CAN_Send_Flag_State()
                                  && GetAdapterState() == ADAPTER_STATE_FASLE) //ADD 16.8.8
         {
             static uint16_t soc_poweroff_cnt = 0;
             if(++soc_poweroff_cnt >= 3)//3��
             {
                 uint8_t buf = SOC;
                 soc_poweroff_cnt = 0;
                 CAN_Send_Message(CAN_ID_POWEROFF_EVENT, &buf, 1); //���͹ػ�ָ��
             }
         }
     }
     
     
     if(GetAdapterState() == ADAPTER_STATE_TRUE)//�������������
     {
         if(GetPowerSwitchState() == POWERSWITCH_STATE_OFF || SOC <= 1)//�����ܿ���Ϊ�ر�״̬
         {
            Report_Battery_State_Init();
         }
     }
     
     Report_Battery_State_With_Can(); //ÿ20���ϴ�һ��״̬
     
     Report_Charge_State_With_Can(); //���״̬
}






void i2c_Report_Battery_State_With_Uart(void)
{
#if (I2C_TYPE == I2C_SOFT)
  i2c_soft_Report_Battery_State_With_Uart(); //GPIOģ��
#elif (I2C_TYPE == I2C_HARD)
  i2c_hrad_Report_Battery_State_With_Uart(); //Ӳ��I2C
#endif
}



void driver_board_poweron_reason_ctrl(void)
{
    if(BoardPower_GetState() == 0) //��δ�ϵ�������
    {
        if(GetPowerSwitchState() == POWERSWITCH_STATE_OFF)//����ܿ����ǹرյĵ���POWER���ϵ���,��ض�����������
        {
            if(GetAdapterState() == ADAPTER_STATE_TRUE)
            {
                set_driver_board_poweron_reason(1); //�ܿ��عرճ��
            }
            else
            {
                //����״̬
            }
        }
        else//���ش�
        {
            if(GetAdapterState() == ADAPTER_STATE_TRUE) //�ڳ�����ϳ�� ��֤�������ڳ������ʱ���������ϵ��
            {
                static uint16_t poweron_cnt = 0;
                
                set_driver_board_poweron_reason(3);  //�ܿ��ش򿪳�絼���������ϵ�
                set_driver_board_reason_3_2_flag(1); //ȷ�������˲��ڳ������ʱ������ϵ�
                if(++poweron_cnt >= 1000)
                {
                  poweron_cnt = 0;
                  BoardPower_SetState(DRIVER_BOARD_POWER_ID, 1); //�ϵ�
                }
            }
            else //
            {
                set_driver_board_poweron_reason(2); //�ܿ��ش򿪶��������������ϵ�
            }
        }
    }
}

//�ӳ�������뿪ʱ�������Դ״̬Ϊ�� ���������ϵ�
void driver_board_poweroff_reason_ctrl(void)
{
    if(GetAdapterState() == ADAPTER_STATE_FASLE)//�����״̬
    {
        if(BoardPower_GetState() == 0 && (get_driver_board_reason_3_2_flag() == 1)) //
        {
            set_driver_board_reason_3_2_flag(0); //����
            set_CAN_Send_Flag(0);
            BoardPower_SetState(DRIVER_BOARD_POWER_ID, 0); //�ϵ�
        }
    }
}




void I2C2_EV_IRQHandler()
{
    I2C_EV_Handler();
}

void I2C2_ER_IRQHandler()
{
    I2C_ER_Handler();
}



uint8_t FuelGauge_Check(void)
{
    uint8_t re = 1;
    uint8_t chargestate;
    uint16_t i = 0;
    
    /*if(GetAdapterState() != ADAPTER_STATE_TRUE) //δ���ϳ����
    {
         re = 0;
    }
    
    delay_ms(2000);
    
    for(i = 0; i < 2000; i++)
    {
        chargestate = GetChargeState();
        if(chargestate == 0)
        {
            re = 0;
            break;
        }
        delay_ms(1);
    }*/
   
    //noerr = i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x08, 2, rx); //��ȡ��ѹֵ
    //voltage = (signed short)(((rx[1] << 8) & 0xff00) + (rx[0] & 0x00ff));  //��ȡ��ص�ѹֵ
    if(voltage < 9000 || voltage > 12200)
    {
        re = 0;
    }
    
    //noerr = i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x10, 2, rx); //��ȡ����ֵ
    //current = (signed short)(((rx[1] << 8) & 0xff00) + (rx[0] & 0x00ff));
    if(current < 1600 || current > 2500 ) //������
    {
        re = 0;
    }
    
    return re;
}



