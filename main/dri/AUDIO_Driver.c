#include "AUDIO_Driver.h"
#include "driver/gpio.h"
#include "Utils.h"

void AUDIO_Init(void)
{
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = (1ULL << AUDIO_DATA_PIN);
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);
}

void AUDIO_Play(uint8_t data)
{
	/// 先拉高2ms
	gpio_set_level(AUDIO_DATA_PIN, 1);
	DelayMs(2);
	// 拉低10ms ---> 重要，文档有误
	gpio_set_level(AUDIO_DATA_PIN, 0);
	DelayMs(10);
	for (uint8_t i = 0; i < 8; i++)
	{
		if(data & 0x01) // 低位先行
		{
			gpio_set_level(AUDIO_DATA_PIN, 1);
			DelayUs(600);
			gpio_set_level(AUDIO_DATA_PIN, 0);
			DelayUs(200);
		}
		else
		{
			gpio_set_level(AUDIO_DATA_PIN, 1);
			DelayUs(200);
			gpio_set_level(AUDIO_DATA_PIN, 0);
			DelayUs(600);
		}
		data >>= 1;
	}
	// 拉高2ms
	gpio_set_level(AUDIO_DATA_PIN, 1);
	DelayMs(2);
}