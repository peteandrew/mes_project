#ifndef __UI_H
#define __UI_H

#include <stdint.h>
#include <stdbool.h>

void uiInit(void);
void uiUpdate(int16_t encCountChange, bool buttonPressed);
void uiValueChangeCB(int16_t valuesChanged);

#endif
