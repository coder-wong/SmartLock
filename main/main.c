#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "KEYBORAD_Driver.h"
#include "MOTOR_Driver.h"
#include "AUDIO_Driver.h"
#include "LED_Driver.h"
#include "FINGER_Driver.h"
#include "BT_Driver.h"
#include "WIFI_Driver.h"
#include "PASSWORD_Driver.h"
#include "OTA_Driver.h"

#include "freertos/idf_additions.h"
#include "nvs_flash.h"

static uint8_t password_count = 0;
uint8_t is_enroll_flag = 0;
static uint8_t is_finger_on = 0;
static uint8_t password_array[6] = {0};
static uint8_t is_ota_flag = 0;

QueueHandle_t gpio_event_queue = NULL; // 写在最外面，其他函数才能用
// 中断服务函数,与中断引脚绑定在一起，不是rtos任务
void gpio_isr_handler(void *arg)
{
	uint32_t gpio_num = (uint32_t)arg;

	if (gpio_num == FINGER_TOUCH_INT) 
	{
		is_finger_on = 1; // 指纹传感器触发中断
		// 消抖，防止放指纹时触发多个中断
		gpio_isr_handler_remove(FINGER_TOUCH_INT);
		gpio_isr_handler_remove(KEYBOARD_INT);
	}

	xQueueSendFromISR(gpio_event_queue, &gpio_num, NULL);
}

// rtos任务：轮询是否录入指纹
void enroll_fingerprint_task(void *arg)
{
	while (1)
	{
		// 轮询是否有指纹需要录入
		if (is_enroll_flag)
		{
			AUDIO_Play(80);		  // 播放“请放手指”
			while (!is_finger_on) // 判断是否有手指放上，有放上，就执行后面的程序
			{
				printf("没有检测到指纹\r\n");
				DelayMs(10);
			}

			uint8_t templates_count = FINGER_GetTemplatesNumber();
			FINGER_Enroll(templates_count + 1); // +1是因为要在前面存在的指纹基础上进入录入

			// 录入完成
			is_enroll_flag = 0;
			is_finger_on = 0;
			FINGER_Cancel();
			while (FINGER_Sleep() == 1)
				;

			gpio_isr_handler_add(KEYBOARD_INT, gpio_isr_handler,
								 (void *)KEYBOARD_INT);
			gpio_isr_handler_add(FINGER_TOUCH_INT, gpio_isr_handler,
								 (void *)FINGER_TOUCH_INT);
		}
		DelayMs(200);
	}
}
// rtos任务：轮询是否识别指纹
void identify_fingerprint_task(void *arg)
{
	while (1)
	{
		// 轮询是否有指纹按下，并且不是录入指纹
		if (!is_enroll_flag && is_finger_on)
		{
			if (FINGER_Identify() == 0)
			{
				printf("解锁成功\n");
				AUDIO_Play(85);
				MOTOR_OpenLock(); // 开锁成功
			}
			else
			{
				printf("解锁失败\n");
				AUDIO_Play(86);
			}

			is_finger_on = 0;
			FINGER_Cancel();
			while (FINGER_Sleep() == 1)
				;
			gpio_isr_handler_add(KEYBOARD_INT, gpio_isr_handler,
								 (void *)KEYBOARD_INT);
			gpio_isr_handler_add(FINGER_TOUCH_INT, gpio_isr_handler,
								 (void *)FINGER_TOUCH_INT);
		}
		DelayMs(200);
	}
}
// rtos任务：不断轮询is_ota_flag标志位
void ota_task(void *args)
{
	while (1)
	{
		if (is_ota_flag)
		{
			is_ota_flag = 0;
			OTA_Start();
		}
		DelayMs(5000);
	}
}

