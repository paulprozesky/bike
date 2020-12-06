#ifndef __USER_H
#define __USER_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

void writeSdCardDemo();
void tickLedOn();
void tickLedOff();
void blinkUserLed(const uint32_t target);

#ifdef __cplusplus
}
#endif

#endif // __USER_H
