#include "gpio.h"
#include "timer.h"
#include "stdio.h"
#include "string.h"



static SYSTIM_BASE systim_base;

///////系统时基初始化
void SYSTIM_BASE_Init(void)
{
    NVIC_InitTypeDef   NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    
    
    memset(&systim_base, 0, sizeof(SYSTIM_BASE));
    
    RCC_APB1PeriphClockCmd(SYSTIM_BASE_TIM_CLK, ENABLE); //时钟使能
     
    TIM_TimeBaseStructure.TIM_Period = SYSTIM_BASE_TIM_ARR - 1;                 //自动重装载寄存器周期的值	
    TIM_TimeBaseStructure.TIM_Prescaler = SYSTIM_BASE_TIM_PSC - 1;              //预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;     //设置时钟分割:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数模式
    TIM_TimeBaseInit(SYSTIM_BASE_TIM, &TIM_TimeBaseStructure); 
     
    TIM_ITConfig(SYSTIM_BASE_TIM, TIM_IT_Update, ENABLE ); //允许更新中断
    
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn; //
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; //
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3; //
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ 
    NVIC_Init(&NVIC_InitStructure); //
    
    TIM_Cmd(SYSTIM_BASE_TIM, ENABLE); //
}

void TIM2_IRQHandler(void) //1ms系统时基中断
{
    if(TIM_GetITStatus(SYSTIM_BASE_TIM, TIM_IT_Update) != RESET) 
    {
        TIM_ClearITPendingBit(SYSTIM_BASE_TIM, TIM_IT_Update); 
        
        systim_base.systim_base_1ms_flag = 1;
        
        if(++systim_base.systim_base_10ms >= 10) //1s计时到 置位标志
        {
            systim_base.systim_base_10ms = 0;
            systim_base.systim_base_10ms_flag = 1;
        }
        
        if(++systim_base.systim_base_1s >= 1000)
        {
            systim_base.systim_base_1s = 0;
            systim_base.systim_base_1s_flag = 1;
        }
    }
}



u8  SYSTIM_BASE_FlagGet(u8 timid)
{
    u8 flag = 0;
    switch(timid)
     {
         case SYSTIM_BASE_10MS:
         {
             flag = systim_base.systim_base_10ms_flag;
         }break;
         
         case SYSTIM_BASE_1S:
         {
             flag = systim_base.systim_base_1s_flag;
         }break;
         
         case SYSTIM_BASE_1MS:
         {
             flag = systim_base.systim_base_1ms_flag;
         }break;
         
         default:
             flag = 0;
             break;
     }
     
     return flag;
}

void SYSTIM_BASE_FlagClear(u8 timid)
{
     switch(timid)
     {
         case SYSTIM_BASE_10MS:
         {
             systim_base.systim_base_10ms_flag = 0;
         }break;
         
         case SYSTIM_BASE_1S:
         {
             systim_base.systim_base_1s_flag = 0;
         }break;
         
         case SYSTIM_BASE_1MS:
         {
             systim_base.systim_base_1ms_flag = 0; 
         }break;
         
         default:
             break;
     }
}
