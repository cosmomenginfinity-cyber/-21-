#ifndef CODE_ASR_CTRL_H_
#define CODE_ASR_CTRL_H_

#define ASR_TARGET_IP           "ws-api.xfyun.cn"
#define ASR_TARGET_PORT         "80"
#define ASR_DOMAIN              "wss://ws-api.xfyun.cn/v2/iat"

/* Keep these two values matched to the microphone wiring and the CPU0 ISR. */
#define ASR_AUDIO_ADC           ADC0_CH4_A4
#define ASR_PIT                 CCU61_CH0
#define RANDOM_NUM_ADC          ADC0_CH2_A2

/* Field configuration: 2.4 GHz Wi-Fi and the existing iFlytek account. */
#define ASR_WIFI_SSID           "DESKTOP-3AEKKQC 9290"
#define ASR_WIFI_PASSWORD       "840@O25v"
#define ASR_APIID               "6c840264"
#define ASR_APISecret           "Mzc3YTdmMTkwYmEwMmRlY2QzNWRhMTAy"
#define ASR_APIKey              "64310ae5f351d9019b6c1e867ea1f583"

#endif
