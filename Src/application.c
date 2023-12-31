#include "application.h"
#include "main.h"
#include "console.h"
#include "flash.h"
#include "audio.h"
#include "sequence.h"
#include "ui.h"


// Step timer config
// BPM: 90
// beats per second: 1.5
// steps per second: 1.5 * 4 = 6
// seconds per step: 0.167

// Timer clock: 96 MHz
// Prescaler: 65535
// Seconds per tick: 1/96 MHz * 65535 = 0.00068265625
// Ticks per step = 0.167 / 0.000683 = 245

static TIM_HandleTypeDef *stepTimer;
static TIM_HandleTypeDef *encTimer;
static TIM_HandleTypeDef *inputTimer;

static volatile uint16_t previousCount;
static volatile int16_t countChange = 0;
static volatile bool tempButtonPressed = false;
static volatile bool buttonPressed = false;
static bool sequencePlaying = false;
static volatile bool triggerStep = false;


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == ENC_BUTTON_Pin) {
    tempButtonPressed = HAL_GPIO_ReadPin(ENC_BUTTON_GPIO_Port, ENC_BUTTON_Pin) == GPIO_PIN_RESET;
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    inputTimer->Instance->CNT = 0;
    HAL_TIM_Base_Start_IT(inputTimer);
  }
}


void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim)
{
  if (htim->Instance == inputTimer->Instance) {
	HAL_TIM_Base_Stop_IT(inputTimer);
	bool testButtonPressed = HAL_GPIO_ReadPin(ENC_BUTTON_GPIO_Port, ENC_BUTTON_Pin) == GPIO_PIN_RESET;
	if (testButtonPressed == tempButtonPressed) {
	  buttonPressed = testButtonPressed;
	} else {
	  buttonPressed = false;
	}
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
  }
  if (htim->Instance == stepTimer->Instance)
  {
    triggerStep = true;
  }
}


void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  countChange = htim->Instance->CNT - previousCount;
  previousCount = htim->Instance->CNT = 32767;
}


void appInit(I2S_HandleTypeDef *i2sMicH, I2S_HandleTypeDef *i2sDACH, SPI_HandleTypeDef *spiFlashH, TIM_HandleTypeDef *stepTimerH, TIM_HandleTypeDef *encTimerH, TIM_HandleTypeDef *inputTimerH)
{
  ConsoleInit();
  flashInit(spiFlashH);
  audioInit(i2sMicH, i2sDACH, &uiValueChangeCB);
  sequenceInit(&uiValueChangeCB);
  stepTimer = stepTimerH;
  encTimer = encTimerH;
  HAL_TIM_Encoder_Start_IT(encTimerH, TIM_CHANNEL_ALL);
  previousCount = encTimerH->Instance->CNT = 32767;
  uiInit();
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
  inputTimer = inputTimerH;
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

    uiUpdate(countChange, buttonPressed);
    countChange = 0;
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


bool appGetAudioClipUsed(uint8_t audioClipNum)
{
  return audioClipUsed(audioClipNum);
}


void appSetAudioClipUsed(uint8_t audioClipNum)
{
  audioSetClipUsed(audioClipNum);
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
  sequencePlaying = true;
}


void appStopSequence(void)
{
  audioStop();
  HAL_TIM_Base_Stop_IT(stepTimer);
  sequencePlaying = false;
}


void appSetSequenceStepChannelParams(uint8_t stepIdx, uint8_t channelIdx, ChannelParams_T params)
{
  setStepChannelParams(channelIdx, stepIdx, params);
}


ChannelParams_T appGetSequenceStepChannelParams(uint8_t stepIdx, uint8_t channelIdx)
{
  return getStepChannelParams(channelIdx, stepIdx);
}


void appToggleClipPlay(void)
{
  if (getAudioRunning()) {
    audioStop();
  } else {
    audioPlayFromFlash();
  }
}


void appToggleSequencePlay(void)
{
  if (sequencePlaying) {
    appStopSequence();
  } else {
    appStartSequence();
  }
}
