#ifndef CODE_ASR_CTRL_H_
#define CODE_ASR_CTRL_H_

#define ASR_TARGET_IP           "ws-api.xfyun.cn"
#define ASR_TARGET_PORT         "80"
#define ASR_DOMAIN              "wss://ws-api.xfyun.cn/v2/iat"
#define ASR_AUDIO_ADC           ADC0_CH4_A4                             // 硅麦ADC
#define ASR_BUTTON              P20_6                                   // 控制按键
#define ASR_PIT                 CCU60_CH1                               // 定时器中断

#define ASR_WIFI_SSID           "DESKTOP-3AEKKQC 9290"                         // wifi名称 wifi需要是2.4G频段
#define ASR_WIFI_PASSWORD       "840@O25v"                            // wifi密码
/*#define ASR_WIFI_SSID           "PC-202211061147 0390"                         // wifi名称 wifi需要是2.4G频段LAPTOP-M556EHGP 0159
#define ASR_WIFI_PASSWORD       "06zJ1]76"                            // wifi密码m902F8\0*/
#define ASR_APIID               "6c840264"                              // 讯飞的id
#define ASR_APISecret           "Mzc3YTdmMTkwYmEwMmRlY2QzNWRhMTAy"      // 讯飞的Secret
#define ASR_APIKey              "64310ae5f351d9019b6c1e867ea1f583"      // 讯飞的Key

#define RANDOM_NUM_ADC          ADC0_CH2_A2                             // 使用ADC生成随机数

#endif /* CODE_ASR_CTRL_H_ */
