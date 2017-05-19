#ifndef __TIMER_H
#define __TIMER_H
#include "stdint.h"
#include "stm32f10x.h"

#define MAX_ARR                             (65536)


//ϵͳʱ���ṹ
#define  SYSTIM_BASE_TIM_CLK                 (RCC_APB1Periph_TIM2)
#define  SYSTIM_BASE_TIM                     (TIM2)
#define  SYSTIM_BASE_TIM_ARR                 (1000)
#define  SYSTIM_BASE_TIM_PSC                 (72)



#define SYSTIM_BASE_10MS    (0)    
#define SYSTIM_BASE_1S  (1)
#define SYSTIM_BASE_1MS   (2)

typedef struct 
{
   u16 systim_base_10ms; //ϵͳʱ��1S����
   u8  systim_base_10ms_flag; //1sʱ�䵽
   
   u16 systim_base_1s; //ϵͳʱ��10ms����
   u8  systim_base_1s_flag; //10ms��ʱ�� 
    
   u8  systim_base_1ms_flag; //1msʱ�䵽
    
}SYSTIM_BASE;



void SYSTIM_BASE_Init(void); //ϵͳʱ����ʼ��
u8  SYSTIM_BASE_FlagGet(u8 timid); //
void SYSTIM_BASE_FlagClear(u8 timid);   //ʱ����λ��־����


#endif

