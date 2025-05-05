#include "FINGER_Driver.h"
#include "Utils.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "AUDIO_Driver.h"

#define BUF_SIZE 1024
void FINGER_Init(void)
{
	uart_config_t uart_config = {
		.baud_rate = 57600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT};
	uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);
	uart_param_config(UART_NUM_1, &uart_config);
	uart_set_pin(UART_NUM_1, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	// 配置指纹模块的中断引脚，只要一放手指就会触发中断引脚
	gpio_config_t io_conf = {0};
	io_conf.intr_type = GPIO_INTR_POSEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = (1ULL << FINGER_TOUCH_INT);
	gpio_config(&io_conf);

	DelayMs(200); // 文档要求上电初始化后延迟200ms
}

void FINGER_GetSerialNumber(void)
{
	uint8_t recv_data[128] = {0};

	// 获取芯片唯一序列号 0x34。确认码=00H 表示 OK；确认码=01H 表示收包有错。
	uint8_t cmd[13] = {
		0xEF, 0x01,				// 包头
		0xFF, 0xFF, 0xFF, 0xFF, // 默认的设备地址
		0x01,					// 包标识
		0x00, 0x04,				// 包长度
		0x34,					// 指令码
		0x00,					// 参数
		0x00, 0x39				// 校验和sum
	};
	uart_write_bytes(UART_NUM_1, (const char *)cmd, 13);
	// Read data from the UART
	int len = uart_read_bytes(UART_NUM_1, recv_data, (BUF_SIZE - 1), 100 / portTICK_PERIOD_MS);
	printf("Received %d bytes: ", len);
	if (len > 0)
	{
		// 校验包标识位和确认码位
		if (recv_data[6] == 0x07 && recv_data[9] == 0x00)
		{
			printf("指纹模块序列号: %.32s\r\n", &recv_data[10]);
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x01)
		{
			printf("获取指纹序列号收包错误。\r\n");
		}
	}
}

uint8_t FINGER_GetTemplatesNumber(void)
{
	uint8_t recv_data[128] = {0};

	// 获取指纹模块中已经存储的指纹数量
	uint8_t cmd[12] = {
		0xEF, 0x01,				// 包头
		0xFF, 0xFF, 0xFF, 0xFF, // 默认的设备地址
		0x01,					// 包标识
		0x00, 0x03,				// 包长度
		0x1D,					// 指令码
		0x00, 0x21				// 校验和sum
	};
	uart_write_bytes(UART_NUM_1, (const char *)cmd, 12);

	// Read data from the UART
	int len = uart_read_bytes(UART_NUM_1, recv_data, (BUF_SIZE - 1),
							  100 / portTICK_PERIOD_MS);

	if (len > 0)
	{
		if (recv_data[6] == 0x07 && recv_data[9] == 0x00)
		{
			printf("已经存储的指纹的数量是：%d\r\n", recv_data[11]);
			return recv_data[11];
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x01)
		{
			printf("获取指纹模板数量收包错误。\r\n");
			return 0xFF;
		}
		else
		{
			return 0xFF;
		}
	}
	else
	{
		return 0xFF;
	}
}

uint8_t FINGER_Sleep(void)
{
	uint8_t recv_data[128] = {0};

	// 指纹模块休眠指令
	uint8_t cmd[12] = {
		0xEF, 0x01,				// 包头
		0xFF, 0xFF, 0xFF, 0xFF, // 默认的设备地址
		0x01,					// 包标识
		0x00, 0x03,				// 包长度
		0x33,					// 指令码
		0x00, 0x37				// 校验和sum
	};
	uart_write_bytes(UART_NUM_1, (const char *)cmd, 12);

	// Read data from the UART
	int len = uart_read_bytes(UART_NUM_1, recv_data, (BUF_SIZE - 1),
							  100 / portTICK_PERIOD_MS);

	if (len > 0)
	{
		if (recv_data[6] == 0x07 && recv_data[9] == 0x00)
		{
			printf("指纹模块休眠成功。\r\n");
			return 0;
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x01)
		{
			printf("指纹模块休眠失败。\r\n");
		}
	}
	return 1;
}

void FINGER_Cancel(void)
{
	uint8_t recv_data[128] = {0};

	// 取消正在进行的录入指纹或者验证指纹流程
	// 防止指纹模块假死
	// 校验和是从包标识至校验和之间所有字节之和，包含包标识不包含校验和
	// 校验和 0x34 = 0x01 + 0x00 + 0x03 + 0x30
	uint8_t cmd[12] = {
		0xEF, 0x01,				// 包头
		0xFF, 0xFF, 0xFF, 0xFF, // 默认的设备地址
		0x01,					// 包标识
		0x00, 0x03,				// 包长度
		0x30,					// 指令码
		0x00, 0x34				// 校验和sum
	};
	uart_write_bytes(UART_NUM_1, (const char *)cmd, 12);

	// Read data from the UART
	int len = uart_read_bytes(UART_NUM_1, recv_data, (BUF_SIZE - 1),
							  100 / portTICK_PERIOD_MS);

	if (len > 0)
	{
		if (recv_data[6] == 0x07 && recv_data[9] == 0x00)
		{
			printf("指纹模块取消指令执行成功。\r\n");
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x01)
		{
			printf("指纹模块取消指令执行失败。\r\n");
		}
	}
}

