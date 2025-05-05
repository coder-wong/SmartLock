#pragma once

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "unistd.h"

#define DelayMs(time) vTaskDelay(time / portTICK_PERIOD_MS)
#define DelayUs(time) usleep(time)
