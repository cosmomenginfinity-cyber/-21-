#include "zf_common_headfile.h"
#include "hmac_sha256.h" 
#include "base64.h"  
#include "asr_ctrl.h"
// ==================== 新增：语音系统状态枚举 ====================
typedef enum {
    VOICE_STATE_IDLE,       // 空闲状态
    VOICE_STATE_LISTENING,  // 正在自动录音并监听（允许 audio_loop 运行）
    VOICE_STATE_EXECUTING   // 正在执行动作中（禁止语音，直到动作结束）
} VoiceSystemState_e;

typedef enum {
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

extern volatile uint8 audio_start_flag;
extern volatile uint8 audio_network_error_flag;
extern volatile AudioNetworkError_e audio_network_error_code;
extern volatile uint8 audio_need_net_flag;
extern volatile uint8 audio_server_link_flag;                        // 语音识别连接服务器标志位
extern volatile uint8 audio_send_data_flag;                          // 语音识别发送数据标志位
extern volatile int   audio_get_count;                               // 音频数据计数
extern volatile VoiceSystemState_e g_voice_state;

// ==================== 新增：本地 VAD 参数 ====================
#define VAD_SOUND_THRESHOLD     (120)       // 声音判定振幅阈值（根据实测环境底噪调节）
#define VAD_SILENCE_LIMIT       (16000)     // 2秒静音限制样本数（8000Hz采样率 * 2秒 = 16000）
#define VAD_MIN_SPEECH_LIMIT    (2400)      // 最短说话时间样本数（8000Hz * 0.3秒 = 2400），防止噪点瞬间触发
#define VAD_IDLE_TIMEOUT_LIMIT  (64000)     // 8秒不说话空闲超时（8000Hz * 8秒 = 64000）
// 语音命令枚举
// ====================  完善后的科目二动作状态枚举 ====================
typedef enum {
    CMD_NONE = 0,

    // 1. 灯光命令
    CMD_LIGHT_LEFT, CMD_LIGHT_RIGHT, CMD_LIGHT_HIGH, CMD_LIGHT_LOW,
    CMD_LIGHT_FOG, CMD_LIGHT_DOUBLE, CMD_LIGHT_INNER, CMD_WIPER,

    // 2. 鸣笛命令
    CMD_HORN_1S, CMD_HORN_2S, CMD_HORN_3S, CMD_HORN_DOUBLE, CMD_HORN_TRIPLE,
    CMD_HORN_QUAD, CMD_HORN_LONG_SHORT, CMD_HORN_URGENT, CMD_HORN_ALARM,
    CMD_HORN_MUSIC,     // 两只老虎
    CMD_HORN_ZHANG_SN,  // 念张师
    CMD_HORN_KOBE,      // 牢大进行曲
    CMD_HORN_TRUMP,     // 【新增】：川普/特朗普进行曲状态码

    // 3. 门洞命令
    CMD_GATE_GO, CMD_GATE_RET,

    // 4. 车模运动命令
    CMD_MOVE_FORWARD_10M,   // 1. 直行10米 (带航向锁)
    CMD_MOVE_BACKWARD_10M,  // 2. 倒车10米 (带反向航向锁)
    CMD_MOVE_S_FORWARD,     // 3. 蛇形前行10米 (大摆幅)
    CMD_MOVE_S_BACKWARD,    // 4. 蛇形后退10米 (大摆幅)
    CMD_MOVE_ROTATE_CCW,    // 5. 逆时针转一圈 (R=3.5m)
    CMD_MOVE_ROTATE_CW,     // 6. 顺时针转一圈 (R=3.5m)
    CMD_MOVE_LEFT,          // 7. 直行2米后左转90度
    CMD_MOVE_RIGHT          // 8. 直行2米后右转90度
} VoiceCmd_e;

extern volatile VoiceCmd_e g_active_voice_cmd; // 全局激活指令
extern char        now_time_out[32];
// ======= 在 menu.c 头部追加以下两行外部变量声明 =======
extern volatile uint32 asr_max_time;  // 外部引用语音最大识别时间戳
void audio_init(void);
void audio_callback(void);
void audio_loop(void);
void audio_session_begin(void);
void audio_session_request_finish(void);
void audio_session_cancel(void);
void Sub2_Execute_Task_CPU0(void);
void Sub2_Execute_Task_CPU2(void);
void voice_text_clear(void);
