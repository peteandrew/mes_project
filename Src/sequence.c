#include "main.h"
#include "sequence.h"
#include "audioTypes.h"
#include "audio.h"
#include "flash.h"

// Step timer config
// BPM: 90
// beats per second: 1.5
// steps per second: 1.5 * 4 = 6
// seconds per step: 0.167

// Timer clock: 96 MHz
// Prescaler: 65535
// Seconds per tick: 1/96 MHz * 65535 = 0.00068265625
// Ticks per step = 0.167 / 0.000683 = 245

// Lowest flash sector available for sequences is found by multiplying the number of flash pages
// per 32K block by the number of clips and dividing by the number of pages per 4K sector
#define BOTTOM_SEQUENCE_SECTOR (NUM_CLIPS * 128) / 16

static uint8_t sequenceIdx = 0;
static bool sequenceUsed = false;
static uint16_t currStep = 0;
static bool sequencePlaying = false;
// stepParams size, if NUM_CHANNELS=3 and NUM_STEPS=16 and ChannelParams_T size=48 bytes
// 3*16*48 = 2304 bytes
// 2304 bytes = 9 flash pages (256 bytes) which means each sequence will use one 4KiB sector (16 pages)
// as this is the smallest number of pages we can erase at a time.
static ChannelParams_T stepParams[NUM_CHANNELS][NUM_STEPS];

static uiChangeCallback uiChangeCB;


static uint16_t sequenceIdxToFlashSectorIdx(uint8_t sequenceIdx)
{
  return BOTTOM_SEQUENCE_SECTOR + sequenceIdx;
}


void sequenceInit(uiChangeCallback _uiChangeCB)
{
  uiChangeCB = _uiChangeCB;
  sequenceLoad();
}


void sequenceSetNum(uint8_t sequenceNum)
{
  if (sequenceNum < 1 || sequenceNum > NUM_SEQUENCES) {
      return;
  }
  sequenceIdx = sequenceNum - 1;
  currStep = 0;
  sequenceLoad();
  uiChangeCB(UI_SEQ);
}


uint8_t sequenceGetNum(void)
{
  return sequenceIdx + 1;
}


ChannelParams_T getStepChannelParams(uint8_t channelIdx, uint8_t stepIdx)
{
  return stepParams[channelIdx][stepIdx];
}


ChannelParams_T getCurrStepChannelParams(uint8_t channelIdx)
{
  return stepParams[channelIdx][currStep];
}


void setStepChannelParams(uint8_t channelIdx, uint8_t stepIdx, ChannelParams_T channelParams)
{
  stepParams[channelIdx][stepIdx] = channelParams;
}


uint8_t getStepIdx()
{
  return currStep;
}


void step()
{
  for (int i=0; i < NUM_CHANNELS; i++) {
    ChannelParams_T params = getCurrStepChannelParams(i);
    if (params.clipNum > 0) {
      audioSetChannelParams(i, params);
      audioSetChannelRunning(i, true);
    }
  }

  // Turn on status LED on every beat
  // (there are four steps - quarter beats - per beat)
  if (currStep % 4 == 0) {
      HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_SET);
  } else {
      HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_RESET);
  }

  if (++currStep >= NUM_STEPS) {
    currStep = 0;
  }
}


void sequenceStart(void)
{
  currStep = 0;
  audioPlayFromFlash();
  // Initially set all channels to not playing
  for (int i=0; i < NUM_CHANNELS; i++) {
    audioSetChannelRunning(i, false);
  }
  sequencePlaying = true;
}


void sequenceStop(void)
{
  audioStop();
  sequencePlaying = false;
}


bool getSequencePlaying(void)
{
  return sequencePlaying;
}


void sequenceStore(void)
{
  uint16_t sectorIdx = sequenceIdxToFlashSectorIdx(sequenceIdx);

  flashEraseSector(sectorIdx);
  flashWriteDataSector(sectorIdx, (uint8_t *) stepParams, sizeof(stepParams));
}


void sequenceLoad(void)
{
  uint16_t sectorIdx = sequenceIdxToFlashSectorIdx(sequenceIdx);
  flashReadDataSector(sectorIdx, (uint8_t *) stepParams, sizeof(stepParams));
  // If we load sequence data from an empty flash sector all bytes will be
  // initialised to 0xFF. We can check this by checking the clip number of
  // the first step of the first channel.
  if (stepParams[0][0].clipNum == 255) {
    sequenceUsed = false;
  } else {
    sequenceUsed = true;
  }
  // We need to reset all values if we've loading an empty sequence
  if (!sequenceUsed) {
    for (uint8_t channelIdx=0; channelIdx < NUM_CHANNELS; channelIdx++) {
      for (uint8_t stepIdx=0; stepIdx < NUM_STEPS; stepIdx++) {
        stepParams[channelIdx][stepIdx].clipNum = 0;
        stepParams[channelIdx][stepIdx].startSample = 0;
        stepParams[channelIdx][stepIdx].endSample = MAX_SAMPLE_IDX;
        stepParams[channelIdx][stepIdx].loop = false;
      }
    }
  }
}


bool getSequenceUsed(void)
{
  return sequenceUsed;
}
