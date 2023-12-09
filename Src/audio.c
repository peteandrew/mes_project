#include "audio.h"
#include "flash.h"
#include "main.h"


// Samples sent over I2S are one word (half word for each channel)
// Samples received over I2S from the microphone are two words (one word for each channel)
// Only the top 24bits of the left channel contains audio data, the right channel is silent
#define I2S_BUFFER_SIZE   256 // Size of I2S buffer in half words (256 half words == 0.5KiB)


static I2S_HandleTypeDef *i2sMic;
static I2S_HandleTypeDef *i2sDAC;

static int16_t audio[CLIP_SAMPLES];
static int16_t buffer[I2S_BUFFER_SIZE];

static volatile int16_t *bufferPtr = &buffer[0];
static volatile bool readyForData;

typedef enum {
  AUDIO_RECORD      = 0u,
  AUDIO_RAM_PLAY    = 0x01u,
  AUDIO_FLASH_PLAY  = 0x10u
} eAudioState_T;
static eAudioState_T audioState = AUDIO_RAM_PLAY;


// When state is record or play from RAM, use only first channel
// When state is play from Flash use all channels
static ChannelParams_T channelParams[NUM_CHANNELS];
static uint16_t sampleIndexes[NUM_CHANNELS];
static bool running[NUM_CHANNELS];


void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  bufferPtr = &buffer[0];
  readyForData = true;
}


void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  bufferPtr = &buffer[I2S_BUFFER_SIZE/2];
  readyForData = true;
}


void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  bufferPtr = &buffer[0];
  readyForData = true;
}


void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  bufferPtr = &buffer[I2S_BUFFER_SIZE/2];
  readyForData = true;
}


void audioInit(I2S_HandleTypeDef *i2sMicH, I2S_HandleTypeDef *i2sDACH)
{
  i2sMic = i2sMicH;
  i2sDAC = i2sDACH;

  for (int i=0; i < NUM_CHANNELS; i++) {
    channelParams[i].clipNum = 1;
    channelParams[i].startSample = 0;
    channelParams[i].endSample = CLIP_SAMPLES - 1;
    channelParams[i].loop = true;
    sampleIndexes[i] = 0;
    running[i] = false;
  }
}


void audioProcessData(void)
{
  if (!readyForData) return;

  // Stored sample (only left frame)
  int16_t sample;
  // When recording (receiving 24bits on 32 bit frames) we need to increment 4 buffer elements (16 bits each)
  // for every sample (left 32 bit frame, right (silent) 32 bit frame).
  // When playing back we transmit using 16 bit frames so we only need to increment 2 buffer elements
  // for every sample (left 16 bits, right 16 bits).
  int8_t increment = 2;
  if (audioState == AUDIO_RECORD) {
    increment = 4;
  }
  // When in AUDIO_RECORD or AUDIO_RAM_PLAY states only use channel 0
  int8_t channelIdx = 0;
  // Sample index in to audio.
  // Follows sampleIndexes[0] for RAM recording and playback as we use entire contents of audio array.
  // Set back to 0 for playback from Flash as we only use I2S_BUFFER_SIZE/4 * NUM_CHANNELS of audio array.
  int16_t ramSampleIdx = 0;

  if (audioState == AUDIO_FLASH_PLAY) {
    for (channelIdx = 0; channelIdx < NUM_CHANNELS; channelIdx++) {
      // Here we read the next chunk of audio clip data from Flash (enough to fill half the buffer)
      // The offset into the audio clip in bytes is given by sampleIndexes[channelIdx] x 2
      // We read I2S_BUFFER_SIZE/2 bytes so I2S_BUFFER_SIZE/4 samples (samples are 16 bits each)
      // This is enough to fill half the buffer as each sample is duplicated for left and right channels
      if (running[channelIdx]) {
        flashReadDataOffset(
            channelParams[channelIdx].clipNum - 1,
            (uint8_t *) &audio[channelIdx * (I2S_BUFFER_SIZE / 2)],
            sampleIndexes[channelIdx] * 2,
            I2S_BUFFER_SIZE / 2
        );

        // FIXME: I think this should come after the buffer fill loop otherwise we prematurely stop channels
        // We also want this to apply when the audio state is AUDIO_RAM_PLAY maybe?
        sampleIndexes[channelIdx] += I2S_BUFFER_SIZE / 4;
        if (sampleIndexes[channelIdx] > channelParams[channelIdx].endSample) {
          if (channelParams[channelIdx].loop) {
            sampleIndexes[channelIdx] = channelParams[channelIdx].startSample;
          } else {
            running[channelIdx] = false;
          }
        }
      }
    }
  } else {
    ramSampleIdx = sampleIndexes[channelIdx];
  }

  for (uint16_t i = 0; i < (I2S_BUFFER_SIZE/2) - 1; i += increment) {
    if (audioState == AUDIO_RECORD) {
      // Only MIC left channel has data so we skip the right channel
      // We also only store the top 16 bits of each 24 bit sample
      // This has the effect of a crude re-sample to 16 bits
      audio[ramSampleIdx] = bufferPtr[i];
    } else if (audioState == AUDIO_RAM_PLAY) {
      // FIXME: I think it should be possible to combine the code for AUDIO_RAM_PLAY and AUDIO_FLASH_PLAY
      // Maybe we just iterate over first channel instead of all channels for AUDIO_RAM_PLAY

      // Send same sample to left and right channels
      if (running[channelIdx]) {
        sample = audio[ramSampleIdx];
      } else {
        sample = 0;
      }
      bufferPtr[i] = sample;
      bufferPtr[i + 1] = sample;
    } else {
      sample = 0;
      for (channelIdx = 0; channelIdx < NUM_CHANNELS; channelIdx++) {
        if (running[channelIdx]) {
          sample += audio[ramSampleIdx + (channelIdx * (I2S_BUFFER_SIZE / 2))];
        }
      }
      bufferPtr[i] = sample;
      bufferPtr[i + 1] = sample;
    }

    ramSampleIdx++;
    if (audioState == AUDIO_RAM_PLAY || audioState == AUDIO_RECORD) {
      if (ramSampleIdx >= 16000) {
        ramSampleIdx = 0;
        if (audioState == AUDIO_RECORD) {
          HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
          // Stop recoding audio
          HAL_I2S_DMAStop(i2sMic);
        } else if (!channelParams[channelIdx].loop) {
          running[channelIdx] = false;
        }
      }
    }
  }

  if (audioState == AUDIO_RAM_PLAY || audioState == AUDIO_RECORD) {
    sampleIndexes[channelIdx] = ramSampleIdx;
  }

  readyForData = false;
}