// rots任务：不断轮询中断队列
void process_isr(void *arg)
{
	uint32_t gpio_num;
	while (1)
	{
		// 轮询中断队列,知道有数据，不然就一直阻塞
		if (xQueueReceive(gpio_event_queue, &gpio_num, portMAX_DELAY)) // 读了之后会自动删除数据
		{
			if (gpio_num == KEYBOARD_INT)
			{
				uint8_t key = KEYBOARD_ReadKey();
				printf("key = %d\n", key);

				// 按键灯光控制
				if (key == 'M')
					LED_Light(11);
				else if (key == '#')
					LED_Light(10);
				else
					LED_Light(key);

				// 将输入的的按键保存到数组中
				password_array[password_count++] = key;
				// 判断是否录入指纹
				if (password_count >= 3 && password_array[password_count - 1] == 'M' && password_array[password_count - 2] == 'M' && password_array[password_count - 3] == 'M')
				{
					password_count = 0;
					memset(password_array, 0, 6);
					is_enroll_flag = 1;
				}
				// 判断是否OTA
				if (password_count >= 3 && password_array[password_count - 1] == 'M' && password_array[password_count - 2] == '#' && password_array[password_count - 3] == 'M')
				{
					is_ota_flag = 1;
				}
				// 按键解锁
				if (password_count == 6)
				{
					nvs_handle password_handle;
					nvs_open("namespace", NVS_READONLY, &password_handle);
					char password_in_flash[7] = {0};
					uint8_t len = 7;
					nvs_get_str(password_handle, "password", password_in_flash, &len);
					nvs_close(password_handle);
					uint8_t i = 0;
					for (i = 0; i < 6; i++)
					{
						if (password_array[i] != password_in_flash[i] - '0')
							break; // 如果密码不相等，跳出循环
					}
					if (i == 6)
					{
						MOTOR_OpenLock();
					}
					if (i < 6) // 输入6位密码后，如果与flash中的永久密码不匹配，就验证临时密码
					{
						if (PASSWORD_ValidateTempPassword(password_array[0] * 100000 + password_array[1] * 10000 + password_array[2] * 1000 + password_array[3] * 100 + password_array[4] * 10 + password_array[5])) // 验证临时密码
						{
							MOTOR_OpenLock();
						}
					}
					password_count = 0;
					memset(password_array, 0, 6);
				}
			}
		}
	}
}
// rtos任务：不断轮询is_enroll_flag标志位
void app_main(void)
{
	KEYBOARD_Init();
	MOTOR_Init();
	AUDIO_Init();
	LED_Init();
	FINGER_Init();

	// 初始化NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	BT_Init();
	WIFI_Init(); // 连上WIFI后，就同步了实时时钟
	// FINGER_GetSerialNumber();
	// FINGER_SetSecurityLevelAsZero();
	// FINGER_GetTemplatesNumber();
	// FINGER_Cancel();
	// while (FINGER_Sleep() == 1)
	// 一直循环直接休眠成功

	/* 按键检测相关 */
	// 创建中断队列
	gpio_event_queue = xQueueCreate(10, sizeof(uint32_t));
	// 启动中断
	gpio_install_isr_service(0);
	// 绑定中断引脚和中断回调函数
	gpio_isr_handler_add(KEYBOARD_INT, gpio_isr_handler, (void *)KEYBOARD_INT);
	gpio_isr_handler_add(FINGER_TOUCH_INT, gpio_isr_handler, (void *)KEYBOARD_INT);
	// 将process_isr注册为freertos任务，开始轮询中断队列gpio_event_queue
	xTaskCreate(process_isr, "process_isr", 2048, NULL, 10, NULL);
	xTaskCreate(enroll_fingerprint_task, "enroll_fingerprint_task", 2048, NULL, 10, NULL);
	xTaskCreate(identify_fingerprint_task, "identify_fingerprint_task", 2048, NULL, 10, NULL);
	xTaskCreate(ota_task, "ota_task", 8192, NULL, 10, NULL);
	/* 电机相关*/
	// MOTOR_OpenLock();

	/* 音频相关 */
	// AUDIO_Play(14);
}