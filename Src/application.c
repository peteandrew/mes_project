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
static volatile bool pollInput = false;

void toggleSequencePlay(void);

typedef enum {
  ACTION      = 1,
  INT_VALUE   = 2,
  BOOL_VALUE  = 3
} itemTypeT;

typedef void (*actionFuncPtr)(void);

typedef struct {
  char*         label;
  itemTypeT     itemType;
  int8_t        ui_value_type;
  bool          read_only;
  actionFuncPtr actionFunc;
} menuItemT;

typedef struct {
  char*       title;
  uint8_t     numItems;
  menuItemT   items[];
} menuT;

menuT mainMenu = {
  .title="Main menu",
  .numItems=6,
  .items={
      {"Channel", INT_VALUE, UI_CHANNEL, false, NULL},
      {"Step", INT_VALUE, UI_STEP, false, NULL},
      {"Clip", INT_VALUE, UI_CLIP, false, NULL},
      {"Start", INT_VALUE, UI_START, false, NULL},
      {"End", INT_VALUE, UI_END, false, NULL},
      {"Play / stop", ACTION, 0, false, &toggleSequencePlay}
  }
};

menuT *menus[] = {
    &mainMenu
};

uint8_t menuIdx = 0;
int8_t menuPos = 0;
int8_t itemSelectState = 0;
bool menuRenderRequired = true;
int8_t uiValuesChanged = 0;

uint16_t previousCount;
int16_t countChange = 0;
bool buttonPressed = false;
bool buttonActioned = false;

uint8_t selectedChannel = 0;
uint8_t selectedStep = 0;
bool sequencePlaying = false;


