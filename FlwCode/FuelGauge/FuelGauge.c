#include "delay.h"
#include "gpio.h"
#include "can.h"
#include "usart.h"
#include "FuelGauge.h"
#include "BoardPowerSupply.h"

#include "drv_i2c.h"
#include "bq34z100.h"
#include "adc.h"

#define CHARGE_DEATH_AREA  50  //充电电流死区

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
#define I2C_TYPE  I2C_SOFT //选择I2C工作模式为管脚模拟


uint8_t  cur_charge_state = 0; //当前充电状态 
uint8_t  pre_charge_state = 0; //之前的充电状态

uint8_t noerr = 0;
signed short current = 0, voltage = 0, SOC = 0, pre_soc = 0;




static void ChargeState_Init(void)//充电状态管脚初始化
{
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    //时钟使能
    RCC_APB2PeriphClockCmd(CHARGE_STATE_GPIO_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    //GPIO配置
    GPIO_Configuration(CHARGE_STATE_PORT, CHARGE_STATE_PIN  | POWERSWITCH_STATE_PIN, GPIO_Mode_IN_FLOATING); //浮空输入
    GPIO_Configuration(ADAPTER_STATE_PORT, ADAPTER_STATE_PIN, GPIO_Mode_IN_FLOATING); //浮空输入
    
    
    
    //PA6 外部中断 
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource6);
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line6;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; //上升沿或下降沿中断
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    //GPIOB.14 外部中断信号
//    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);
//    
//    EXTI_InitStructure.EXTI_Line = EXTI_Line14;
//    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
//    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; //上升沿或下降沿中断
//    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
//    EXTI_Init(&EXTI_InitStructure);
//    
//    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);
       
       
    //GPIOB.1 外部中断信号
//    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);
//    
//    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
//    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
//    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //下降沿中断
//    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
//    EXTI_Init(&EXTI_InitStructure);
//    
//    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);

      
}


//返回适配器接入状态
uint8_t GetAdapterState(void)
{
      return  ADAPTER_STATE;
}

//返回充电状态 仅适配器接入情况下有效
uint8_t GetChargeState(void)
{
       return CHARGE_STATE;
}

//返回电源开关状态 仅适配器接入情况下有效
uint8_t GetPowerSwitchState(void)
{
     return POWERSWITCH_STATE;
}



void FuelGauge_Init_hard(void)
{
    ChargeState_Init(); //充电状态管脚初始化
    
    i2cInit(BATTERY_I2C);
    
    delay_ms(600);//延迟一段时间
    
    //reset
    tx[0] = 0x41;
    tx[1] = 0x00;
    i2cWriteBuffer(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x00, 2, tx);
    delay_ms(1000);//延迟一段时间，等待BQ34Z100稳定
    
    //使能电量计IT算法
      tx[0] = 0x21;
      tx[1] = 0x00;
      i2cWriteBuffer(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x00, 2, tx);
      delay_ms(50);//延迟一段时间，等待BQ34Z100稳定
    
//读出初值
    i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x02, 2, rx); //读取电量值
    SOC = rx[0];
    
    if(GetAdapterState() == ADAPTER_STATE_TRUE) //接上充电器
    {
        cur_charge_state = 1;
    }
    else //未连接充电器
    {
        cur_charge_state = 0; //未连接充电器
    }
}

void FuelGauge_Init_soft(void) //使用管脚模拟I2C
{
    ChargeState_Init(); //充电状态管脚初始化
    
    bq34z100_Init(); //初始化
    bq34z100_run(&voltage, &current, &SOC); //获取电流电压电量
    
    if(GetAdapterState() == ADAPTER_STATE_TRUE) //接上充电器
    {
        cur_charge_state = 1;
    }
    else //未连接充电器
    {
        cur_charge_state = 0; //未连接充电器
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


//当充电状态发生改变时 上报一次充电状态
void Report_Charge_State_With_Can(void)
{
      if(GetAdapterState() == ADAPTER_STATE_TRUE) //接上充电器
      {
           cur_charge_state = 1;
      }
      else //未连接充电器
      {
           cur_charge_state = 0; //未连接充电器
      }

       if(pre_charge_state != cur_charge_state) //如果充电状态发生改变则上报一次充电状态
       {
           tx[0] = cur_charge_state; //充电状态(其中0表示未连接充电器，1表示正在充电  2表示电量充满)
           tx[1] = pre_charge_state;
           CAN_Send_Message(CAN_ID_CHARGE_STATE, &tx[0], 2); //发送充电状态
           
           /*printf("can send cur_charge_state %d  pre_charge_state %d \r\n", cur_charge_state, pre_charge_state);*/
           pre_charge_state = cur_charge_state;
       }
}

//PA6外部中断 充电状态发生改变
void EXTI9_5_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line6) != RESET)
    {
        Report_Charge_State_With_Can(); //状态发生改变
        EXTI_ClearITPendingBit(EXTI_Line6);
    }
}

