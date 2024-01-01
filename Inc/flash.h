#ifndef FLASH_H
#define FLASH_H

#include "stm32f4xx_hal.h"

void flashInit(SPI_HandleTypeDef *spiFlashH);
uint16_t flashReadDeviceId(void);
void flashEraseSector(uint16_t sectorIdx);
void flashEraseBlock(uint8_t blockIdx);
void flashWriteDataSector(uint16_t sectorIdx, uint8_t *data, uint16_t size);
void flashWriteDataBlock(uint8_t blockIdx, uint8_t *data, uint16_t size);
void flashReadDataSector(uint16_t sectorIdx, uint8_t *data, uint16_t length);
void flashReadDataBlock(uint8_t blockIdx, uint8_t *data, uint16_t length);
void flashReadDataBlockOffset(uint8_t blockIdx, uint8_t *data, uint16_t offset, uint16_t length);

#endif
