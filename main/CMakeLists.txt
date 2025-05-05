idf_component_register(
    # SRCS��ʾҪ������`.c`�ļ�
    SRCS 
    "main.c"
    "dri/KEYBORAD_Driver.c"
    "dri/MOTOR_Driver.c"
    "dri/AUDIO_Driver.c"
    "dri/LED_Driver.c"
    "dri/FINGER_Driver.c"
    "dri/BT_Driver.c"
    "dri/WIFI_Driver.c"
    "dri/SNTP_Driver.c"
    "dri/PASSWORD_Driver.c"
    "dri/OTA_Driver.c"
    # INCLUDE_DIRS��ʾҪ������`.h`�ļ���Ŀ¼
    INCLUDE_DIRS 
    "./dri")
