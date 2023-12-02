#include "flash.h"
#include "main.h"

#define CMD_READ_ID         0x90
#define CMD_WRITE_ENABLE    0x06
#define CMD_READ_STATUS     0x05
#define CMD_BLOCK_ERASE_32K 0x52
#define CMD_PAGE_PROGRAM    0x02
#define CMD_READ            0x03


static SPI_HandleTypeDef *spiFlash;


void flashInit(SPI_HandleTypeDef *spiFlashH)
{
  spiFlash = spiFlashH;
}


uint16_t flashReadDeviceId(void)
{
  uint8_t bufferOut[] = {CMD_READ_ID, 0, 0, 0};
  uint8_t bufferIn[] = {0, 0};

  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_RESET);
  if (HAL_SPI_Transmit(spiFlash, bufferOut, sizeof(bufferOut), 1000) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_SPI_Receive(spiFlash, bufferIn, sizeof(bufferIn), 1000) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_SET);

  return bufferIn[0]<<8 | bufferIn[1];
}


static uint8_t flashReadStatusRegister(void)
{
  uint8_t bufferOut[] = {CMD_READ_STATUS};
  uint8_t bufferIn[] = {0};
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_RESET);
  if (HAL_SPI_Transmit(spiFlash, bufferOut, sizeof(bufferOut), 1000) !=HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_SPI_Receive(spiFlash, bufferIn, sizeof(bufferIn), 1000) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_SET);

  return bufferIn[0];
}


static void flashWriteEnable(void)
{
  uint8_t bufferOut = CMD_WRITE_ENABLE;
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_RESET);
  if (HAL_SPI_Transmit(spiFlash, &bufferOut, 1, 1000) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_SET);
}


static uint32_t blockIdxToAddress(uint8_t idx)
{
  // 32K block index to address
  return idx * 0x8000;
}


void flashEraseBlock(uint8_t blockIdx)
{
  flashWriteEnable();

  uint32_t address24 = blockIdxToAddress(blockIdx);

  uint8_t bufferOut[4];
  bufferOut[0] = CMD_BLOCK_ERASE_32K;
  bufferOut[1] = (address24 >> 16) & 0xFF;
  bufferOut[2] = (address24 >> 8) & 0xFF;
  bufferOut[3] = address24 & 0xFF;

  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_RESET);
  if (HAL_SPI_Transmit(spiFlash, bufferOut, sizeof(bufferOut), 1000) !=HAL_OK)
  {
    Error_Handler();
  }
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_SET);

  // Wait until busy flag is cleared
  while (flashReadStatusRegister() & 0x01);
}


void flashWriteData(uint8_t blockIdx, uint8_t *data, uint16_t size)
{
  uint32_t address24 = blockIdxToAddress(blockIdx);
  uint16_t currentByte = 0;

  uint8_t bufferOut[260];
  bufferOut[0] = CMD_PAGE_PROGRAM;

  while (currentByte < size) {
    bufferOut[1] = (address24 >> 16) & 0xFF;
    bufferOut[2] = (address24 >> 8) & 0xFF;
    bufferOut[3] = address24 & 0xFF;
    uint16_t idx = 4;
    uint16_t pageTopIdx = currentByte + 256;
    for (; currentByte < pageTopIdx; currentByte++) {
      if (currentByte < size) {
        bufferOut[idx] = data[currentByte];
      } else {
        bufferOut[idx] = 0;
      }
      idx++;
      address24++;
    }

    flashWriteEnable();

    HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(spiFlash, bufferOut, sizeof(bufferOut), 1000) !=HAL_OK)
    {
      Error_Handler();
    }
    HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_SET);

    // Wait until busy flag is cleared
    while (flashReadStatusRegister() & 0x01);
  }
}


void flashReadData(uint8_t blockIdx, uint8_t *data, uint16_t length)
{
  uint32_t address24 = blockIdxToAddress(blockIdx);

  uint8_t bufferOut[4];
  bufferOut[0] = CMD_READ;
  bufferOut[1] = (address24 >> 16) & 0xFF;
  bufferOut[2] = (address24 >> 8) & 0xFF;
  bufferOut[3] = address24 & 0xFF;

  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_RESET);
  if (HAL_SPI_Transmit(spiFlash, bufferOut, sizeof(bufferOut), 1000) !=HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_SPI_Receive(spiFlash, data, length, 1000) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_SET);
}


void flashReadDataOffset(uint8_t blockIdx, uint8_t *data, uint16_t offset, uint16_t length)
{
  uint32_t address24 = blockIdxToAddress(blockIdx);
  address24 += offset;

  uint8_t bufferOut[4];
  bufferOut[0] = CMD_READ;
  bufferOut[1] = (address24 >> 16) & 0xFF;
  bufferOut[2] = (address24 >> 8) & 0xFF;
  bufferOut[3] = address24 & 0xFF;

  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_RESET);
  if (HAL_SPI_Transmit(spiFlash, bufferOut, sizeof(bufferOut), 1000) !=HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_SPI_Receive(spiFlash, data, length, 1000) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_SET);
}
