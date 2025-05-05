/// 保证头文件只会被导入一次
/// gcc扩展语法
#pragma once

#include "driver/gpio.h"

#include "Utils.h"

/// INT中断引脚 P0, 按键按下时硬件会自动触发中断
#define KEYBOARD_INT GPIO_NUM_0
/// SCL时钟引脚 P1
#define I2C_SCL GPIO_NUM_1
/// SDA数据引脚 P2
#define I2C_SDA GPIO_NUM_2

// 使用 1ULL 的原因是为了确保位移操作的结果是一个 64 位无符号整数，避免潜在的溢出或类型转换问题。
// #define I2C_SCL_SEL (1ULL << I2C_SCL)
// #define I2C_SDA_SEL (1ULL << I2C_SDA)

/// 设置SDA引脚为输入方向
#define I2C_SDA_IN gpio_set_direction(I2C_SDA, GPIO_MODE_INPUT)
/// 设置SDA引脚为输出方向
#define I2C_SDA_OUT gpio_set_direction(I2C_SDA, GPIO_MODE_OUTPUT)

/// 拉高SCL引脚
#define I2C_SCL_H gpio_set_level(I2C_SCL, 1)
/// 拉低SCL引脚
#define I2C_SCL_L gpio_set_level(I2C_SCL, 0)

/// 拉高SDA引脚
#define I2C_SDA_H gpio_set_level(I2C_SDA, 1)
/// 拉低SDA引脚
#define I2C_SDA_L gpio_set_level(I2C_SDA, 0)

/// 读取SDA引脚电平的值
#define I2C_READ_SDA gpio_get_level(I2C_SDA)

/// SC12B的地址.原理图中ASEL这个引脚接的地
#define I2C_ADDR 0x42

void KEYBOARD_Init(void);
uint8_t KEYBOARD_ReadKey(void);