uint8_t FINGER_GetImage(void)
{
	uint8_t recv_data[128] = {0};

	uint8_t cmd[12] = {
		0xEF, 0x01,				// 包头
		0xFF, 0xFF, 0xFF, 0xFF, // 默认的设备地址
		0x01,					// 包标识
		0x00, 0x03,				// 包长度
		0x01,					// 指令码
		0x00, 0x05				// 校验和sum
	};
	uart_write_bytes(UART_NUM_1, (const char *)cmd, 12);

	// Read data from the UART
	int len = uart_read_bytes(UART_NUM_1, recv_data, (BUF_SIZE - 1),
							  100 / portTICK_PERIOD_MS);

	if (len > 0)
	{
		// 校验包标识位和确认码位
		if (recv_data[6] == 0x07 && recv_data[9] == 0x00)
		{
			printf("获取指纹图像成功。\r\n");
			return 0;
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x01)
		{
			printf("获取指纹图像失败。\r\n");
			return 1;
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x02)
		{
			printf("未检测到手指。\r\n");
			return 1;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}
}

uint8_t FINGER_GenChar(uint8_t BufferID)
{
	uint8_t recv_data[128] = {0};

	uint8_t cmd[12] = {
		0xEF, 0x01,				// 包头
		0xFF, 0xFF, 0xFF, 0xFF, // 默认的设备地址
		0x01,					// 包标识
		0x00, 0x04,				// 包长度
		0x02,					// 指令码
		'\0',					// BufferID占位符
		'\0'					// 校验和sum
	};
	cmd[10] = BufferID;
	// 校验和的计算
	uint16_t sum = cmd[6] + cmd[7] + cmd[8] + cmd[9] + cmd[10];
	cmd[11] = (sum >> 8);
	cmd[12] = sum;

	uart_write_bytes(UART_NUM_1, (const char *)cmd, 12);

	// Read data from the UART
	int len = uart_read_bytes(UART_NUM_1, recv_data, (BUF_SIZE - 1),
							  500 / portTICK_PERIOD_MS); // 100MS太短，处理时间不够

	if (len > 0)
	{
		if (recv_data[6] == 0x07 && recv_data[9] == 0x00)
		{
			printf("生成指纹特征成功。\r\n");
			return 0;
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x01)
		{
			printf("生成指纹特征时收包失败。\r\n");
			return 1;
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x06)
		{
			printf("指纹图像太乱而生不成特征。\r\n");
			return 1;
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x07)
		{
			printf("指纹图像正常，但特征点太少而生不成特征。\r\n");
			return 1;
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x08)
		{
			printf("当前指纹特征与之前特征之间无关联。\r\n");
			return 1;
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x0A)
		{
			printf("合并失败。\r\n");
			return 1;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}
}

// 合并特征
// 3.3.1.5 合并特征（生成模板）PS_RegModel
uint8_t FINGER_RegModel(void)
{
	uint8_t command[12] = {
		0xEF, 0x01,				// 包头
		0xFF, 0xFF, 0xFF, 0xFF, // 默认地址
		0x01,					// 包标识
		0x00, 0x03,				// 包长度
		0x05,					// 指令码
		0x00, 0x09				// 校验和
	};

	// 通过串口发送数据
	uart_write_bytes(UART_NUM_1, (const char *)command, 12);

	// 读取数据
	uint8_t recv_data[64] = {0};
	int length = uart_read_bytes(UART_NUM_1, recv_data, BUF_SIZE - 1,
								 500 / portTICK_PERIOD_MS);

	if (length > 0)
	{
		if (recv_data[6] == 0x07 && recv_data[9] == 0x00)
		{
			printf("合并特征成功。\r\n");
			return 0;
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x0A)
		{
			printf("合并失败.\r\n");
			return 1;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}
}

// 存储指纹模板，存储到某一页
// 3.3.1.6 储存模板PS_StoreChar
uint8_t FINGER_StoreChar(uint8_t PageID)
{
	uint8_t command[15] = {
		0xEF, 0x01,				// 包头
		0xFF, 0xFF, 0xFF, 0xFF, // 默认地址
		0x01,					// 包标识
		0x00, 0x06,				// 包长度
		0x06,					// 指令码
		0x01,					// BufferID
		0x00, '\0',				// PageID
		'\0', '\0'				// 校验和
	};

	command[12] = PageID;
	uint16_t sum = command[6] + command[7] + command[8] + command[9] +
				   command[10] + command[11] + command[12];
	command[13] = sum >> 8;
	command[14] = sum;

	// 通过串口发送数据
	uart_write_bytes(UART_NUM_1, (const char *)command, 15);

	// 读取数据
	uint8_t recv_data[64] = {0};
	int length = uart_read_bytes(UART_NUM_1, recv_data, BUF_SIZE - 1,
								 500 / portTICK_PERIOD_MS);

	if (length > 0)
	{
		if (recv_data[6] == 0x07 && recv_data[9] == 0x00)
		{
			printf("指纹模板存储成功.\r\n");
			return 0;
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x0B)
		{
			printf("PageID超出指纹库范围.\r\n");
			return 1;
		}
		else if (recv_data[6] == 0x07 && recv_data[9] == 0x18)
		{
			printf("写入指纹模块的Flash失败.\r\n");
			return 1;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 1;
	}
}

/// 将安全等级设置为0
/// 3.3.1.12 写系统寄存器PS_WriteReg
/// 寄存器号：7， 加密等级寄存器 内容：0，level 0
void FINGER_SetSecurityLevelAsZero(void)
{
	uint8_t command[14] = {
		0xEF, 0x01,				// 包头
		0xFF, 0xFF, 0xFF, 0xFF, // 默认地址
		0x01,					// 包标识
		0x00, 0x05,				// 包长度
		0x0E,					// 指令码
		0x07,					// 寄存器号
		0x00,					// 内容
		0x00, 0x1B				// 校验和
	};

	// 通过串口发送数据
	uart_write_bytes(UART_NUM_1, (const char *)command, 14);

	// 读取数据
	uint8_t recv_data[64] = {0};
	int length = uart_read_bytes(UART_NUM_1, recv_data, BUF_SIZE - 1,
								 100 / portTICK_PERIOD_MS);

	if (length > 0)
	{
		if (recv_data[6] == 0x07 && recv_data[9] == 0x00)
		{
			printf("指纹模块安全等级设置为0成功.\r\n");
		}
	}
}

uint8_t FINGER_Enroll(uint8_t PageID)
{
	uint8_t n = 1;
SendGetImageLabel:
	if (FINGER_GetImage() == 1) // 1表示失败，就重新获取图像
	{
		goto SendGetImageLabel;
	}
	if (FINGER_GenChar(n) == 1) // 1表示失败，就重新获取图像
	{
		goto SendGetImageLabel;
	}

	printf("第 %d 次获取图像以及生成特征成功.\r\n", n);
	AUDIO_Play(82);
	DelayMs(2000);

	if (n < 4)
	{
		n++; // 获取四次指纹图像
		AUDIO_Play(81);
		DelayMs(1000);
		goto SendGetImageLabel;
	}
	if (FINGER_RegModel() == 1)
	{
		AUDIO_Play(84); // 录入失败
		return 1; 
	}
	if (FINGER_StoreChar(PageID) == 1)
	{
		AUDIO_Play(84);// 录入失败
		return 1;
	}
	// 指纹录入成功
	AUDIO_Play(83);
	return 0; 
}


uint8_t FINGER_SearchFinger(void)
{
	uint8_t command[17] = {
		0xEF, 0x01,				// 包头
		0xFF, 0xFF, 0xFF, 0xFF, // 默认地址
		0x01,					// 包标识
		0x00, 0x08,				// 包长度
		0x04,					// 指令码
		0x01,					// BufferID
		0x00, 0x00,				// 搜索的起始页码
		0xFF, 0xFF,				// 搜索的结束页码
		0x02, 0x0c				// 校验和
	};

	// 通过串口发送数据
	uart_write_bytes(UART_NUM_1, (const char *)command, 17);

	// 读取数据
	uint8_t recv_data[64] = {0};
	int length = uart_read_bytes(UART_NUM_1, recv_data, 64 - 1,
								 500 / portTICK_PERIOD_MS);

	if (length > 0)
	{
		if (recv_data[6] == 0x07 && recv_data[9] == 0x00)
		{
			return 0; // 搜索成功
		}
		else
		{
			return 1; // 搜索失败
		}
	}
	else
	{
		return 1;
	}
}

uint8_t FINGER_Identify(void)
{
SendGetImageLabel:
	if (FINGER_GetImage() == 1) // 1表示失败，就重新获取图像
	{
		goto SendGetImageLabel;
	}
	if (FINGER_GenChar(1) == 1)
	{
		goto SendGetImageLabel;
	}
	if(FINGER_SearchFinger() == 0)
	{
		return 0;
	}
	return 1;
}