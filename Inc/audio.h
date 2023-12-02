#ifndef AUDIO_H
#define AUDIO_H

#include "stm32f4xx_hal.h"

void audioInit(I2S_HandleTypeDef *i2sMicH, I2S_HandleTypeDef *i2sDACH);
void audioProcessData(void);
void audioRecord(void);
void audioPlay(void);
void audioPlayFromFlash(void);
void audioStop(void);
void audioSetClipNum(uint8_t audioClipNum);
uint8_t audioGetClipNum(void);
void audioSetStartSample(uint16_t startSample);
void audioSetEndSample(uint16_t endSample);
void audioStore(void);
void audioLoad(void);
int16_t * audioGetData(void);

#endif
