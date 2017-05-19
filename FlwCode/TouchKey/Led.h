#ifndef _LED_H_
#define _LED_H_

#include "stm32f10x.h"

#define LED_HIGH 800
#define LED_LOW  400
#define LED_OFF  0



void led_init(void);
void led_brightness_set(uint16_t val);


#endif