//PB14外部中断 充电状态发生改变
//void EXTI15_10_IRQHandler(void)
//{
//    if(EXTI_GetITStatus(EXTI_Line14) != RESET)
//    {
//        Report_Charge_State_With_Can(); //状态发生改变
//        EXTI_ClearITPendingBit(EXTI_Line14);
//    }
//}

//PB1外部中断 
//void EXTI1_IRQHandler(void)
//{
//    if(EXTI_GetITStatus(EXTI_Line1) != RESET)
//    {
//        Report_Charge_State_With_Can(); //状态发生改变
//        EXTI_ClearITPendingBit(EXTI_Line1);
//    }
//}

//通过电流的正负来判断
//static void get_chargestate_through_current(void)
//{
//    static uint8_t cur_chargestate = 0;
//    static uint8_t pre_chargestate = 0;
//    
//    if(current > 0) //只能加快LED灯的指示
//    {
//        cur_chargestate = 1; //在充电
//    }
//    else
//    {
//        cur_chargestate = 0;
//    }
//    
//    if(pre_chargestate != cur_chargestate)
//    {
//        tx[0] = SOC;   //电池电量SOC
//        tx[1] = cur_chargestate;  //充电状态(其中0表示未连接充电器，1表示正在充电  2表示电量充满)
//        CAN_Send_Message(CAN_ID_BATTERY_STATE, &tx[0], 2);
//        pre_chargestate = cur_chargestate;
//        CLI();
//        cur_charge_state = cur_chargestate;
//        SEI();
//    }
//}



void Report_Battery_State_With_Can(void)
{
      //向CAN FIFO中添加发送数据
      static uint16_t report_time = 0;

      if(report_time++ >= 8)
      {
          report_time = 0;

          tx[0] = SOC;   //电池电量SOC
          tx[1] = cur_charge_state;  //充电状态(其中0表示未连接充电器，1表示正在充电  2表示电量充满)
          CAN_Send_Message(CAN_ID_BATTERY_STATE, &tx[0], 2); //发送电量及充电状态 其中0表示未连接充电器，1表示正在充电  2表示电量充满
          /*printf("can send SOC %d cur_charge_state %d\r\n", SOC, cur_charge_state);*/
      }
}

//在主控板发送了一次0x600握手信号后立即返回电量值
void Report_Battery_State_Init(void)
{
    tx[0] = SOC;   //电池电量SOC
    tx[1] = cur_charge_state;  //充电状态(其中0表示未连接充电器，1表示正在充电  2表示电量充满)
    CAN_Send_Message(CAN_ID_BATTERY_STATE, &tx[0], 2); //发送电量及充电状态 其中0表示未连接充电器，1表示正在充电  2表示电量充满
}

//获得当前电量值
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
    
      noerr = i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x10, 2, rx); //读取电流值
      if(noerr)
      {
        current = (signed short)(((rx[1] << 8) & 0xff00) + (rx[0] & 0x00ff));
        //printf("Current():%dmA. \r\n", current);
      }
    
      noerr = i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x08, 2, rx); //读取电压值
      if(noerr)
      {
        voltage = (signed short)(((rx[1] << 8) & 0xff00) + (rx[0] & 0x00ff));
        /*printf("voltage():%dmV. \r\n", voltage);*/
        
        if(voltage <= 8000) //如果电量计电压值小于等于8V则 重启电量计
        {
            //reset
           FuelGauge_Init();
        }
      }
      
      
      noerr = i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x02, 2, rx); //读取电量值
      if(noerr)
      {
         SOC = rx[0];
         if(SOC > SOC_MAX_VAL || SOC < SOC_MIN_VAL) //范围判断
         {
             FuelGauge_Init();
         }
         
         //如果电量小于3时发送低电量关机
         if(SOC <= 3) //如果电量为0则发送关机指令
         {
             // add a check poweroff flag condition by sch --20160421
             //如果主控板有电,并且没有处于关机中 且与主控板握手成功
             if(BoardPower_GetState() && (Get_BoardPower_Poweroff_flag() == 0) \
                                      && CAN_Send_Flag_State()
                                      && GetAdapterState() == ADAPTER_STATE_FASLE) //ADD 16.8.8
             {
                 static uint16_t soc_poweroff_cnt = 0;
                 if(++soc_poweroff_cnt >= 3)//3秒
                 {
                     uint8_t buf = SOC;
                     soc_poweroff_cnt = 0;
                     CAN_Send_Message(CAN_ID_POWEROFF_EVENT, &buf, 1); //发送关机指令
                 }
             }
         }
         //printf("StateOfCharge(): %dperc. \r\n", SOC);
         printf("%d\r\n", SOC);
      }

      if(current == 0 && voltage == 0 && SOC == 0) //数据校验
      {
          FuelGauge_Init();
      }
      
       if(GetAdapterState() == ADAPTER_STATE_TRUE)//如果接上适配器
       {
           if(GetPowerSwitchState() == POWERSWITCH_STATE_OFF || SOC <= 1)//并且总开关为关闭状态
           {
              Report_Battery_State_Init();
           }
       }
       
       Report_Battery_State_With_Can(); //每20秒上传一次状态
}



