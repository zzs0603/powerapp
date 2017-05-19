#ifndef _FLW_GPIO_H
#define _FLW_GPIO_H
#include "stm32f10x_gpio.h"
#include "common.h"



void GPIO_Configuration(GPIO_TypeDef *GPIOx, uint16_t pin, GPIOMode_TypeDef mode);


#endif 

