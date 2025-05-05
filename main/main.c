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

QueueHandle_t gpio_event_queue = NULL; // д�������棬��������������
// �жϷ�����,���ж����Ű���һ�𣬲���rtos����
void gpio_isr_handler(void *arg)
{
	uint32_t gpio_num = (uint32_t)arg;

	if (gpio_num == FINGER_TOUCH_INT) 
	{
		is_finger_on = 1; // ָ�ƴ����������ж�
		// ��������ֹ��ָ��ʱ��������ж�
		gpio_isr_handler_remove(FINGER_TOUCH_INT);
		gpio_isr_handler_remove(KEYBOARD_INT);
	}

	xQueueSendFromISR(gpio_event_queue, &gpio_num, NULL);
}

// rtos������ѯ�Ƿ�¼��ָ��
void enroll_fingerprint_task(void *arg)
{
	while (1)
	{
		// ��ѯ�Ƿ���ָ����Ҫ¼��
		if (is_enroll_flag)
		{
			AUDIO_Play(80);		  // ���š������ָ��
			while (!is_finger_on) // �ж��Ƿ�����ָ���ϣ��з��ϣ���ִ�к���ĳ���
			{
				printf("û�м�⵽ָ��\r\n");
				DelayMs(10);
			}

			uint8_t templates_count = FINGER_GetTemplatesNumber();
			FINGER_Enroll(templates_count + 1); // +1����ΪҪ��ǰ����ڵ�ָ�ƻ����Ͻ���¼��

			// ¼�����
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
// rtos������ѯ�Ƿ�ʶ��ָ��
void identify_fingerprint_task(void *arg)
{
	while (1)
	{
		// ��ѯ�Ƿ���ָ�ư��£����Ҳ���¼��ָ��
		if (!is_enroll_flag && is_finger_on)
		{
			if (FINGER_Identify() == 0)
			{
				printf("�����ɹ�\n");
				AUDIO_Play(85);
				MOTOR_OpenLock(); // �����ɹ�
			}
			else
			{
				printf("����ʧ��\n");
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
// rtos���񣺲�����ѯis_ota_flag��־λ
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

// rots���񣺲�����ѯ�ж϶���
void process_isr(void *arg)
{
	uint32_t gpio_num;
	while (1)
	{
		// ��ѯ�ж϶���,֪�������ݣ���Ȼ��һֱ����
		if (xQueueReceive(gpio_event_queue, &gpio_num, portMAX_DELAY)) // ����֮����Զ�ɾ������
		{
			if (gpio_num == KEYBOARD_INT)
			{
				uint8_t key = KEYBOARD_ReadKey();
				printf("key = %d\n", key);

				// �����ƹ����
				if (key == 'M')
					LED_Light(11);
				else if (key == '#')
					LED_Light(10);
				else
					LED_Light(key);

				// ������ĵİ������浽������
				password_array[password_count++] = key;
				// �ж��Ƿ�¼��ָ��
				if (password_count >= 3 && password_array[password_count - 1] == 'M' && password_array[password_count - 2] == 'M' && password_array[password_count - 3] == 'M')
				{
					password_count = 0;
					memset(password_array, 0, 6);
					is_enroll_flag = 1;
				}
				// �ж��Ƿ�OTA
				if (password_count >= 3 && password_array[password_count - 1] == 'M' && password_array[password_count - 2] == '#' && password_array[password_count - 3] == 'M')
				{
					is_ota_flag = 1;
				}
				// ��������
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
							break; // ������벻��ȣ�����ѭ��
					}
					if (i == 6)
					{
						MOTOR_OpenLock();
					}
					if (i < 6) // ����6λ����������flash�е��������벻ƥ�䣬����֤��ʱ����
					{
						if (PASSWORD_ValidateTempPassword(password_array[0] * 100000 + password_array[1] * 10000 + password_array[2] * 1000 + password_array[3] * 100 + password_array[4] * 10 + password_array[5])) // ��֤��ʱ����
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
// rtos���񣺲�����ѯis_enroll_flag��־λ
void app_main(void)
{
	KEYBOARD_Init();
	MOTOR_Init();
	AUDIO_Init();
	LED_Init();
	FINGER_Init();

	// ��ʼ��NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	BT_Init();
	WIFI_Init(); // ����WIFI�󣬾�ͬ����ʵʱʱ��
	// FINGER_GetSerialNumber();
	// FINGER_SetSecurityLevelAsZero();
	// FINGER_GetTemplatesNumber();
	// FINGER_Cancel();
	// while (FINGER_Sleep() == 1)
	// һֱѭ��ֱ�����߳ɹ�

	/* ���������� */
	// �����ж϶���
	gpio_event_queue = xQueueCreate(10, sizeof(uint32_t));
	// �����ж�
	gpio_install_isr_service(0);
	// ���ж����ź��жϻص�����
	gpio_isr_handler_add(KEYBOARD_INT, gpio_isr_handler, (void *)KEYBOARD_INT);
	gpio_isr_handler_add(FINGER_TOUCH_INT, gpio_isr_handler, (void *)KEYBOARD_INT);
	// ��process_isrע��Ϊfreertos���񣬿�ʼ��ѯ�ж϶���gpio_event_queue
	xTaskCreate(process_isr, "process_isr", 2048, NULL, 10, NULL);
	xTaskCreate(enroll_fingerprint_task, "enroll_fingerprint_task", 2048, NULL, 10, NULL);
	xTaskCreate(identify_fingerprint_task, "identify_fingerprint_task", 2048, NULL, 10, NULL);
	xTaskCreate(ota_task, "ota_task", 8192, NULL, 10, NULL);
	/* ������*/
	// MOTOR_OpenLock();

	/* ��Ƶ��� */
	// AUDIO_Play(14);
}