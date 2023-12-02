#ifndef FLASH_H
#define FLASH_H

#include "stm32f4xx_hal.h"

void flashInit(SPI_HandleTypeDef *spiFlashH);
uint16_t flashReadDeviceId(void);
void flashEraseBlock(uint8_t blockIdx);
void flashWriteData(uint8_t blockIdx, uint8_t *data, uint16_t size);
void flashReadData(uint8_t blockIdx, uint8_t *data, uint16_t length);
void flashReadDataOffset(uint8_t blockIdx, uint8_t *data, uint16_t offset, uint16_t length);

#endif
