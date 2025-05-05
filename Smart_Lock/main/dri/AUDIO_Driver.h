#pragma once 
#include "stdint.h"

#define AUDIO_DATA_PIN GPIO_NUM_9

void AUDIO_Init(void);

void AUDIO_Play(uint8_t data);
