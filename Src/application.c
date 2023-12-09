#include "application.h"
#include "main.h"
#include "console.h"
#include "flash.h"
#include "audio.h"
#include "sequence.h"


// Step timer config
// BPM: 90
// beats per second: 1.5
// seconds per beat: 0.666

// Timer clock: 96 MHz
// Prescaler: 65535
// Seconds per tick: 1/96 MHz * 65535 = 0.00068265625
// Ticks per beat = 0.666 / 0.000683 = 975


static TIM_HandleTypeDef *stepTimer;
static volatile bool triggerStep = false;


void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim)
{
  if (htim->Instance == stepTimer->Instance)
  {
    triggerStep = true;
  }
}


void appInit(I2S_HandleTypeDef *i2sMicH, I2S_HandleTypeDef *i2sDACH, SPI_HandleTypeDef *spiFlashH, TIM_HandleTypeDef *stepTimerH)
{
  ConsoleInit();
  flashInit(spiFlashH);
  audioInit(i2sMicH, i2sDACH);
  stepTimer = stepTimerH;

  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
}


void appLoop(void)
{
  while(1)
  {
    ConsoleProcess();
    audioProcessData();

    if (triggerStep) {
      for (int i=0; i < NUM_CHANNELS; i++) {
        ChannelParams_T params = getCurrStepChannelParams(i);
        if (params.clipNum > 0) {
          audioSetChannelParams(i, params);
          audioSetChannelRunning(i, true);
        }
      }

      step();
      triggerStep = false;
    }
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


void appSetAudioLoop(bool loop)
{
  audioSetLoop(loop);
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


void appSetAudioChannelParams(uint8_t channelIdx, ChannelParams_T params)
{
  audioSetChannelParams(channelIdx, params);
}


ChannelParams_T appGetAudioChannelParams(uint8_t channelIdx)
{
  return audioGetChannelParams(channelIdx);
}


void appSetAudioChannelRunning(uint8_t channelIdx, bool runningState)
{
  audioSetChannelRunning(channelIdx, runningState);
}


void appStartSequence(void)
{
  setStepIdx(0);
  audioPlayFromFlash();
  // Initially set all channels to not playing
  for (int i=0; i < NUM_CHANNELS; i++) {
    audioSetChannelRunning(i, false);
  }
  HAL_TIM_Base_Start_IT(stepTimer);
  triggerStep = true;
}


void appStopSequence(void)
{
  audioStop();
  HAL_TIM_Base_Stop_IT(stepTimer);
}


void appSetSequenceStepChannelParams(uint8_t stepIdx, uint8_t channelIdx, ChannelParams_T params)
{
  setStepChannelParams(channelIdx, stepIdx, params);
}


ChannelParams_T appGetSequenceStepChannelParams(uint8_t stepIdx, uint8_t channelIdx)
{
  return getStepChannelParams(channelIdx, stepIdx);
}
