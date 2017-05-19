#ifndef _FW_VERSION_H
#define _FW_VERSION_H


#include "stdint.h"

//版本标示格式: VM.HW.SW.REPE

#define  VM     2
#define  HW     0
#define  SW     0
#define  REPE   22



uint8_t version_view(uint8_t *version_table, uint8_t len);


#endif

