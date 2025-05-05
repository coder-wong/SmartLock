#pragma once

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include <stdint.h>
#include "driver/rmt_encoder.h"
#include "esp_check.h"

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      6

/// led灯珠的数量
#define EXAMPLE_LED_NUMBERS         12
#define EXAMPLE_CHASE_SPEED_MS      100

void LED_Init(void);
void LED_Light(uint8_t led_number);