typedef struct
{
  u32  soc_per;
  float vol;
}BAT_TABLE;

#define BATT_H_VOL (12500/4.0)
#define BATT_L_VOL (9800/4.0)
//根据电池当前电压估算电量值
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
    
    ad_vbat1 = Get_Adc(1); //获取采样值
    
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


void i2c_soft_Report_Battery_State_With_Uart(void) //使用管脚模拟I2C
{
    bq34z100_run(&voltage, &current, &SOC); //获取电流电压电量
    
    
    //SOC = batt_soc_estimation();
    //printf("S-%d\r\n", SOC);
    //此处判断读取值范围 如果错误进入错误处理估算电量值
    if(voltage < 8000 || voltage > 13000) //如果电池电压值
    {
        SOC = batt_soc_estimation();
    }
    
    printf("%d\r\n", SOC);
    //如果电量小于3时发送低电量关机
     if(SOC <= 3) //如果电量为0则发送关机指令
     {
         // add a check poweroff flag condition by sch --20160421
         //如果主控板有电,并且没有处于关机中 且与主控板握手成功
         if(BoardPower_GetState() && (Get_BoardPower_Poweroff_flag() == 0) \
                                  && CAN_Send_Flag_State()
                                  && GetAdapterState() == ADAPTER_STATE_FASLE) //ADD 16.8.8
         {
             static uint16_t soc_poweroff_cnt = 0;
             if(++soc_poweroff_cnt >= 3)//3秒
             {
                 uint8_t buf = SOC;
                 soc_poweroff_cnt = 0;
                 CAN_Send_Message(CAN_ID_POWEROFF_EVENT, &buf, 1); //发送关机指令
             }
         }
     }
     
     
     if(GetAdapterState() == ADAPTER_STATE_TRUE)//如果接上适配器
     {
         if(GetPowerSwitchState() == POWERSWITCH_STATE_OFF || SOC <= 1)//并且总开关为关闭状态
         {
            Report_Battery_State_Init();
         }
     }
     
     Report_Battery_State_With_Can(); //每20秒上传一次状态
     
     Report_Charge_State_With_Can(); //充电状态
}






void i2c_Report_Battery_State_With_Uart(void)
{
#if (I2C_TYPE == I2C_SOFT)
  i2c_soft_Report_Battery_State_With_Uart(); //GPIO模拟
#elif (I2C_TYPE == I2C_HARD)
  i2c_hrad_Report_Battery_State_With_Uart(); //硬件I2C
#endif
}



void driver_board_poweron_reason_ctrl(void)
{
    if(BoardPower_GetState() == 0) //在未上电的情况下
    {
        if(GetPowerSwitchState() == POWERSWITCH_STATE_OFF)//如果总开关是关闭的但是POWER板上电了,则必定接入适配器
        {
            if(GetAdapterState() == ADAPTER_STATE_TRUE)
            {
                set_driver_board_poweron_reason(1); //总开关关闭充电
            }
            else
            {
                //错误状态
            }
        }
        else//开关打开
        {
            if(GetAdapterState() == ADAPTER_STATE_TRUE) //在充电座上充电 保证机器人在充电座上时驱动板是上电的
            {
                static uint16_t poweron_cnt = 0;
                
                set_driver_board_poweron_reason(3);  //总开关打开充电导致驱动板上电
                set_driver_board_reason_3_2_flag(1); //确保机器人不在充电座上时驱动板断电
                if(++poweron_cnt >= 1000)
                {
                  poweron_cnt = 0;
                  BoardPower_SetState(DRIVER_BOARD_POWER_ID, 1); //上电
                }
            }
            else //
            {
                set_driver_board_poweron_reason(2); //总开关打开动作导致驱动板上电
            }
        }
    }
}

//从充电座上离开时，如果电源状态为关 则给驱动板断电
void driver_board_poweroff_reason_ctrl(void)
{
    if(GetAdapterState() == ADAPTER_STATE_FASLE)//不充电状态
    {
        if(BoardPower_GetState() == 0 && (get_driver_board_reason_3_2_flag() == 1)) //
        {
            set_driver_board_reason_3_2_flag(0); //清零
            set_CAN_Send_Flag(0);
            BoardPower_SetState(DRIVER_BOARD_POWER_ID, 0); //断电
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
    
    /*if(GetAdapterState() != ADAPTER_STATE_TRUE) //未接上充电器
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
   
    //noerr = i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x08, 2, rx); //读取电压值
    //voltage = (signed short)(((rx[1] << 8) & 0xff00) + (rx[0] & 0x00ff));  //读取电池电压值
    if(voltage < 9000 || voltage > 12200)
    {
        re = 0;
    }
    
    //noerr = i2cRead(BATTERY_I2C, I2C2_BFG_SLAVE_ADDR, 0x10, 2, rx); //读取电流值
    //current = (signed short)(((rx[1] << 8) & 0xff00) + (rx[0] & 0x00ff));
    if(current < 1600 || current > 2500 ) //充电电流
    {
        re = 0;
    }
    
    return re;
}