void audioRecord(void)
{
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
  // Stop playing audio
  HAL_I2S_DMAStop(i2sDAC);
  // Start recording audio
  // Receiving 32bit frames and buffer uses 16bit data size so receive size is half buffer size
  audioState = AUDIO_RECORD;
  sampleIndexes[0] = 0;
  HAL_I2S_Receive_DMA(i2sMic, (uint16_t *) buffer, I2S_BUFFER_SIZE/2);
}


void audioPlay(void)
{
  audioState = AUDIO_RAM_PLAY;
  sampleIndexes[0] = 0;
  running[0] = true;
  HAL_I2S_Transmit_DMA(i2sDAC, (uint16_t *) buffer, I2S_BUFFER_SIZE);
}


void audioPlayFromFlash(void)
{
  audioState = AUDIO_FLASH_PLAY;
  sampleIndexes[0] = channelParams[0].startSample;
  running[0] = true;
  HAL_I2S_Transmit_DMA(i2sDAC, (uint16_t *) buffer, I2S_BUFFER_SIZE);
}


void audioStop(void)
{
  HAL_I2S_DMAStop(i2sDAC);
}


void audioSetClipNum(uint8_t audioClipNum)
{
  if (audioClipNum < 1 || audioClipNum > 50) {
    return;
  }
  channelParams[0].clipNum = audioClipNum;
}


uint8_t audioGetClipNum(void)
{
  return channelParams[0].clipNum;
}


void audioSetStartSample(uint16_t startSample)
{
  if (startSample > CLIP_SAMPLES) {
    return;
  }
  if (startSample >= channelParams[0].endSample) {
    return;
  }
  channelParams[0].startSample = startSample;
}


void audioSetEndSample(uint16_t endSample)
{
  if (endSample > CLIP_SAMPLES) {
    return;
  }
  if (endSample <= channelParams[0].startSample) {
    return;
  }
  channelParams[0].endSample = endSample;
}


void audioSetLoop(bool loop)
{
  channelParams[0].loop = loop;
}


void audioStore(void)
{
  uint8_t blockIdx = channelParams[0].clipNum - 1;

  flashEraseBlock(blockIdx);
  flashWriteData(blockIdx, (uint8_t *) audio, CLIP_SAMPLES*2);
}


void audioLoad(void)
{
  uint8_t blockIdx = channelParams[0].clipNum - 1;

  flashReadData(blockIdx, (uint8_t *) audio, CLIP_SAMPLES*2);
}


int16_t * audioGetData(void)
{
  return audio;
}


void audioSetChannelParams(uint8_t channelIdx, ChannelParams_T params)
{
  channelParams[channelIdx] = params;
}


ChannelParams_T audioGetChannelParams(uint8_t channelIdx)
{
  return channelParams[channelIdx];
}


void audioSetChannelRunning(uint8_t channelIdx, bool runningState)
{
  running[channelIdx] = runningState;
  if (runningState) {
    sampleIndexes[channelIdx] = channelParams[channelIdx].startSample;
  }
}
