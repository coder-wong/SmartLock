#include "PASSWORD_Driver.h"

/// 用来计算临时密码的hash code
/// 将字符串s转换成i32数值类型
/// 密码格式：AtguiguSmartLock001+8位时间戳
int32_t hash_code(char *s)
{
    int32_t h = 0;
    int i = 0;
    while (i < 19 + 8)
    {
        h = (h << 5) - h + ((uint8_t)s[i] | 0);
        i++;
    }
    return h;
}

uint8_t PASSWORD_ValidateTempPassword(uint32_t input_password)
{
	time_t now;
	time(&now);

	uint64_t now_in_minutes = now / 60;

	char buff[21];
	char *serial_number = "SamSmartLock001";
	for(uint8_t k = 0; k < 20; k++) // 20次代表20分钟
	{
		sprintf(buff, "%" PRIu64, now_in_minutes - k);

		char key[23] = {0};// 15(serial_number)+8(now_in_minutes)
		for(uint8_t i = 0; i < 15; i++)
		{
			key[i] = serial_number[i];
		}
		for(uint8_t j = 0; j < 8; j++)
		{
			key[15 + j] = buff[j];
		}
		/// 根据"序列号+时间戳"计算出一个i32的数值
		int32_t code = hash_code(key);
		/// 对100_0000求余，得到6位数字
        int32_t temp_password = ((code % 1000000) + 1000000) % 1000000;
		if(input_password == temp_password)
		{
			return 1; // 验证成功
		}
	}
	return 0; // 验证失败
}