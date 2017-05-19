#ifndef _TOUCHKEY_H
#define _TOUCHKEY_H

#include "gpio.h"

/* Touch Pad TIM defined */
#define TOUCH_KEY_TIM_CLK                   (RCC_APB2Periph_TIM1)
#define TOUCH_KEY_TIM                       (TIM1)
#define TOUCH_KEY_TIM_ARR                   (1000)
#define TOUCH_KEY_TIM_PSC                   (72)         



/* Touch Key port & pin defined */
#define TOUCH_KEY_GPIO_CLK              (RCC_APB2Periph_GPIOA)
#define TOUCH_KEY_GPIO_PORT             (GPIOA)
#define TOUCH_KEY_GPIO_PIN              (GPIO_Pin_8)
#define TOUCH_KEY                        PAin(8)


/* Touch Led port & pin defined */
#define TOUCH_LED_GPIO_CLK              (RCC_APB2Periph_GPIOB)
#define TOUCH_LED_GPIO_PORT             (GPIOB)
#define TOUCH_LED_GPIO_PIN              (GPIO_Pin_5)

#define TOUCH_LED_EN                    PBout(5)




typedef enum _KEYSTATE
{
    CLICK = 1,
    HOLDDOWN = 2,
    DOUBCLICK_TEMP = 3, //
    DOUBCLICK = 4,
}KEYSTATE;

#define KEY_DOWN 1
#define KEY_UP   0

#define DEATHTIME           10       //10ms
#define CLICK_TIME          3000
#define HOLDDOWN_TIME       3000
#define DOUBCLICK_TIME      300


typedef struct
{
    uint8_t   state;
    uint32_t  doubclk_timeflow;
    uint32_t  timeflow;
    uint8_t   check_flag_en;
}KEY_STR;



void TouchKey_Init(void);
void Key_Process(void);

void TouchKey_Check_Flag(uint8_t val);

void set_low_power_hold_key_flag(uint8_t flag);
uint8_t get_low_power_hold_key_flag(void);

#endif

