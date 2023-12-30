#include "application.h"
#include "main.h"
#include "console.h"
#include "flash.h"
#include "audio.h"
#include "sequence.h"
#include "st7789.h"
#include "ui_values.h"


// Step timer config
// BPM: 90
// beats per second: 1.5
// steps per second: 1.5 * 4 = 6
// seconds per tick: 0.167

// Timer clock: 96 MHz
// Prescaler: 65535
// Seconds per tick: 1/96 MHz * 65535 = 0.00068265625
// Ticks per beat = 0.666 / 0.000683 = 245


#define MENU_X_OFFSET 25
#define MENU_Y_OFFSET 45
#define MENU_ITEM_HEIGHT 25
#define MENU_MARKER_X 10


static TIM_HandleTypeDef *stepTimer;
static TIM_HandleTypeDef *encTimer;
static TIM_HandleTypeDef *inputTimer;
static volatile bool triggerStep = false;

void switchMainMenu(void);
void switchAudioClipMenu(void);
void switchAudioClipPlayMenu(void);
void switchAudioClipRecordMenu(void);
void switchSequenceMenu(void);
void switchSequenceEditMenu(void);

void toggleClipPlay(void);
void toggleSequencePlay(void);

void appRecordAudio(void);
void appPlayAudio(void);
void appStoreAudio(void);

typedef enum {
  ACTION      = 1,
  INT_VALUE   = 2,
  BOOL_VALUE  = 3
} itemTypeT;

typedef void (*actionFuncPtr)(void);

typedef struct {
  char*         label;
  itemTypeT     itemType;
  int16_t       uiValueType;
  bool          read_only;
  actionFuncPtr actionFunc;
} menuItemT;

typedef struct {
  char*       title;
  uint8_t     numItems;
  menuItemT   items[];
} menuT;

typedef enum {
  MAIN_MENU               = 0,
  AUDIO_CLIP_MENU         = 1,
  AUDIO_CLIP_PLAY_MENU    = 2,
  AUDIO_CLIP_RECORD_MENU  = 3,
  SEQUENCE_MENU           = 4,
  SEQUENCE_EDIT_MENU      = 5
} menuIndexT;

menuT mainMenu = {
  .title="Main Menu",
  .numItems=2,
  .items={
    {"Audio Clips", ACTION, 0, false, &switchAudioClipMenu},
    {"Sequences", ACTION, 0, false, &switchSequenceMenu}
  }
};

menuT audioClipMenu = {
    .title="Audio Clips",
    .numItems=3,
    .items={
        {"Play", ACTION, 0, false, &switchAudioClipPlayMenu},
        {"Record", ACTION, 0, false, &switchAudioClipRecordMenu},
        {"Back", ACTION, 0, false, &switchMainMenu}
    }
};

menuT audioClipPlayMenu = {
    .title="Play Clip",
    .numItems=6,
    .items={
        {"Clip", INT_VALUE, UI_CLIP, false, NULL},
        {"Play / stop", ACTION, 0, false, &toggleClipPlay},
        {"Start", INT_VALUE, UI_CLIP_START, false, NULL},
        {"End", INT_VALUE, UI_CLIP_END, false, NULL},
        {"Loop", BOOL_VALUE, UI_CLIP_LOOP, false, NULL},
        {"Back", ACTION, 0, false, &switchAudioClipMenu}
    }
};

menuT audioClipRecordMenu = {
    .title="Record Clip",
    .numItems=5,
    .items={
        {"Clip", INT_VALUE, UI_CLIP, false, NULL},
        {"Record", ACTION, 0, false, &appRecordAudio},
        {"Play", ACTION, 0, false, &appPlayAudio},
        {"Store", ACTION, 0, false, &appStoreAudio},
        {"Back", ACTION, 0, false, &switchAudioClipMenu}
    }
};

menuT sequenceMenu = {
  .title="Sequences",
  .numItems=4,
  .items={
      {"Sequence", INT_VALUE, UI_SEQ, false, NULL},
      {"Play / stop", ACTION, 0, false, &toggleSequencePlay},
      {"Edit", ACTION, 0, false, &switchSequenceEditMenu},
      {"Back", ACTION, 0, false, &switchMainMenu},
  }
};

