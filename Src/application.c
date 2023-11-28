#include "stdbool.h"

#include "main.h"
#include "console.h"


#define CMD_READ_ID         0x90
#define CMD_WRITE_ENABLE    0x06
#define CMD_READ_STATUS     0x05
#define CMD_BLOCK_ERASE_32K 0x52
#define CMD_PAGE_PROGRAM    0x02
#define CMD_READ            0x03

#define CLIP_SIZE     16000 // Size of clip buffer in half words (1 second of audio at 16 kHz sample rate == 16000 half words == 31KiB)
#define BUFFER_SIZE   256   // Size of input/output buffer in half words (256 half words == 0.5KiB)

#define FLASH_PLAY_START_POS  5000  // Index in half words
#define FLASH_PLAY_END_POS    10000


I2S_HandleTypeDef *i2sMic;
I2S_HandleTypeDef *i2sDAC;
SPI_HandleTypeDef *spiFlash;

int16_t audio[CLIP_SIZE];
int16_t buffer[BUFFER_SIZE];

volatile int16_t *bufferPtr = &buffer[0];
volatile bool readyForData;

volatile bool recording;
volatile uint16_t sampleIdx;

int8_t audioClip = 1;
int16_t clipPos = 0;  // Byte offset into audio clip for playback from Flash

// 1 - play from RAM, 2 - play from Flash
int8_t audioType = 1;



void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  bufferPtr = &buffer[0];
  readyForData = true;
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  bufferPtr = &buffer[BUFFER_SIZE/2];
  readyForData = true;
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  bufferPtr = &buffer[0];
  readyForData = true;
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  bufferPtr = &buffer[BUFFER_SIZE/2];
  readyForData = true;
}


void flashReadDataOffset(uint8_t blockIdx, uint8_t *data, uint16_t offset, uint16_t length);

void processData()
{
  int16_t sample;
  // When recording (receiving 24bits on 32bit frames) we need to increment 4 buffer elements
  // for every sample (left top 16 bits, left bottom 8 bits, right - silent elements)
  // When playing back we transmit using 16bit so we only need to increment 2 buffer elements
  // for every sample (left 16 bits, right 16 bits).
  int8_t increment = 2;
  if (recording) {
    increment = 4;
  }

  if (audioType == 2 && !recording) {
    // Here we read the next chunk of audio clip data from Flash (enough to fill half the buffer)
    // The offset into the audio clip in bytes is given by clipPos
    // We read BUFFER_SIZE/2 bytes so BUFFER_SIZE/4 sample (samples are 16 bits each)
    // This is enough to fill half the buffer as each sample is duplicated for left and right channels
    flashReadDataOffset(audioClip - 1, (uint8_t *) audio, clipPos, BUFFER_SIZE/2);
    clipPos += BUFFER_SIZE/2;
    // Multiple start and end pos by 2 as these values are defined as 16 bit sample indexes
    if (clipPos > FLASH_PLAY_END_POS*2) {
      clipPos = FLASH_PLAY_START_POS*2;
    }
  }

  for (uint16_t i = 0; i < (BUFFER_SIZE/2) - 1; i += increment) {
    if (recording) {
      // Only MIC left channel has data so we skip the right channel
      // We also only store the top 16 bits of each 24 bit sample
      // This has the effect of a crude re-sample to 16 bits
      audio[sampleIdx] = bufferPtr[i];
    } else {
      // Send same sample to left and right channels
      sample = audio[sampleIdx];
      bufferPtr[i] = sample;
      bufferPtr[i + 1] = sample;
    }

    sampleIdx++;
    if (audioType == 1) {
      if (sampleIdx > 16000) {
        sampleIdx = 0;
        if (recording) {
          recording = false;
          HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
          // Stop recoding audio
          HAL_I2S_DMAStop(i2sMic);
        }
      }
    } else if (audioType == 2) {
      // Reset sampleIdx back to 0 on last iteration for Flash play
      // We always use the same BUFFER_SIZE/4 samples and replace these
      // on each call to processData
      // This could maybe be moved out of the loop
      if (sampleIdx >= (BUFFER_SIZE/4)) {
        sampleIdx = 0;
      }
    }
  }

  readyForData = false;
}


void appInit(I2S_HandleTypeDef *i2sMicH, I2S_HandleTypeDef *i2sDACH, SPI_HandleTypeDef *spiFlashH)
{
  i2sMic = i2sMicH;
  i2sDAC = i2sDACH;
  spiFlash = spiFlashH;

  ConsoleInit();

  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
}


void appLoop(void)
{
  while(1)
  {
    ConsoleProcess();

    if (readyForData) {
      processData();
    }
  }
}


void appToggleLED(void)
{
  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
}



uint16_t appFlashReadDeviceId(void)
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


void appRecordAudio(void)
{
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
  // Stop playing audio
  HAL_I2S_DMAStop(i2sDAC);
  // Start recording audio
  // Receiving 32bit frames and buffer uses 16bit data size so receive size is half buffer size
  audioType = 1;
  sampleIdx = 0;
  recording = true;
  HAL_I2S_Receive_DMA(i2sMic, (uint16_t *) buffer, BUFFER_SIZE/2);
}


void appPlayAudio(void)
{
  audioType = 1;
  sampleIdx = 0;
  HAL_I2S_Transmit_DMA(i2sDAC, (uint16_t *) buffer, BUFFER_SIZE);
}


void appPlayAudioFromFlash(void)
{
  audioType = 2;
  sampleIdx = 0;
  clipPos = FLASH_PLAY_START_POS*2;
  HAL_I2S_Transmit_DMA(i2sDAC, (uint16_t *) buffer, BUFFER_SIZE);
}


void appStopAudio(void)
{
  HAL_I2S_DMAStop(i2sDAC);
}


uint8_t flashReadStatusRegister(void)
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


void flashWriteEnable(void)
{
  uint8_t bufferOut = CMD_WRITE_ENABLE;
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_RESET);
  if (HAL_SPI_Transmit(spiFlash, &bufferOut, 1, 1000) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_SET);
}


uint32_t blockIdxToAddress(uint8_t idx)
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


void appSetAudioClipNum(uint8_t audioClipNum)
{
  if (audioClipNum < 1 || audioClipNum > 50) {
    return;
  }
  audioClip = audioClipNum;
}


uint8_t appGetAudioClipNum(void)
{
  return audioClip;
}


void appStoreAudio(void)
{
  uint8_t blockIdx = audioClip - 1;

  flashEraseBlock(blockIdx);
  flashWriteData(blockIdx, (uint8_t *) audio, CLIP_SIZE*2);
}


void appLoadAudio(void)
{
  uint8_t blockIdx = audioClip - 1;

  flashReadData(blockIdx, (uint8_t *) audio, CLIP_SIZE*2);
}


int16_t * appOutputAudioData(void)
{
  return audio;
}
