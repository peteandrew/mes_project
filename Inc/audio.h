#ifndef AUDIO_H
#define AUDIO_H

#include "stdbool.h"
#include "stm32f4xx_hal.h"
#include "audioTypes.h"
#include "ui_values.h"

// Samples stored in RAM or flash are a half word (only left channel is stored)
#define CLIP_SAMPLES     16000 // 1 second of audio at 16 kHz sample rate (16000 half word samples == 31KiB)

void audioInit(I2S_HandleTypeDef *i2sMicH, I2S_HandleTypeDef *i2sDACH, uiChangeCallback _uiChangeCB);
void audioProcessData(void);
void audioRecord(void);
void audioPlay(void);
void audioPlayFromFlash(void);
void audioStop(void);
void audioSetClipNum(uint8_t audioClipNum);
uint8_t audioGetClipNum(void);
void audioSetStartSample(uint16_t startSample);
void audioSetEndSample(uint16_t endSample);
void audioSetLoop(bool loop);
void audioStore(void);
void audioLoad(void);
int16_t * audioGetData(void);
void audioSetChannelParams(uint8_t channelIdx, ChannelParams_T params);
ChannelParams_T audioGetChannelParams(uint8_t channelIdx);
void audioSetChannelRunning(uint8_t channelIdx, bool runningState);
bool getAudioRunning(void);

#endif
