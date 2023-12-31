#ifndef AUDIO_TYPES_H
#define AUDIO_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Samples stored in RAM or flash are a half word (only left channel is stored)
#define CLIP_SAMPLES    16000 // 1 second of audio at 16 kHz sample rate (16000 half word samples == 31KiB)
#define MAX_SAMPLE_IDX CLIP_SAMPLES-1
// The flash chip can store 128 x 32KiB blocks
// Each audio clip consumes one block. 768 bytes (3 pages) of each block is not used for audio data.
// 1 block = 32768 bytes (128 pages). 1 clip = 32000 bytes (125 pages).
// Limit the number of clips to 100 to allow the rest of the flash to be used for sequences.
#define NUM_CLIPS 100
#define NUM_CHANNELS 3
#define MAX_CHANNEL_IDX NUM_CHANNELS-1

typedef struct {
  uint8_t clipNum;
  uint16_t startSample;
  uint16_t endSample;
  bool loop;
} ChannelParams_T;

#endif