menuT sequenceEditMenu = {
  .title="Sequence Edit",
  .numItems=6,
  .items={
      {"Channel", INT_VALUE, UI_SEQ_CHANNEL, false, NULL},
      {"Step", INT_VALUE, UI_SEQ_STEP, false, NULL},
      {"Clip", INT_VALUE, UI_SEQ_CLIP, false, NULL},
      {"Start", INT_VALUE, UI_SEQ_CLIP_START, false, NULL},
      {"End", INT_VALUE, UI_SEQ_CLIP_END, false, NULL},
      {"Back", ACTION, 0, false, &switchSequenceMenu},
  }
};

menuT *menus[] = {
    &mainMenu,
    &audioClipMenu,
    &audioClipPlayMenu,
    &audioClipRecordMenu,
    &sequenceMenu,
    &sequenceEditMenu
};

menuIndexT menuIdx = MAIN_MENU;
int8_t menuPos = 0;
int8_t itemSelectState = 0;
bool menuRenderRequired = true;
int16_t uiValuesChanged = 0;

volatile uint16_t previousCount;
volatile int16_t countChange = 0;
volatile bool tempButtonPressed = false;
volatile bool buttonPressed = false;
bool buttonActioned = false;

uint8_t sequence = 0;
uint8_t sequenceChannel = 0;
uint8_t sequenceStep = 0;
bool sequencePlaying = false;


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


uint16_t expnt(uint8_t power)
{
  // Exponent implementation from StackOverflow: https://stackoverflow.com/a/101613

  uint16_t base = 10;
  uint16_t result = 1;

  for (;;) {
    if (power & 1) {
      result *= base;
    }
    power >>= 1;
    if (!power) {
      break;
    }
    base *= base;
  }

  return result;
}

void uiValueChangeCB(int16_t valuesChanged)
{
  uiValuesChanged = valuesChanged;
}

uint16_t getUIValueInt(int16_t uiValueType)
{
  switch (uiValueType) {

  case UI_CLIP:
    return audioGetChannelParams(0).clipNum;
  case UI_CLIP_START:
    return audioGetChannelParams(0).startSample;
  case UI_CLIP_END:
    return audioGetChannelParams(0).endSample;
  case UI_SEQ:
    return sequence;
  case UI_SEQ_CHANNEL:
    return sequenceChannel;
  case UI_SEQ_STEP:
    return sequenceStep;
  case UI_SEQ_CLIP:
    return getStepChannelParams(sequenceChannel, sequenceStep).clipNum;
  case UI_SEQ_CLIP_START:
    return getStepChannelParams(sequenceChannel, sequenceStep).startSample;
  case UI_SEQ_CLIP_END:
    return getStepChannelParams(sequenceChannel, sequenceStep).endSample;
  }

  return 0;
}

bool getUIValueBool(int16_t uiValueType)
{
  switch (uiValueType) {
  case UI_CLIP_LOOP:
    return audioGetChannelParams(0).loop;
  }

  return false;
}

void uiAudioClipChange(int16_t changeAmt)
{
  ChannelParams_T params = audioGetChannelParams(0);
  if (changeAmt > 0 && params.clipNum + changeAmt > 50) {
    params.clipNum = 50;
  } else if (changeAmt < 0 && params.clipNum + changeAmt < 0) {
    params.clipNum = 0;
  } else {
    params.clipNum += changeAmt;
  }
  params.startSample = 0;
  params.endSample = CLIP_SAMPLES - 1;
  params.loop = false;
  audioSetChannelParams(0, params);
  if (menuIdx == AUDIO_CLIP_PLAY_MENU) {
    audioPlayFromFlash();
  }
  uiValueChangeCB(UI_CLIP | UI_CLIP_START | UI_CLIP_END | UI_CLIP_LOOP);
}

