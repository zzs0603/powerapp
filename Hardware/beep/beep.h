#ifndef _BEEP_H
#define _BEEP_H



//·äÃùÆ÷PB9
#define BEEP_GPIO_CLK        (RCC_APB2Periph_GPIOB)
#define BEEP_PORT                    (GPIOB)
#define BEEP_PIN                     (GPIO_Pin_9)
#define BEEP_STATE                   PBout(9)
#define BEEP_ON                      (PBout(9) = 1)
#define BEEP_OFF                     (PBout(9) = 0)



void beep_init();


#endif
