#include "ui.h"
#include "ui_values.h"
#include "audio.h"
#include "application.h"
#include "sequence.h"
#include "st7789.h"
#include "stdbool.h"


#define MENU_X_OFFSET 25
#define MENU_Y_OFFSET 45
#define MENU_ITEM_HEIGHT 25
#define MENU_MARKER_X 10


static void switchMainMenu(void);
static void switchAudioClipMenu(void);
static void switchAudioClipPlayMenu(void);
static void switchAudioClipRecordMenu(void);
static void switchSequenceMenu(void);
static void switchSequenceEditMenu(void);

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

static menuT mainMenu = {
  .title="Main Menu",
  .numItems=2,
  .items={
    {"Audio Clips", ACTION, 0, false, &switchAudioClipMenu},
    {"Sequences", ACTION, 0, false, &switchSequenceMenu}
  }
};

static menuT audioClipMenu = {
    .title="Audio Clips",
    .numItems=3,
    .items={
        {"Play", ACTION, 0, false, &switchAudioClipPlayMenu},
        {"Record", ACTION, 0, false, &switchAudioClipRecordMenu},
        {"Back", ACTION, 0, false, &switchMainMenu}
    }
};

static menuT audioClipPlayMenu = {
    .title="Play Clip",
    .numItems=6,
    .items={
        {"Clip", INT_VALUE, UI_CLIP, false, NULL},
        {"Play / stop", ACTION, 0, false, &appToggleClipPlay},
        {"Start", INT_VALUE, UI_CLIP_START, false, NULL},
        {"End", INT_VALUE, UI_CLIP_END, false, NULL},
        {"Loop", BOOL_VALUE, UI_CLIP_LOOP, false, NULL},
        {"Back", ACTION, 0, false, &switchAudioClipMenu}
    }
};