void uiAudioStartChange(int16_t changeAmt)
{
  ChannelParams_T params = audioGetChannelParams(0);
  if (changeAmt > 0 && params.startSample + changeAmt >= CLIP_SAMPLES - 2) {
    params.startSample = CLIP_SAMPLES - 2;
  } else if (changeAmt < 0 && params.startSample + changeAmt < 0) {
    params.startSample = 0;
  } else if (params.startSample + changeAmt > params.endSample) {
    params.startSample = params.endSample - 1;
  } else {
    params.startSample += changeAmt;
  }
  audioSetChannelParams(0, params);
  uiValueChangeCB(UI_CLIP_START);
}

void uiAudioEndChange(int16_t changeAmt)
{
  ChannelParams_T params = audioGetChannelParams(0);
  if (changeAmt > 0 && params.endSample + changeAmt >= CLIP_SAMPLES - 1) {
    params.endSample = CLIP_SAMPLES - 1;
  } else if (changeAmt < 0 && params.endSample + changeAmt < 0) {
    params.endSample = 0;
  } else if (params.endSample + changeAmt < params.startSample) {
    params.endSample = params.startSample + 1;
  } else {
    params.endSample += changeAmt;
  }
  audioSetChannelParams(0, params);
  uiValueChangeCB(UI_CLIP_END);
}

void uiAudioLoopToggle(void)
{
  ChannelParams_T params = audioGetChannelParams(0);
  params.loop = !params.loop;
  audioSetChannelParams(0, params);
  uiValueChangeCB(UI_CLIP_LOOP);
}

void uiSequenceChannelChange(int16_t changeAmt)
{
  if (changeAmt > 0 && sequenceChannel + changeAmt >= NUM_CHANNELS) {
    sequenceChannel = NUM_CHANNELS - 1;
  } else if (changeAmt < 0 && sequenceChannel + changeAmt < 0) {
    sequenceChannel = 0;
  } else {
    sequenceChannel += changeAmt;
  }
  uiValueChangeCB(UI_SEQ_CHANNEL | UI_SEQ_CLIP | UI_SEQ_CLIP_START | UI_SEQ_CLIP_END);
}

void uiSequenceStepChange(int16_t changeAmt)
{
  if (changeAmt > 0 && sequenceStep + changeAmt >= NUM_STEPS) {
    sequenceStep = NUM_STEPS - 1;
  } else if (changeAmt < 0 && sequenceStep + changeAmt < 0) {
    sequenceStep = 0;
  } else {
    sequenceStep += changeAmt;
  }
  uiValueChangeCB(UI_SEQ_STEP | UI_SEQ_CLIP | UI_SEQ_CLIP_START | UI_SEQ_CLIP_END);
}

void uiSequenceClipChange(int16_t changeAmt)
{
  ChannelParams_T params = getStepChannelParams(sequenceChannel, sequenceStep);
  if (changeAmt > 0 && params.clipNum + changeAmt > 50) {
    params.clipNum = 50;
  } else if (changeAmt < 0 && params.clipNum + changeAmt < 0) {
    params.clipNum = 0;
  } else {
    params.clipNum += changeAmt;
  }
  params.startSample = 0;
  params.endSample = CLIP_SAMPLES - 1;
  setStepChannelParams(sequenceChannel, sequenceStep, params);
  uiValueChangeCB(UI_SEQ_CLIP | UI_SEQ_CLIP_START | UI_SEQ_CLIP_END);
}

void uiSequenceStartChange(int16_t changeAmt)
{
  ChannelParams_T params = getStepChannelParams(sequenceChannel, sequenceStep);
  if (changeAmt > 0 && params.startSample + changeAmt >= CLIP_SAMPLES - 2) {
    params.startSample = CLIP_SAMPLES - 2;
  } else if (changeAmt < 0 && params.startSample + changeAmt < 0) {
    params.startSample = 0;
  } else if (params.startSample + changeAmt > params.endSample) {
    params.startSample = params.endSample - 1;
  } else {
    params.startSample += changeAmt;
  }
  setStepChannelParams(sequenceChannel, sequenceStep, params);
  uiValueChangeCB(UI_SEQ_CLIP_START);
}

