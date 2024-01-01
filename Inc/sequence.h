#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "audioTypes.h"
#include "ui_values.h"

// After storing 100 audio clips (12800 pages) the flash chip has enough space for
// 224 sequences (each sequence uses one 4KB sector - 16 pages, 16384 - 12800 = 3584 pages,
// 3584 / 16 = 224).
// But for now we will limit the number of sequences to 150 to allow space for future uses
// of the remaining flash capacity.
#define NUM_SEQUENCES 150
#define NUM_STEPS 16
#define MAX_STEP_IDX NUM_STEPS-1

void sequenceInit(uiChangeCallback _uiChangeCB);
void sequenceSetNum(uint8_t sequenceNum);
uint8_t sequenceGetNum(void);
ChannelParams_T getStepChannelParams(uint8_t channelIdx, uint8_t stepIdx);
ChannelParams_T getCurrStepChannelParams(uint8_t channelIdx);
void setStepChannelParams(uint8_t channelIdx, uint8_t stepIdx, ChannelParams_T channelParams);
uint8_t getStepIdx();
void step();
void sequenceStart(void);
void sequenceStop(void);
bool getSequencePlaying(void);
void sequenceStore(void);
void sequenceLoad(void);
bool getSequenceUsed(void);

#endif
