#ifndef _FUELGAUGE_H
#define _FUELGAUGE_H


#include "stdint.h"
//#define I2C1_DR                     ((uint32_t)0x40005410)
//#define I2C1_SPEED                  (300000)
//#define I2C1_SLED_SLAVE_ADDR        (0x46)
//#define I2C1_BLLED_SLAVE_ADDR       (0x4E)

#define I2C2_DR                     ((uint32_t)0x40005810)
#define I2C2_SPEED                  (100000)
#define I2C2_BFG_SLAVE_ADDR         (0xAA)

#define BATTERY_I2C                     (I2C2)
#define BATTERY_I2C2_CLK                (RCC_APB1Periph_I2C2)
#define BATTERY_I2C2_GPIO_CLK           (RCC_APB2Periph_GPIOB)
#define BATTERY_I2C2_GPIO_PORT          (GPIOB)
#define BATTERY_I2C2_SCL_PIN            (GPIO_Pin_10)
#define BATTERY_I2C2_SDA_PIN            (GPIO_Pin_11)

#define BQ34Z100_DMA_TX_CHANNEL     (DMA1_Channel4)
#define BQ34Z100_DMA_RX_CHANNEL     (DMA1_Channel5)

#define BQ34Z100_DMA_TX_IRQ         (DMA1_Channel4_IRQn) //
#define BQ34Z100_DMA_RX_IRQ         (DMA1_Channel5_IRQn)


#define BQ34Z100_DMA_TX_TCFLAG      (DMA1_FLAG_TC4)
#define BQ34Z100_DMA_RX_TCFLAG      (DMA1_FLAG_TC5)


//充电状态管脚定义 PB1
#define CHARGE_STATE_GPIO_CLK       (RCC_APB2Periph_GPIOB)
#define CHARGE_STATE_PORT           (GPIOB)
#define CHARGE_STATE_PIN            (GPIO_Pin_1)
#define CHARGE_STATE                PBin(1)
#define CHARGE_STATE_TRUE           (0)
#define CHARGE_STATE_FASLE          (1)




//适配器接入状态管脚定义  PB14
//#define ADAPTER_STATE_PORT           (GPIOB)
//#define ADAPTER_STATE_PIN            (GPIO_Pin_14)
//#define ADAPTER_STATE                PBin(14)
//#define ADAPTER_STATE_TRUE           (1)
//#define ADAPTER_STATE_FASLE          (0)

//适配器接入状态管脚定义  PA6
#define ADAPTER_STATE_PORT           (GPIOA)
#define ADAPTER_STATE_PIN            (GPIO_Pin_6)
#define ADAPTER_STATE                PAin(6)
#define ADAPTER_STATE_TRUE           (1)
#define ADAPTER_STATE_FASLE          (0)


//电源开关在适配器接入情况下的状态
#define POWERSWITCH_STATE_PORT           (GPIOB)
#define POWERSWITCH_STATE_PIN            (GPIO_Pin_12)
#define POWERSWITCH_STATE                PBin(12)
#define POWERSWITCH_STATE_ON           (1)
#define POWERSWITCH_STATE_OFF          (0)



#define CLI()      __set_PRIMASK(1)  
#define SEI()      __set_PRIMASK(0)  



void FuelGauge_Init(void);


void Report_Charge_State_With_Can(void);
void Report_Battery_State_Init(void);

uint8_t GetAdapterState(void);
uint8_t GetPowerSwitchState(void);
//uint8_t GetChargeState(void);

void i2c_Report_Battery_State_With_Uart(void);
    
uint8_t FuelGauge_Check(void);
uint8_t Get_SOC(void);

void driver_board_poweron_reason_ctrl(void);
void driver_board_poweroff_reason_ctrl(void);


#endif

