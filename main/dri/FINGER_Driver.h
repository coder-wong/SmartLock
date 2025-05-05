#pragma once
#include "stdint.h"

#define UART_TX_PIN 21
#define UART_RX_PIN 20
#define FINGER_TOUCH_INT 10
// 初始化指纹模块
void FINGER_Init(void);
/// 获取指纹唯一的序列号
void FINGER_GetSerialNumber(void);
/// 获取模板数量
uint8_t FINGER_GetTemplatesNumber(void);
/// 指纹休眠指令
uint8_t FINGER_Sleep(void);
/* 取消指令
 *  - 取消验证指纹操作
 *  - 取消录入指纹操作
 */
void FINGER_Cancel(void);

void FINGER_SetSecurityLevelAsZero(void);

// 录入指纹
// PageID:表示将指纹录入到哪一页
uint8_t FINGER_Enroll(uint8_t PageID);
// 验证指纹
uint8_t FINGER_Identify(void);
