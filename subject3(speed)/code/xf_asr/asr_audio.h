#ifndef CODE_XF_ASR_ASR_AUDIO_H_
#define CODE_XF_ASR_ASR_AUDIO_H_

#include "zf_common_headfile.h"
#include "hmac_sha256.h"
#include "base64.h"
#include "asr_ctrl.h"

typedef enum
{
    VOICE_STATE_IDLE = 0,
    VOICE_STATE_LISTENING,
    VOICE_STATE_EXECUTING
} VoiceSystemState_e;

typedef enum
{
    AUDIO_NET_ERR_NONE = 0,
    AUDIO_NET_ERR_WIFI_INIT,
    AUDIO_NET_ERR_GET_TIME,
    AUDIO_NET_ERR_AUTH_URL,
    AUDIO_NET_ERR_WS_CONNECT,
    AUDIO_NET_ERR_START_SEND,
    AUDIO_NET_ERR_AUDIO_SEND,
    AUDIO_NET_ERR_FINAL_SEND,
    AUDIO_NET_ERR_RESULT_TIMEOUT
} AudioNetworkError_e;

typedef enum
{
    CMD_NONE = 0,

    CMD_LIGHT_LEFT,
    CMD_LIGHT_RIGHT,
    CMD_LIGHT_HIGH,
    CMD_LIGHT_LOW,
    CMD_LIGHT_FOG,
    CMD_LIGHT_DOUBLE,
    CMD_LIGHT_INNER,
    CMD_WIPER,

    CMD_HORN_1S,
    CMD_HORN_2S,
    CMD_HORN_3S,
    CMD_HORN_DOUBLE,
    CMD_HORN_TRIPLE,
    CMD_HORN_QUAD,
    CMD_HORN_LONG_SHORT,
    CMD_HORN_URGENT,
    CMD_HORN_ALARM
} VoiceCmd_e;

#define VAD_SOUND_THRESHOLD     (120)
#define VAD_SILENCE_LIMIT       (16000)
#define VAD_MIN_SPEECH_LIMIT    (2400)
#define VAD_IDLE_TIMEOUT_LIMIT  (64000)

extern volatile uint8 audio_start_flag;
extern volatile uint8 audio_network_error_flag;
extern volatile AudioNetworkError_e audio_network_error_code;
extern volatile uint8 audio_need_net_flag;
extern volatile uint8 audio_server_link_flag;
extern volatile uint8 audio_send_data_flag;
extern volatile int audio_get_count;
extern volatile VoiceSystemState_e g_voice_state;
extern volatile uint8 g_voice_task_active;
extern volatile VoiceCmd_e g_active_voice_cmd;
extern volatile uint8 g_tld_ready;
extern volatile uint8 g_tld_uart_active;
extern volatile uint8 g_lamp_scan_enabled;
extern volatile uint32 g_light_command_value;
extern volatile uint32 g_light_command_sequence;
extern volatile uint32 g_light_ack_sequence;
extern volatile uint32 asr_max_time;
extern char now_time_out[32];

void audio_init(void);
void audio_callback(void);
void audio_loop(void);
void audio_session_begin(void);
void audio_session_request_finish(void);
void audio_session_cancel(void);
void voice_text_clear(void);
void voice_action_service_cpu0(void);
void voice_light_clear_async(void);
void voice_execute_light_cpu2(void);

#endif
