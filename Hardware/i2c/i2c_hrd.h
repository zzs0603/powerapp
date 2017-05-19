#ifndef _I2C_HRD_H
#define _I2C_HRD_H


#include "stm32f10x.h"

//exported types
typedef enum i2c_result
{
    NO_ERR = 0,
    TIMEOUT = 1,
    BUS_BUSY = 2,
    SEND_START_ERR = 3,
    ADDR_MATCH_ERR = 4,
    ADDR_HRADER_MATCH_ERR = 5,
    DATA_TIMEOUT = 6,
    WAIT_COMM = 7,
    STOP_TIMEOUT = 8
}I2C_Result;

typedef enum i2c_state
{
    COMM_DONE = 0,
    COMM_PRE = 1,
    COMM_IN_PROCESS = 2,
    CHECK_IN_PROCESS =3,
    COMM_EXIT = 4 //
}I2C_STATE;

extern I2C_STATE i2c_comm_state;
extern uint8_t MasterReceptionComplete;
extern uint8_t MasterTransitionComplete;
extern uint8_t WriteComplete;
extern uint8_t SlaveReceptionComplete;
extern uint8_t SlaveTransitionComplete;

void I2C_Comm_Init(I2C_TypeDef* I2CX, uint32_t I2C_Speed, uint16_t I2C_Addr);
void I2C_Comm_MasterWrite(I2C_TypeDef* I2Cx, uint16_t slave_addr, uint32_t offset, uint8_t *pBuffer, uint32_t length);
//void I2C_Comm_SlaveRead(I2C_TypeDef *I2Cx, uint8_t *pBuffer, uint32_t length);
void I2C_Comm_MasterRead(I2C_TypeDef* I2Cx, uint16_t slave_addr, uint32_t offset, uint8_t *pBuffer, uint32_t length);
//void I2C_Comm_SlaveWrite(I2C_TypeDef* I2Cx, uint8_t* pBuffer, uint32_t length);

void i2c1_evt_isr(void);
void i2c1_err_isr(void);
void i2c2_evt_isr(void);
void i2c2_err_isr(void);


#endif