static menuT audioClipRecordMenu = {
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

static menuT sequenceMenu = {
  .title="Sequences",
  .numItems=4,
  .items={
      {"Sequence", INT_VALUE, UI_SEQ, false, NULL},
      {"Play / stop", ACTION, 0, false, &appToggleSequencePlay},
      {"Edit", ACTION, 0, false, &switchSequenceEditMenu},
      {"Back", ACTION, 0, false, &switchMainMenu},
  }
};

static menuT sequenceEditMenu = {
  .title="Sequence Edit",
  .numItems=7,
  .items={
      {"Channel", INT_VALUE, UI_SEQ_CHANNEL, false, NULL},
      {"Step", INT_VALUE, UI_SEQ_STEP, false, NULL},
      {"Clip", INT_VALUE, UI_SEQ_CLIP, false, NULL},
      {"Start", INT_VALUE, UI_SEQ_CLIP_START, false, NULL},
      {"End", INT_VALUE, UI_SEQ_CLIP_END, false, NULL},
      {"Store", ACTION, 0, false, &appStoreSequence},
      {"Back", ACTION, 0, false, &switchSequenceMenu},
  }
};

static menuT *menus[] = {
    &mainMenu,
    &audioClipMenu,
    &audioClipPlayMenu,
    &audioClipRecordMenu,
    &sequenceMenu,
    &sequenceEditMenu
};

static menuIndexT menuIdx = MAIN_MENU;
static int8_t menuPos = 0;
static int8_t itemSelectState = 0;
static bool menuRenderRequired = true;
static int16_t uiValuesChanged = 0;
static bool buttonActioned = false;
static uint8_t sequenceChannel = 0;
static uint8_t sequenceStep = 0;


static uint16_t expnt(uint8_t power)
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


static uint16_t getUIValueInt(int16_t uiValueType)
{
  switch (uiValueType) {

  case UI_CLIP:
    return appGetAudioChannelParams(0).clipNum;
  case UI_CLIP_START:
    return appGetAudioChannelParams(0).startSample;
  case UI_CLIP_END:
    return appGetAudioChannelParams(0).endSample;
  case UI_SEQ:
    return appGetSequenceNum();
  case UI_SEQ_CHANNEL:
    return sequenceChannel;
  case UI_SEQ_STEP:
    return sequenceStep;
  case UI_SEQ_CLIP:
    return appGetSequenceStepChannelParams(sequenceStep, sequenceChannel).clipNum;
  case UI_SEQ_CLIP_START:
    return appGetSequenceStepChannelParams(sequenceStep, sequenceChannel).startSample;
  case UI_SEQ_CLIP_END:
    return appGetSequenceStepChannelParams(sequenceStep, sequenceChannel).endSample;
  }

  return 0;
}

static bool getUIValueBool(int16_t uiValueType)
{
  switch (uiValueType) {
  case UI_CLIP_LOOP:
    return appGetAudioChannelParams(0).loop;
  }

  return false;
}

static void uiAudioClipChange(int16_t changeAmt)
{
  ChannelParams_T params = appGetAudioChannelParams(0);
  if (changeAmt > 0 && params.clipNum + changeAmt > NUM_CLIPS) {
    params.clipNum = NUM_CLIPS;
  } else if (changeAmt < 0 && params.clipNum + changeAmt < 1) {
    params.clipNum = 1;
  } else {
    params.clipNum += changeAmt;
  }
  params.startSample = 0;
  params.endSample = MAX_SAMPLE_IDX;
  params.loop = false;
  appSetAudioChannelParams(0, params);
  if (menuIdx == AUDIO_CLIP_PLAY_MENU) {
    appPlayAudioFromFlash();
  }
  uiValueChangeCB(UI_CLIP | UI_CLIP_START | UI_CLIP_END | UI_CLIP_LOOP);
}


static void uiAudioStartChange(int16_t changeAmt)
{
  ChannelParams_T params = appGetAudioChannelParams(0);
  if (changeAmt > 0 && params.startSample + changeAmt > MAX_SAMPLE_IDX - 1) {
    params.startSample = MAX_SAMPLE_IDX - 1;
  } else if (changeAmt < 0 && params.startSample + changeAmt < 0) {
    params.startSample = 0;
  } else if (params.startSample + changeAmt > params.endSample) {
    params.startSample = params.endSample - 1;
  } else {
    params.startSample += changeAmt;
  }
  appSetAudioChannelParams(0, params);
  uiValueChangeCB(UI_CLIP_START);
}


static void uiAudioEndChange(int16_t changeAmt)
{
  ChannelParams_T params = appGetAudioChannelParams(0);
  if (changeAmt > 0 && params.endSample + changeAmt > MAX_SAMPLE_IDX) {
    params.endSample = MAX_SAMPLE_IDX;
  } else if (changeAmt < 0 && params.endSample + changeAmt < 0) {
    params.endSample = 0;
  } else if (params.endSample + changeAmt < params.startSample) {
    params.endSample = params.startSample + 1;
  } else {
    params.endSample += changeAmt;
  }
  appSetAudioChannelParams(0, params);
  uiValueChangeCB(UI_CLIP_END);
}


static void uiAudioLoopToggle(void)
{
  ChannelParams_T params = appGetAudioChannelParams(0);
  params.loop = !params.loop;
  appSetAudioChannelParams(0, params);
  uiValueChangeCB(UI_CLIP_LOOP);
}


static void uiSequenceChange(int16_t changeAmt)
{
  uint8_t sequenceNum = appGetSequenceNum();
  if (changeAmt > 0 && sequenceNum + changeAmt > NUM_SEQUENCES) {
    sequenceNum = NUM_SEQUENCES;
  } else if (changeAmt < 0 && sequenceNum + changeAmt < 1) {
    sequenceNum = 1;
  } else {
    sequenceNum += changeAmt;
  }
  appSetSequenceNum(sequenceNum);
}


static void uiSequenceChannelChange(int16_t changeAmt)
{
  if (changeAmt > 0 && sequenceChannel + changeAmt > MAX_CHANNEL_IDX) {
    sequenceChannel = MAX_CHANNEL_IDX;
  } else if (changeAmt < 0 && sequenceChannel + changeAmt < 0) {
    sequenceChannel = 0;
  } else {
    sequenceChannel += changeAmt;
  }
  uiValueChangeCB(UI_SEQ_CHANNEL | UI_SEQ_CLIP | UI_SEQ_CLIP_START | UI_SEQ_CLIP_END);
}


static void uiSequenceStepChange(int16_t changeAmt)
{
  if (changeAmt > 0 && sequenceStep + changeAmt > MAX_STEP_IDX) {
    sequenceStep = MAX_STEP_IDX;
  } else if (changeAmt < 0 && sequenceStep + changeAmt < 0) {
    sequenceStep = 0;
  } else {
    sequenceStep += changeAmt;
  }
  uiValueChangeCB(UI_SEQ_STEP | UI_SEQ_CLIP | UI_SEQ_CLIP_START | UI_SEQ_CLIP_END);
}


static void uiSequenceClipChange(int16_t changeAmt)
{
  ChannelParams_T params = appGetSequenceStepChannelParams(sequenceStep, sequenceChannel);
  if (changeAmt > 0 && params.clipNum + changeAmt > NUM_CLIPS) {
    params.clipNum = NUM_CLIPS;
  } else if (changeAmt < 0 && params.clipNum + changeAmt < 0) {
    params.clipNum = 0;
  } else {
    params.clipNum += changeAmt;
  }
  params.startSample = 0;
  params.endSample = MAX_SAMPLE_IDX;
  appSetSequenceStepChannelParams(sequenceStep, sequenceChannel, params);
  uiValueChangeCB(UI_SEQ_CLIP | UI_SEQ_CLIP_START | UI_SEQ_CLIP_END);
}


static void uiSequenceStartChange(int16_t changeAmt)
{
  ChannelParams_T params = appGetSequenceStepChannelParams(sequenceStep, sequenceChannel);
  if (changeAmt > 0 && params.startSample + changeAmt > MAX_SAMPLE_IDX - 1) {
    params.startSample = MAX_SAMPLE_IDX - 1;
  } else if (changeAmt < 0 && params.startSample + changeAmt < 0) {
    params.startSample = 0;
  } else if (params.startSample + changeAmt > params.endSample) {
    params.startSample = params.endSample - 1;
  } else {
    params.startSample += changeAmt;
  }
  appSetSequenceStepChannelParams(sequenceStep, sequenceChannel, params);
  uiValueChangeCB(UI_SEQ_CLIP_START);
}


static void uiSequenceEndChange(int16_t changeAmt)
{
  ChannelParams_T params = appGetSequenceStepChannelParams(sequenceStep, sequenceChannel);
  if (changeAmt > 0 && params.endSample + changeAmt > MAX_SAMPLE_IDX) {
    params.endSample = MAX_SAMPLE_IDX;
  } else if (changeAmt < 0 && params.endSample + changeAmt < 0) {
    params.endSample = 0;
  } else if (params.endSample + changeAmt < params.startSample) {
    params.endSample = params.startSample + 1;
  } else {
    params.endSample += changeAmt;
  }
  appSetSequenceStepChannelParams(sequenceStep, sequenceChannel, params);
  uiValueChangeCB(UI_SEQ_CLIP_END);
}


static void renderMenuItem(uint8_t itemPos, const char *str, itemTypeT itemType, int16_t uiValueType, uint8_t selectState)
{
  uint16_t y = MENU_Y_OFFSET + itemPos * MENU_ITEM_HEIGHT;

  char valStr[] = "      ";

  switch (itemType) {
  case INT_VALUE:
    uint16_t val = getUIValueInt(uiValueType);

    uint8_t numDigits = 5;
    for (int i = 0; i < numDigits; i++) {
      uint16_t divisor = expnt(i);
      uint8_t digit = (val / divisor) % 10;
      valStr[numDigits - i - 1] = '0' + digit;
    }

    // FIXME: This doesn't really belong here.
    // This function shouldn't know about special conditions for different value types.
    // Perhaps a pointer to the function to call should be passed in.
    if (uiValueType == UI_CLIP && appGetAudioClipUsed((uint8_t) val)) {
	valStr[5] = '*';
    } else if (uiValueType == UI_SEQ && appGetSequenceUsed()) {
	valStr[5] = '*';
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

static void renderMenuMarker(uint8_t oldItemPos, uint8_t newItemPos)
{
  uint16_t y = MENU_Y_OFFSET + oldItemPos * MENU_ITEM_HEIGHT;
  ST7789_Fill(MENU_MARKER_X, y, MENU_MARKER_X + 12, y + MENU_ITEM_HEIGHT, WHITE);

  y = MENU_Y_OFFSET + newItemPos * MENU_ITEM_HEIGHT;
  ST7789_WriteChar(MENU_MARKER_X, y, '>', Font_11x18, BLACK, WHITE);
}


static uint8_t simpleStrlen(char *str)
{
  uint8_t len = 0;
  while (*str != '\0') {
    len++;
    str++;
  }
  return len;
}


static void switchToMenu(menuIndexT menuIdx_)
{
  menuIdx = menuIdx_;
  menuPos = 0;
  menuRenderRequired = true;
}


static void switchMainMenu(void)
{
  appStopSequence();
  switchToMenu(MAIN_MENU);
}


static void switchAudioClipMenu(void)
{
  appStopAudio();
  switchToMenu(AUDIO_CLIP_MENU);
}


static void switchAudioClipPlayMenu(void)
{
  switchToMenu(AUDIO_CLIP_PLAY_MENU);
}


static void switchAudioClipRecordMenu(void)
{
  switchToMenu(AUDIO_CLIP_RECORD_MENU);
}


static void switchSequenceMenu(void)
{
  switchToMenu(SEQUENCE_MENU);
}


static void switchSequenceEditMenu(void)
{
  switchToMenu(SEQUENCE_EDIT_MENU);
}


void uiInit(void)
{
  ST7789_Init();
  ST7789_Fill_Color(WHITE);
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
    case UI_SEQ:
      uiSequenceChange(valChange);
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


void uiValueChangeCB(int16_t valuesChanged)
{
  uiValuesChanged = valuesChanged;
}
