#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "audioTypes.h"

ChannelParams_T getStepChannelParams(uint8_t channelIdx, uint8_t stepIdx);
ChannelParams_T getCurrStepChannelParams(uint8_t channelIdx);
void setStepChannelParams(uint8_t channelIdx, uint8_t stepIdx, ChannelParams_T channelParams);
void setStepIdx(uint8_t stepIdx);
uint8_t getStepIdx();
void step();

#endif
