#ifndef _ADC_H
#define _ADC_H

#include "sys.h"



void adc_init(void);
u16 Get_Adc(u8 ch);
u16 Get_Adc_Average(u8 ch,u8 times);




#endif

