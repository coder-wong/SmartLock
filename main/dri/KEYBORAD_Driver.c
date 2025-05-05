#include "KEYBORAD_Driver.h"

void I2C_Start(void)
{
    I2C_SDA_OUT;
    I2C_SDA_H;
    I2C_SCL_H;
    DelayMs(1);
    I2C_SDA_L;
    DelayMs(1);
    I2C_SCL_L;
    DelayMs(1);
}

uint8_t I2C_SendByteAndGetNACK(uint8_t dataToSend)
{
    uint8_t i;
    I2C_SDA_OUT;
    for (i = 0; i < 8; i++)
    {
        I2C_SCL_L;
        DelayMs(1);
        if ((dataToSend >> 7) & 0b00000001)
        {
            I2C_SDA_H;
        }
        else
        {
            I2C_SDA_L;
        }
        DelayMs(1);
        I2C_SCL_H;
        DelayMs(1);
        dataToSend <<= 1;
    }
    I2C_SCL_L;
    DelayMs(3);
    I2C_SDA_IN;
    DelayMs(3);
    I2C_SCL_H;
    DelayMs(1);
    i = 250;
    while (i--)
    {
        if (I2C_READ_SDA == 0)
        {
            I2C_SCL_L;
            return 0;
        }
    }
    I2C_SCL_L;
    return 1;
}

void I2C_Respond(uint8_t ACKSignal)
{
    I2C_SDA_OUT;
    I2C_SDA_L;
    I2C_SCL_L;
    if (ACKSignal)
    {
        I2C_SDA_H;
    }
    else
    {
        I2C_SDA_L;
    }
    DelayMs(1);
    I2C_SCL_H;
    DelayMs(1);
    I2C_SCL_L;
}

void I2C_Stop(void)
{
    I2C_SCL_L;
    I2C_SDA_OUT;
    I2C_SDA_L;
    DelayMs(1);
    I2C_SCL_H;
    DelayMs(1);
    I2C_SDA_H;
}

uint8_t I2C_Receive8Bit(void)
{
    uint8_t i, buffer = 0;
    I2C_SDA_IN;
    I2C_SCL_L;
    for (i = 0; i < 8; i++)
    {
        DelayMs(1);
        I2C_SCL_H;
        if (I2C_READ_SDA)
        {
            buffer = (buffer << 1) | 1;
        }
        else
        {
            buffer = (buffer << 1) | 0;
        }
        DelayMs(1);
        I2C_SCL_L;
    }

    return buffer;
}

uint16_t I2C_SimpleReadFromDevice(void)
{
    uint8_t buffer1, buffer2;
    I2C_Start();
    if (I2C_SendByteAndGetNACK((I2C_ADDR << 1) | 0b00000001))
    {
        I2C_Stop();
        return 0xFF;
    }
    buffer1 = I2C_Receive8Bit();
    I2C_Respond(0);
    buffer2 = I2C_Receive8Bit();
    I2C_Respond(1);
    I2C_Stop();
    return (((uint16_t)buffer1) << 8) | buffer2;
}
uint8_t KEYBOARD_ReadKey(void)
{
    uint16_t content = I2C_SimpleReadFromDevice();
    if (content == 0x4000) return 1;
    if (content == 0x2000) return 2;
    if (content == 0x1000) return 3;
    if (content == 0x0100) return 4;
    if (content == 0x0400) return 5;
    if (content == 0x0200) return 6;
    if (content == 0x0800) return 7;
    if (content == 0x0040) return 8;
    if (content == 0x0020) return 9;
    if (content == 0x0010) return '#';
    if (content == 0x8000) return 0;
    if (content == 0x0080) return 'M';
    return 0xFF;
}

/// GPIO初始化
void KEYBOARD_Init(void)
{
    gpio_config_t io_conf;
    // 禁用中断
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // 设置为输出模式
    io_conf.mode = GPIO_MODE_OUTPUT;
    // 选择要使用的引脚
    // ULL => unsigned long long
    // pin_bit_mask: 0b0000 => 0b0110
	// 固定写法
    io_conf.pin_bit_mask = (1ULL << I2C_SCL) | (1ULL << I2C_SDA);
    // 禁用下拉
    io_conf.pull_down_en = 0;
    // 使能上拉
    io_conf.pull_up_en = 1;
    // 使GPIO配置生效
    gpio_config(&io_conf);

    // 中断
    // POSEDGE：上升沿，positive edge
    // NEGEDGE：下降沿，negative edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << KEYBOARD_INT);
    gpio_config(&io_conf);
}