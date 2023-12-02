#ifndef APPLICATION_H
#define APPLICATION_H

#include "stm32f4xx_hal.h"


void appInit(I2S_HandleTypeDef *i2sMicH, I2S_HandleTypeDef *i2sDACH, SPI_HandleTypeDef *spiFlashH);
void appLoop(void);
void appToggleLED(void);
uint16_t appFlashReadDeviceId(void);
void appRecordAudio(void);
void appPlayAudio(void);
void appPlayAudioFromFlash(void);
void appStopAudio(void);
void appSetAudioClipNum(uint8_t audioClipNum);
uint8_t appGetAudioClipNum(void);
void appSetAudioStartSample(uint16_t startSample);
void appSetAudioEndSample(uint16_t startSample);
void appStoreAudio(void);
void appLoadAudio(void);
int16_t * appOutputAudioData(void);

#endif