void uiSequenceEndChange(int16_t changeAmt)
{
  ChannelParams_T params = getStepChannelParams(sequenceChannel, sequenceStep);
  if (changeAmt > 0 && params.endSample + changeAmt >= CLIP_SAMPLES - 1) {
    params.endSample = CLIP_SAMPLES - 1;
  } else if (changeAmt < 0 && params.endSample + changeAmt < 0) {
    params.endSample = 0;
  } else if (params.endSample + changeAmt < params.startSample) {
    params.endSample = params.startSample + 1;
  } else {
    params.endSample += changeAmt;
  }
  setStepChannelParams(sequenceChannel, sequenceStep, params);
  uiValueChangeCB(UI_SEQ_CLIP_END);
}


void renderMenuItem(uint8_t itemPos, const char *str, itemTypeT itemType, int16_t uiValueType, uint8_t selectState)
{
  uint16_t y = MENU_Y_OFFSET + itemPos * MENU_ITEM_HEIGHT;

  char valStr[] = "     ";

  switch (itemType) {
  case INT_VALUE:
    uint16_t val = getUIValueInt(uiValueType);

    uint8_t numDigits = 5;
    for (int i = 0; i < numDigits; i++) {
      uint16_t divisor = expnt(i);
      uint8_t digit = (val / divisor) % 10;
      valStr[numDigits - i - 1] = '0' + digit;
    }
    break;
  case BOOL_VALUE:
    if (getUIValueBool(uiValueType)) {
      valStr[0] = 'Y';
      valStr[1] = 'e';
      valStr[2] = 's';
    } else {
      valStr[0] = 'N';
      valStr[1] = 'o';
    }
    break;
  default:
    break;
  }


  ST7789_WriteString(MENU_X_OFFSET, y, str, Font_11x18, RED, WHITE);
  if (selectState == 1) {
    ST7789_WriteString(MENU_X_OFFSET + 130, y, valStr, Font_11x18, WHITE, RED);
  } else if (selectState == 2) {
    ST7789_WriteString(MENU_X_OFFSET + 130, y, valStr, Font_11x18, WHITE, GREEN);
  } else if (selectState == 3) {
    ST7789_WriteString(MENU_X_OFFSET + 130, y, valStr, Font_11x18, WHITE, BLUE);
  } else {
    ST7789_WriteString(MENU_X_OFFSET + 130, y, valStr, Font_11x18, RED, WHITE);
  }
}

void renderMenuMarker(uint8_t oldItemPos, uint8_t newItemPos)
{
  uint16_t y = MENU_Y_OFFSET + oldItemPos * MENU_ITEM_HEIGHT;
  ST7789_Fill(MENU_MARKER_X, y, MENU_MARKER_X + 12, y + MENU_ITEM_HEIGHT, WHITE);

  y = MENU_Y_OFFSET + newItemPos * MENU_ITEM_HEIGHT;
  ST7789_WriteChar(MENU_MARKER_X, y, '>', Font_11x18, BLACK, WHITE);
}

uint8_t simpleStrlen(char *str)
{
  uint8_t len = 0;
  while (*str != '\0') {
    len++;
    str++;
  }
  return len;
}

