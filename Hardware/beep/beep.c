#include "beep.h"
#include "gpio.h"


static unsigned int cur_count = 0, tar_count = 0;
static unsigned char beep_on_flag = 0;

void beep_init()
{
    RCC_APB2PeriphClockCmd(BEEP_GPIO_CLK, ENABLE);
    GPIO_Configuration(BEEP_PORT, BEEP_PIN, GPIO_Mode_Out_PP); //·äÃùÆ÷ÅäÖÃ
    BEEP_OFF;
}



//¿ØÖÆbeepÏìµÄÊ±¼äms
void beep_run()
{
    if(beep_on_flag)
    {
        BEEP_ON;
        if(++cur_count >= tar_count)
        {
            beep_on_flag = 0;
            cur_count = tar_count;
            BEEP_OFF;
        }
    }
}

void beep_on_ms(unsigned int ms)
{
    beep_on_flag = 1;
    cur_count = 0;
    tar_count = ms;
}

