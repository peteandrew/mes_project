#ifndef APPLICATION_H
#define APPLICATION_H

#include "stdbool.h"
#include "stm32f4xx_hal.h"
#include "audioTypes.h"

#define STRINGIZE_DETAIL_(v) #v
#define STRINGIZE(v) STRINGIZE_DETAIL_(v)

void appInit(I2S_HandleTypeDef *i2sMicH, I2S_HandleTypeDef *i2sDACH, SPI_HandleTypeDef *spiFlashH, TIM_HandleTypeDef *stepTimerH, TIM_HandleTypeDef *encTimerH, TIM_HandleTypeDef *inputTimerH);
void appLoop(void);
void appToggleLED(void);
uint16_t appFlashReadDeviceId(void);
void appRecordAudio(void);
void appPlayAudio(void);
void appPlayAudioFromFlash(void);
void appStopAudio(void);
void appSetAudioClipNum(uint8_t audioClipNum);
uint8_t appGetAudioClipNum(void);
bool appGetAudioClipUsed(uint8_t audioClipNum);
void appSetAudioClipUsed(uint8_t audioClipNum);
void appSetAudioStartSample(uint16_t startSample);
void appSetAudioEndSample(uint16_t startSample);
void appSetAudioLoop(bool loop);
void appStoreAudio(void);
void appLoadAudio(void);
int16_t * appOutputAudioData(void);
void appSetAudioChannelParams(uint8_t channelIdx, ChannelParams_T params);
ChannelParams_T appGetAudioChannelParams(uint8_t channelIdx);
void appSetAudioChannelRunning(uint8_t channelIdx, bool runningState);
void appStartSequence(void);
void appStopSequence(void);
void appSetSequenceStepChannelParams(uint8_t stepIdx, uint8_t channelIdx, ChannelParams_T params);
ChannelParams_T appGetSequenceStepChannelParams(uint8_t stepIdx, uint8_t channelIdx);
void appToggleClipPlay(void);
void appToggleSequencePlay(void);

#endif