void uiUpdate(int16_t encCountChange, bool buttonPressed)
{
  menuT *currMenu = menus[menuIdx];

  if (menuRenderRequired) {
    ST7789_Fill_Color(WHITE);

    uint8_t titleLen = simpleStrlen(currMenu->title);
    uint8_t titleX = ((22 - titleLen) / 2) * 11;
    ST7789_WriteString(titleX, 5, currMenu->title, Font_11x18, BLUE, WHITE);

    for (int i = 0; i < currMenu->numItems; i++) {
      renderMenuItem(i, currMenu->items[i].label, currMenu->items[i].itemType, currMenu->items[i].uiValueType, false);
    }

    renderMenuMarker(menuPos, menuPos);

    menuRenderRequired = false;

    return;
  }

  int oldMenuPos = menuPos;

  if (itemSelectState == 0) {
    if (encCountChange > 0) {
      if (menuPos++ == currMenu->numItems - 1) {
        menuPos = currMenu->numItems - 1;
      }
    } else if (encCountChange < 0) {
      if (menuPos-- == 0) {
        menuPos = 0;
      }
    }
  } else if (encCountChange != 0) {
    int16_t valChange;
    if (itemSelectState == 1) {
      if (encCountChange > 0) {
        valChange = 1;
      } else {
        valChange = -1;
      }
    } else if (itemSelectState == 2) {
      valChange = encCountChange;
    } else {
      valChange = encCountChange * 100;
    }

    switch (currMenu->items[menuPos].uiValueType) {
    case UI_CLIP:
      uiAudioClipChange(valChange);
      break;
    case UI_CLIP_START:
      uiAudioStartChange(valChange);
      break;
    case UI_CLIP_END:
      uiAudioEndChange(valChange);
      break;
    case UI_SEQ_CHANNEL:
      uiSequenceChannelChange(valChange);
      break;
    case UI_SEQ_STEP:
      uiSequenceStepChange(valChange);
      break;
    case UI_SEQ_CLIP:
      uiSequenceClipChange(valChange);
      break;
    case UI_SEQ_CLIP_START:
      uiSequenceStartChange(valChange);
      break;
    case UI_SEQ_CLIP_END:
      uiSequenceEndChange(valChange);
      break;
    }
  }

  if (oldMenuPos != menuPos) {
    renderMenuMarker(oldMenuPos, menuPos);
  }

  if (buttonPressed) {
    if (!buttonActioned) {
      if (!currMenu->items[menuPos].read_only) {
        switch (currMenu->items[menuPos].itemType) {
          case INT_VALUE:
            if (++itemSelectState > 3) {
              itemSelectState = 0;
            }
            uiValuesChanged |= currMenu->items[menuPos].uiValueType;
            break;
          case BOOL_VALUE:
            if (currMenu->items[menuPos].uiValueType == UI_CLIP_LOOP) {
              uiAudioLoopToggle();
            }
            break;
          case ACTION:
            if (currMenu->items[menuPos].actionFunc) {
              currMenu->items[menuPos].actionFunc();
            }
            break;
        }
      }
      buttonActioned = true;
    }
  } else {
	buttonActioned = false;
  }

  if (uiValuesChanged) {
    int8_t currItemSelectState = 0;

    for (int i = 0; i < currMenu->numItems; i++) {
      if (uiValuesChanged & currMenu->items[i].uiValueType) {
        if (i == menuPos) {
          currItemSelectState = itemSelectState;
        } else {
          currItemSelectState = 0;
        }
        renderMenuItem(i, currMenu->items[i].label, currMenu->items[i].itemType, currMenu->items[i].uiValueType, currItemSelectState);
      }
    }

    uiValuesChanged = 0;
  }

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
  ST7789_Init();
  ST7789_Fill_Color(WHITE);
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


void switchToMenu(menuIndexT menuIdx_)
{
  menuIdx = menuIdx_;
  menuPos = 0;
  menuRenderRequired = true;
}


void switchMainMenu(void)
{
  appStopSequence();
  switchToMenu(MAIN_MENU);
}


void switchAudioClipMenu(void)
{
  audioStop();
  switchToMenu(AUDIO_CLIP_MENU);
}


void switchAudioClipPlayMenu(void)
{
  switchToMenu(AUDIO_CLIP_PLAY_MENU);
}


void switchAudioClipRecordMenu(void)
{
  switchToMenu(AUDIO_CLIP_RECORD_MENU);
}


void switchSequenceMenu(void)
{
  switchToMenu(SEQUENCE_MENU);
}


void switchSequenceEditMenu(void)
{
  switchToMenu(SEQUENCE_EDIT_MENU);
}


void toggleClipPlay(void)
{
  if (getAudioRunning()) {
    audioStop();
  } else {
    audioPlayFromFlash();
  }
}


void toggleSequencePlay(void)
{
  if (sequencePlaying) {
    appStopSequence();
  } else {
    appStartSequence();
  }
}
