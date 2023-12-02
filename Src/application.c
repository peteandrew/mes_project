#include "main.h"
#include "console.h"
#include "flash.h"
#include "audio.h"


void appInit(I2S_HandleTypeDef *i2sMicH, I2S_HandleTypeDef *i2sDACH, SPI_HandleTypeDef *spiFlashH)
{
  ConsoleInit();
  flashInit(spiFlashH);
  audioInit(i2sMicH, i2sDACH);

  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
}


void appLoop(void)
{
  while(1)
  {
    ConsoleProcess();
    audioProcessData();
  }
}


void appToggleLED(void)
{
  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
}



uint16_t appFlashReadDeviceId(void)
{
  return flashReadDeviceId();
}


void appRecordAudio(void)
{
  audioRecord();
}


void appPlayAudio(void)
{
  audioPlay();
}


void appPlayAudioFromFlash(void)
{
  audioPlayFromFlash();
}


void appStopAudio(void)
{
  audioStop();
}


void appSetAudioClipNum(uint8_t audioClipNum)
{
  audioSetClipNum(audioClipNum);
}


uint8_t appGetAudioClipNum(void)
{
  return audioGetClipNum();
}


void appSetAudioStartSample(uint16_t startSample)
{
  audioSetStartSample(startSample);
}


void appSetAudioEndSample(uint16_t endSample)
{
  audioSetEndSample(endSample);
}


void appStoreAudio(void)
{
  audioStore();
}


void appLoadAudio(void)
{
  audioLoad();
}


int16_t * appOutputAudioData(void)
{
  return audioGetData();
}
