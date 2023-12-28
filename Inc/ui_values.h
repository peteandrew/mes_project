#ifndef __UI_VALUES_H
#define __UI_VALUES_H

enum {
  UI_CLIP           = 0x001,
  UI_CLIP_START     = 0x002,
  UI_CLIP_END       = 0x004,
  UI_CLIP_LOOP      = 0x008,
  UI_SEQ            = 0x010,
  UI_SEQ_CHANNEL    = 0x020,
  UI_SEQ_STEP       = 0x040,
  UI_SEQ_CLIP       = 0x080,
  UI_SEQ_CLIP_START = 0x100,
  UI_SEQ_CLIP_END   = 0x200,
  UI_AUDIO_RUNNING  = 0x400
};

typedef void (*uiChangeCallback)(int16_t);

#endif
