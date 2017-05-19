#ifndef _CRC16_H
#define _CRC16_H


typedef struct{
    char Crch;
    char Crcl;
}CRCCODE;

extern void crc16(char Data,CRCCODE *Code);

extern void crc16block(const char *pData, int Size, CRCCODE *Code);

#endif

