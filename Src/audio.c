#include "stdbool.h"

#include "audio.h"
#include "flash.h"
#include "main.h"


// Create audio state variable / enum
// State can be: record, play from RAM, play from Flash

// Create struct to store channel parameters
// Attributes for: clip number, start offset (in samples), end offset, current offset
// Create array of audio channel states
// When state is record or play from RAM, use only first channel
// When state is play from Flash use all channels

// Add functions to set channel parameters



// Samples stored in RAM or flash are a half word (only left channel is stored)
#define CLIP_SAMPLES     16000 // 1 second of audio at 16 kHz sample rate (16000 half word samples == 31KiB)
// Samples sent over I2S are one word (half word for each channel)
// Samples received over I2S from the microphone are two words (one word for each channel)
// Only the top 24bits of the left channel contains audio data, the right channel is silent
#define I2S_BUFFER_SIZE   256 // Size of I2S buffer in half words (256 half words == 0.5KiB)

#define FLASH_PLAY_START_POS  5000  // Index in samples
#define FLASH_PLAY_END_POS    10000


static I2S_HandleTypeDef *i2sMic;
static I2S_HandleTypeDef *i2sDAC;

static int16_t audio[CLIP_SAMPLES];
static int16_t buffer[I2S_BUFFER_SIZE];

static volatile int16_t *bufferPtr = &buffer[0];
static volatile bool readyForData;

static volatile bool recording;
static volatile uint16_t sampleIdx;

static int8_t audioClip = 1;
static int16_t clipPos = 0;  // Offset into audio clip in samples for playback from Flash

// 1 - play from RAM, 2 - play from Flash
static int8_t audioType = 1;


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
  if (recording) {
    increment = 4;
  }

  if (audioType == 2 && !recording) {
    // Here we read the next chunk of audio clip data from Flash (enough to fill half the buffer)
    // The offset into the audio clip in bytes is given by clipPos (in samples) x 2
    // We read I2S_BUFFER_SIZE/2 bytes so I2S_BUFFER_SIZE/4 samples (samples are 16 bits each)
    // This is enough to fill half the buffer as each sample is duplicated for left and right channels
    flashReadDataOffset(audioClip - 1, (uint8_t *) audio, clipPos * 2, I2S_BUFFER_SIZE/2);
    clipPos += I2S_BUFFER_SIZE/4;
    if (clipPos > FLASH_PLAY_END_POS) {
      clipPos = FLASH_PLAY_START_POS;
    }
  }

  for (uint16_t i = 0; i < (I2S_BUFFER_SIZE/2) - 1; i += increment) {
    if (recording) {
      // Only MIC left channel has data so we skip the right channel
      // We also only store the top 16 bits of each 24 bit sample
      // This has the effect of a crude re-sample to 16 bits
      audio[sampleIdx] = bufferPtr[i];
    } else {
      // Send same sample to left and right channels
      sample = audio[sampleIdx];
      bufferPtr[i] = sample;
      bufferPtr[i + 1] = sample;
    }

    sampleIdx++;
    if (audioType == 1) {
      if (sampleIdx > 16000) {
        sampleIdx = 0;
        if (recording) {
          recording = false;
          HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
          // Stop recoding audio
          HAL_I2S_DMAStop(i2sMic);
        }
      }
    } else if (audioType == 2) {
      // Reset sampleIdx back to 0 on last iteration for Flash play
      // We always use the same I2S_BUFFER_SIZE/4 samples and replace these
      // on each call to processData
      // This could maybe be moved out of the loop
      if (sampleIdx >= (I2S_BUFFER_SIZE/4)) {
        sampleIdx = 0;
      }
    }
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
  audioType = 1;
  sampleIdx = 0;
  recording = true;
  HAL_I2S_Receive_DMA(i2sMic, (uint16_t *) buffer, I2S_BUFFER_SIZE/2);
}


void audioPlay(void)
{
  audioType = 1;
  sampleIdx = 0;
  HAL_I2S_Transmit_DMA(i2sDAC, (uint16_t *) buffer, I2S_BUFFER_SIZE);
}


void audioPlayFromFlash(void)
{
  audioType = 2;
  sampleIdx = 0;
  clipPos = FLASH_PLAY_START_POS*2;
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
  audioClip = audioClipNum;
}


uint8_t audioGetClipNum(void)
{
  return audioClip;
}


void audioStore(void)
{
  uint8_t blockIdx = audioClip - 1;

  flashEraseBlock(blockIdx);
  flashWriteData(blockIdx, (uint8_t *) audio, CLIP_SAMPLES*2);
}


void audioLoad(void)
{
  uint8_t blockIdx = audioClip - 1;

  flashReadData(blockIdx, (uint8_t *) audio, CLIP_SAMPLES*2);
}


int16_t * audioGetData(void)
{
  return audio;
}
