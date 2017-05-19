#ifndef _BOARDPOWERSUPPLY_H
#define _BOARDPOWERSUPPLY_H


#include "gpio.h"


/* main board port & pin defined */
#define MAIN_BOARD_POWER_GPIO_CLK       (RCC_APB2Periph_GPIOA)
#define MAIN_BOARD_POWER_EN_PORT        (GPIOA)
#define MAIN_BOARD_POWER_EN_PIN         (GPIO_Pin_4)
#define MAIN_BOARD_POWER_EN             PAout(4)

/* drv board port & pin defined */
#define DRIVER_BOARD_POWER_GPIO_CLK     (RCC_APB2Periph_GPIOA)
#define DRIVER_BOARD_POWER_EN_PORT      (GPIOA)
#define DRIVER_BOARD_POWER_EN_PIN       (GPIO_Pin_5)
#define DRIVER_BOARD_POWER_EN           PAout(5)


#define MAIN_BOARD_POWER_ID        1
#define DRIVER_BOARD_POWER_ID      2
#define ALL_BOARD_POWER_ID         0xff

void BoardPowerSupply_Init(void);
void BoardPower_SetState(uint8_t power_id, uint8_t power_state);
uint8_t BoardPower_GetState(void);
void BoardPower_Poweroff_flag(uint8_t flag);
void BoardPower_Poweroff_Ctrl(void);

uint8_t Get_BoardPower_Poweroff_flag(void);

void set_driver_board_poweron_reason(uint8_t reason);
uint8_t get_driver_board_poweron_reason(void);

void set_driver_board_reason_3_2_flag(uint8_t flag);
uint8_t get_driver_board_reason_3_2_flag(void);

#endif


