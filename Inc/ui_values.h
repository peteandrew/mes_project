#ifndef __UI_VALUES_H
#define __UI_VALUES_H

enum {
  UI_CHANNEL = 0x01,
  UI_STEP = 0x02,
  UI_CLIP = 0x04,
  UI_START = 0x08,
  UI_END  = 0x10,
};

typedef void (*uiChangeCallback)(int8_t);

#endif
