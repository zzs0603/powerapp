#include "Led.h"

#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"


#define TIM3_ARR (900 - 1)
#define TIM3_PSC  0


void led_init(void)
{
    GPIO_InitTypeDef         GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimBaseStructure;
    TIM_OCInitTypeDef        TIM_OCInitStructure;
    //clk
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    //remap
    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE);
    
    //GPIOB.5
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //TIM_CH2
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    //≥ı ºªØTIM3
    TIM_TimBaseStructure.TIM_Period = 899;
    TIM_TimBaseStructure.TIM_Prescaler = 0;
    TIM_TimBaseStructure.TIM_ClockDivision = 0;
    TIM_TimBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimBaseStructure);
    
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);
    
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_Cmd(TIM3, ENABLE);
}


void led_brightness_set(uint16_t val)
{
    TIM_SetCompare2(TIM3, val);
}