void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim)
{
  if (htim->Instance == inputTimer->Instance) {
    pollInput = true;
  }
  if (htim->Instance == stepTimer->Instance)
  {
    triggerStep = true;
  }
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

void uiValueChangeCB(int8_t valuesChanged)
{
  uiValuesChanged = valuesChanged;
}

uint16_t getUIValueInt(int8_t uiValueType)
{
  switch (uiValueType) {
  case UI_CHANNEL:
    return selectedChannel;
  case UI_STEP:
    return selectedStep;
  case UI_CLIP:
    return getStepChannelParams(selectedChannel, selectedStep).clipNum;
  case UI_START:
    return getStepChannelParams(selectedChannel, selectedStep).startSample;
  case UI_END:
    return getStepChannelParams(selectedChannel, selectedStep).endSample;
  }

  return 0;
}

bool getUIValueBool(int8_t uiValueType)
{
  /*switch (uiValueType) {
  case VAL_4:
    return get_val_4();
  }*/

  return false;
}

void selectedChannelChange(int16_t changeAmt)
{
  if (changeAmt > 0 && selectedChannel + changeAmt >= NUM_CHANNELS) {
    selectedChannel = NUM_CHANNELS - 1;
  } else if (changeAmt < 0 && selectedChannel + changeAmt < 0) {
    selectedChannel = 0;
  } else {
    selectedChannel += changeAmt;
  }
  uiValueChangeCB(UI_CHANNEL | UI_CLIP | UI_START | UI_END);
}

void selectedStepChange(int16_t changeAmt)
{
  if (changeAmt > 0 && selectedStep + changeAmt >= NUM_STEPS) {
    selectedStep = NUM_STEPS - 1;
  } else if (changeAmt < 0 && selectedStep + changeAmt < 0) {
    selectedStep = 0;
  } else {
    selectedStep += changeAmt;
  }
  uiValueChangeCB(UI_STEP | UI_CLIP | UI_START | UI_END);
}

void clipChange(int16_t changeAmt)
{
  ChannelParams_T params = getStepChannelParams(selectedChannel, selectedStep);
  if (changeAmt > 0 && params.clipNum + changeAmt > 50) {
    params.clipNum = 50;
  } else if (changeAmt < 0 && params.clipNum + changeAmt < 0) {
    params.clipNum = 0;
  } else {
    params.clipNum += changeAmt;
  }
  params.startSample = 0;
  params.endSample = CLIP_SAMPLES;
  setStepChannelParams(selectedChannel, selectedStep, params);
  uiValueChangeCB(UI_CLIP | UI_START | UI_END);
}

void startChange(int16_t changeAmt)
{
  ChannelParams_T params = getStepChannelParams(selectedChannel, selectedStep);
  if (changeAmt > 0 && params.startSample + changeAmt > CLIP_SAMPLES) {
    params.startSample = CLIP_SAMPLES;
  } else if (changeAmt < 0 && params.startSample + changeAmt < 0) {
    params.startSample = 0;
  } else if (params.startSample + changeAmt > params.endSample) {
    params.startSample = params.endSample - 1;
  } else {
    params.startSample += changeAmt;
  }
  setStepChannelParams(selectedChannel, selectedStep, params);
  uiValueChangeCB(UI_START);
}

void endChange(int16_t changeAmt)
{
  ChannelParams_T params = getStepChannelParams(selectedChannel, selectedStep);
  if (changeAmt > 0 && params.endSample + changeAmt > CLIP_SAMPLES) {
    params.endSample = CLIP_SAMPLES;
  } else if (changeAmt < 0 && params.endSample + changeAmt < 0) {
    params.endSample = 0;
  } else if (params.endSample + changeAmt < params.startSample) {
    params.endSample = params.startSample + 1;
  } else {
    params.endSample += changeAmt;
  }
  setStepChannelParams(selectedChannel, selectedStep, params);
  uiValueChangeCB(UI_END);
}


void renderMenuItem(uint8_t itemPos, const char *str, itemTypeT itemType, int8_t uiValueType, uint8_t selectState)
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

bool uiUpdate(int16_t encCountChange, bool buttonPressed)
{
  bool buttonActioned = false;
  menuT *currMenu = menus[menuIdx];

  if (menuRenderRequired) {
    ST7789_Fill_Color(WHITE);

    //menuItemT menu[] = menus[menuIdx];
    ST7789_WriteString(90, 5, currMenu->title, Font_11x18, BLUE, WHITE);

    for (int i = 0; i < currMenu->numItems; i++) {
      renderMenuItem(i, currMenu->items[i].label, currMenu->items[i].itemType, currMenu->items[i].ui_value_type, false);
    }

    renderMenuMarker(menuPos, menuPos);

    menuRenderRequired = false;

    return buttonActioned;
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

    switch (currMenu->items[menuPos].ui_value_type) {
    case UI_CHANNEL:
      selectedChannelChange(valChange);
      break;
    case UI_STEP:
      selectedStepChange(valChange);
      break;
    case UI_CLIP:
      clipChange(valChange);
      break;
    case UI_START:
      startChange(valChange);
      break;
    case UI_END:
      endChange(valChange);
      break;
    }
  }

  if (oldMenuPos != menuPos) {
    renderMenuMarker(oldMenuPos, menuPos);
  }

  if (buttonPressed) {
    if (!currMenu->items[menuPos].read_only) {
      switch (currMenu->items[menuPos].itemType) {
      case INT_VALUE:
        if (++itemSelectState > 3) {
          itemSelectState = 0;
        }
        uiValuesChanged |= currMenu->items[menuPos].ui_value_type;
        break;
      case BOOL_VALUE:
        /*if (currMenu->items[menuPos].ui_value_type == VAL_4) {
          val_4_toggle();
        }*/
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

  if (uiValuesChanged) {
    int8_t currItemSelectState = 0;

    for (int i = 0; i < currMenu->numItems; i++) {
      if (uiValuesChanged & currMenu->items[i].ui_value_type) {
        if (i == menuPos) {
          currItemSelectState = itemSelectState;
        } else {
          currItemSelectState = 0;
        }
        renderMenuItem(i, currMenu->items[i].label, currMenu->items[i].itemType, currMenu->items[i].ui_value_type, currItemSelectState);
      }
    }

    uiValuesChanged = 0;
  }

  return buttonActioned;
}


void appInit(I2S_HandleTypeDef *i2sMicH, I2S_HandleTypeDef *i2sDACH, SPI_HandleTypeDef *spiFlashH, TIM_HandleTypeDef *stepTimerH, TIM_HandleTypeDef *encTimerH, TIM_HandleTypeDef *inputTimerH)
{
  ConsoleInit();
  flashInit(spiFlashH);
  audioInit(i2sMicH, i2sDACH, &uiValueChangeCB);
  sequenceInit(&uiValueChangeCB);
  stepTimer = stepTimerH;
  encTimer = encTimerH;
  HAL_TIM_Encoder_Start(encTimerH, TIM_CHANNEL_ALL);
  previousCount = encTimerH->Instance->CNT = 32767;
  ST7789_Init();
  ST7789_Fill_Color(WHITE);
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
  inputTimer = inputTimerH;
  HAL_TIM_Base_Start_IT(inputTimer);
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

    if (pollInput) {
      countChange = encTimer->Instance->CNT - previousCount;
      previousCount = encTimer->Instance->CNT = 32767;
      buttonPressed = HAL_GPIO_ReadPin(ENC_BUTTON_GPIO_Port, ENC_BUTTON_Pin) == GPIO_PIN_RESET;
      if (buttonPressed) {
        if (buttonActioned) {
          buttonPressed = false;
        }
      } else {
        buttonActioned = false;
      }

      pollInput = false;

      bool uiButtonActioned = uiUpdate(countChange, buttonPressed);
      if (uiButtonActioned) {
        buttonActioned = true;
      }
    }

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


void toggleSequencePlay(void)
{
  if (sequencePlaying) {
    appStopSequence();
  } else {
    appStartSequence();
  }
}


void appSetSequenceStepChannelParams(uint8_t stepIdx, uint8_t channelIdx, ChannelParams_T params)
{
  setStepChannelParams(channelIdx, stepIdx, params);
}


ChannelParams_T appGetSequenceStepChannelParams(uint8_t stepIdx, uint8_t channelIdx)
{
  return getStepChannelParams(channelIdx, stepIdx);
}
