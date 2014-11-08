#ifndef __SPI_H
#define __SPI_H

#include "mGeneral.h"

#define SPIenable   SPI_Cmd(SPI3, ENABLE)
#define SPIdisable  SPI_Cmd(SPI3, DISABLE)


void SPI3_Init(void);

u16 SPIx_ReadWriteByte(u16 TxData);

#endif 