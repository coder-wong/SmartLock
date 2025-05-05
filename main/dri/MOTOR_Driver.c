#include "MOTOR_Driver.h"
#include "driver/gpio.h"
#include "Utils.h"

void MOTOR_Init(void)
{
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = ((1ULL << MOTOR_PIN0) | (1ULL << MOTOR_PIN1));
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);
	// 初始化引脚为低电平,防止启动就发生转动（电平一样就不转，不一样就正转或反转）
	gpio_set_level(MOTOR_PIN1, 0);
	gpio_set_level(MOTOR_PIN0, 0);
}

void MOTOR_OpenLock(void)
{
	gpio_set_level(MOTOR_PIN1, 1);
	gpio_set_level(MOTOR_PIN0, 0);
	DelayMs(1000);

	gpio_set_level(MOTOR_PIN1, 0);
	gpio_set_level(MOTOR_PIN0, 0);
	DelayMs(1000);

	gpio_set_level(MOTOR_PIN1, 0);
	gpio_set_level(MOTOR_PIN0, 1);
	DelayMs(1000);

	gpio_set_level(MOTOR_PIN1, 0);
	gpio_set_level(MOTOR_PIN0, 0);
	DelayMs(1000);
}