#ifndef __BQ34Z100_H
#define __BQ34Z100_H

#include "iic.h"



void bq34z100_Init(void);
void bq34z100_run(signed short* pvol, signed short* pcur, signed short* psoc);

signed short bq34z100_getvol(void);
signed short bq34z100_getcur(void);
signed short bq34z100_getsoc(void);


#endif








