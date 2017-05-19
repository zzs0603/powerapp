/*
 * crc.h
 *
 *  Created on: 2016年3月4日
 *      Author: shench
 */

#ifndef COM_CRC_H_
#define COM_CRC_H_

#include "stdint.h"

/**
 * MODBUS :Poly 0x8005, init 0xFFFF
 * IBM    :Poly 0x8005  init 0x0000
 * CCITT  :Poly 0x1021  init 0xFFFF
 */
#define CRC16_MODBUS_TYPE
//#define CRC16_CCITT_TYPE

uint8_t crc8_updateByte(uint8_t crc, const uint8_t data);
uint8_t crc8_updateCRC(uint8_t crc, const uint8_t *data, int32_t length);

uint16_t crc16_updateByte(uint16_t crc, const uint8_t data);
uint16_t crc16_updateCRC(uint16_t crc, const uint8_t *data, int32_t length);

uint32_t crc32_updateByte(uint32_t crc, const uint8_t data);
uint32_t crc32_updateCRC(uint32_t crc, const uint8_t *data, int32_t length);

#endif /* COM_CRC_H_ */
