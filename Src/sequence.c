#include "sequence.h"
#include "audioTypes.h"

#define NUM_STEPS 16

static uint16_t currStep = 0;
static ChannelParams_T stepParams[NUM_CHANNELS][NUM_STEPS];


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


void setStepIdx(uint8_t stepIdx)
{
  if (stepIdx >= NUM_STEPS) {
    return;
  }
  currStep = stepIdx;
}


uint8_t getStepIdx()
{
  return currStep;
}


void step()
{
  if (++currStep >= NUM_STEPS) {
    currStep = 0;
  }
}
