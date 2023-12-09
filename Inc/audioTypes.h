#ifndef AUDIO_TYPES_H
#define AUDIO_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define NUM_CHANNELS 3

typedef struct {
  uint8_t clipNum;
  uint16_t startSample;
  uint16_t endSample;
  bool loop;
} ChannelParams_T;

#endif
