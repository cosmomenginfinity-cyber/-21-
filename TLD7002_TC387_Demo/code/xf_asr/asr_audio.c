#include "asr_audio.h"
#include "sub2_navigation.h"
#include "asr_ctrl.h"
#include "websocket_client.h"
#include "zf_device_dot_matrix_screen.h"
#include "zf_device_tld7002.h"
#include "initialize.h"
#include "encoder.h"
#include "imu_datalogger.h"
#include "ins.h"
#include <string.h>
#include "global_vars.h"

#define     ASR_SEND_DATA_MAX_LENTH     (4000)
volatile VoiceSystemState_e g_voice_state = VOICE_STATE_IDLE;

#define LPF_SHIFT   8


#define NOTE_C3  131
#define NOTE_D3  147
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_G3  196
#define NOTE_A3  220
#define NOTE_B3  247

#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494

#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988

#define NOTE_C6  1047
#define NOTE_D6  1175
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_G6  1568
#define NOTE_A6  1760
#define NOTE_B6  1976

#define NOTE_REST 0

#pragma section all "cpu1_dsram"
char        asr_url_out[512];

#pragma section all "cpu0_dsram"
char        time_now_data[32];
char        now_time_out[32];
uint8       wifi_spi_get_data_buffer[1024];
char        http_request[1024];

#define ASR_TIME_RETRY_COUNT         (4U)
#define ASR_TIME_RETRY_DELAY_MS      (250U)
#define ASR_TIME_CACHE_MAX_AGE_MS    (240000U)

static uint8  audio_time_cache_valid = 0;
static uint32 audio_time_cache_ms = 0;

int16       adc_get_one_data;
volatile int audio_get_count = -1;

fifo_struct adc_data_fifo;
uint32      fifo_data_count;
int16       adc_get_data[ASR_SEND_DATA_MAX_LENTH * 2];
int16       fifo_get_data[ASR_SEND_DATA_MAX_LENTH];

char        json_data[ASR_SEND_DATA_MAX_LENTH * 3];
char        temp_data[WEBSOCKET_SEND_BUFFER_SIZE];
uint8       websocket_receive_buffer[4096];

volatile uint8  audio_start_flag = 0;
volatile uint8  audio_need_net_flag = 0;
volatile uint8  audio_server_link_flag = 0;
volatile uint8  audio_send_data_flag = 0;
volatile uint8  audio_network_error_flag = 0;
volatile AudioNetworkError_e audio_network_error_code = AUDIO_NET_ERR_NONE;
volatile uint32 asr_max_time = 0;

static uint8 audio_wifi_reconnect_required = 0;

char        g_voice_text[256] = {0};
volatile    VoiceCmd_e g_active_voice_cmd = CMD_NONE;

volatile uint8 g_vad_connect_print_flag = 0;
volatile uint8 g_vad_speech_print_flag  = 0;
volatile uint8 g_vad_silence_print_flag = 0;
volatile uint8 g_vad_idle_print_flag    = 0;
#pragma section all restore

void sub2_horn_on(uint32 frequency_hz)
{
    pwm_init(SUB2_HORN_PWM_PIN, frequency_hz, 5000);
}

void sub2_horn_off(void)
{
    pwm_init(SUB2_HORN_PWM_PIN, 1000, 0);
}

typedef struct {
    uint16_t frequency;
    uint16_t duration_ms;
} Note_t;

void play_two_tigers(void)
{
    const Note_t song[] = {
        {NOTE_C4, 400}, {NOTE_D4, 400}, {NOTE_E4, 400}, {NOTE_C4, 400},
        {NOTE_C4, 400}, {NOTE_D4, 400}, {NOTE_E4, 400}, {NOTE_C4, 400},
        {NOTE_E4, 400}, {NOTE_F4, 400}, {NOTE_G4, 800},
        {NOTE_E4, 400}, {NOTE_F4, 400}, {NOTE_G4, 800},
        {NOTE_G4, 200}, {NOTE_A4, 200}, {NOTE_G4, 200}, {NOTE_F4, 200}, {NOTE_E4, 400}, {NOTE_C4, 400},
        {NOTE_G4, 200}, {NOTE_A4, 200}, {NOTE_G4, 200}, {NOTE_F4, 200}, {NOTE_E4, 400}, {NOTE_C4, 400},
        {NOTE_D4, 400}, {NOTE_G3, 400}, {NOTE_C4, 800},
        {NOTE_D4, 400}, {NOTE_G3, 400}, {NOTE_C4, 800}
    };
    uint32_t num_notes = sizeof(song) / sizeof(song[0]);
    for (uint32_t i = 0; i < num_notes; i++) {
        sub2_horn_on(song[i].frequency);
        system_delay_ms(song[i].duration_ms - 40);
        sub2_horn_off();
        system_delay_ms(40);
    }
    sub2_horn_off();
}

void play_remembering_zhang(void)
{
    const Note_t song[] = {
        {NOTE_G4, 300}, {NOTE_E4, 300}, {NOTE_G4, 300}, {NOTE_E4, 300}, {NOTE_E4, 600}, {NOTE_D5, 600}, {NOTE_REST, 600},
        {NOTE_C5, 300}, {NOTE_C5, 300}, {NOTE_C5, 300}, {NOTE_E5, 300}, {NOTE_E5, 300}, {NOTE_G4, 300}, {NOTE_G4, 600}, {NOTE_REST, 600},
        {NOTE_REST, 600}, {NOTE_A4, 300}, {NOTE_G4, 300}, {NOTE_A4, 300}, {NOTE_G4, 300}, {NOTE_G4, 600}, {NOTE_REST, 600},
        {NOTE_E4, 300}, {NOTE_C4, 300}, {NOTE_F4, 300}, {NOTE_E4, 300},
        {NOTE_F4, 300}, {NOTE_A4, 300}, {NOTE_G4, 600}, {NOTE_G4, 600}, {NOTE_REST, 600},
        {NOTE_G4, 300}, {NOTE_E4, 300}, {NOTE_G4, 300}, {NOTE_E4, 300}, {NOTE_E4, 600}, {NOTE_D5, 600}, {NOTE_REST, 600},
        {NOTE_C5, 300}, {NOTE_C5, 300}, {NOTE_C5, 300}, {NOTE_E5, 300}, {NOTE_E5, 300}, {NOTE_A4, 300}, {NOTE_A4, 300}, {NOTE_G4, 600},
        {NOTE_REST, 600}, {NOTE_A4, 300}, {NOTE_B4, 300}, {NOTE_C5, 300}, {NOTE_C5, 300},
        {NOTE_A4, 300}, {NOTE_A4, 300}, {NOTE_G4, 300}, {NOTE_G4, 600}, {NOTE_REST, 600},
        {NOTE_D5, 600}, {NOTE_E5, 600}, {NOTE_E5, 300}, {NOTE_D5, 300}, {NOTE_C5, 1800}
    };
    uint32_t num_notes = sizeof(song) / sizeof(song[0]);
    for (uint32_t i = 0; i < num_notes; i++) {
        if (song[i].frequency == NOTE_REST) {
            sub2_horn_off();
            system_delay_ms(song[i].duration_ms);
        } else {
            sub2_horn_on(song[i].frequency);
            system_delay_ms(song[i].duration_ms - 40);
            sub2_horn_off();
            system_delay_ms(40);
        }
    }
    sub2_horn_off();
}

void play_see_you_again(void)
{
    const Note_t song[] = {
        {NOTE_G5, 100}, {NOTE_D6, 100}, {NOTE_C6, 100}, {NOTE_G5, 100}, {NOTE_G5, 100},
        {NOTE_C6, 50},  {NOTE_D6, 50},  {NOTE_E6, 50},  {NOTE_D6, 50},  {NOTE_C6, 50}, {NOTE_D6, 50},
        {NOTE_G5, 100}, {NOTE_D6, 100}, {NOTE_C6, 100}, {NOTE_G5, 100}, {NOTE_G5, 100},
        {NOTE_C6, 50},  {NOTE_D6, 50},  {NOTE_E6, 50},  {NOTE_D6, 50},  {NOTE_C6, 50}, {NOTE_D6, 50},
        {NOTE_E5, 100}, {NOTE_G5, 100},
        {NOTE_A5, 300}, {NOTE_G5, 100}, {NOTE_G5, 100},
        {NOTE_REST, 300}, {NOTE_C6, 100},
        {NOTE_D6, 100}, {NOTE_D6, 100}, {NOTE_C6, 100}, {NOTE_D6, 100}, {NOTE_E6, 200},
        {NOTE_REST, 200}, {NOTE_E5, 100}, {NOTE_G5, 100},
        {NOTE_A5, 100}, {NOTE_B5, 100}, {NOTE_A5, 100}, {NOTE_G5, 100}, {NOTE_E5, 100}, {NOTE_D5, 100},
        {NOTE_D6, 100}, {NOTE_D6, 100}, {NOTE_C6, 100},
        {NOTE_D5, 100}, {NOTE_D5, 100}, {NOTE_D5, 100}, {NOTE_C5, 100}, {NOTE_REST, 200},
        {NOTE_REST, 200}, {NOTE_C5, 100}, {NOTE_E5, 100}, {NOTE_G5, 100},
        {NOTE_A5, 300}, {NOTE_G5, 100}, {NOTE_G5, 100},
        {NOTE_REST, 300}, {NOTE_C5, 100},
        {NOTE_D5, 100}, {NOTE_D5, 100}, {NOTE_C5, 100}, {NOTE_E5, 200}, {NOTE_REST, 200},
        {NOTE_D5, 100}, {NOTE_E5, 100}, {NOTE_G5, 100},
        {NOTE_A5, 100}, {NOTE_C6, 100}, {NOTE_D6, 100}, {NOTE_E6, 100}, {NOTE_D6, 100}, {NOTE_C6, 100}, {NOTE_G5, 100}, {NOTE_A5, 100}, {NOTE_C6, 100},
        {NOTE_E5, 100}, {NOTE_E5, 100}, {NOTE_E5, 100}, {NOTE_D5, 100}, {NOTE_REST, 400}
    };
    uint32_t num_notes = sizeof(song) / sizeof(song[0]);
    for (uint32_t i = 0; i < num_notes; i++) {
        if (song[i].frequency == NOTE_REST) {
            sub2_horn_off();
            system_delay_ms(song[i].duration_ms);
        } else {
            sub2_horn_on(song[i].frequency);
            system_delay_ms(song[i].duration_ms - 25);
            sub2_horn_off();
            system_delay_ms(25);
        }
    }
    sub2_horn_off();
}

void play_trump_march(void)
{
    const Note_t song[] = {
        {NOTE_G4, 250}, {NOTE_E5, 500}, {NOTE_REST, 500}, {NOTE_REST, 250}, {NOTE_E5, 250}, {NOTE_D5, 500},
        {NOTE_C5, 250}, {NOTE_D5, 250}, {NOTE_E5, 250}, {NOTE_G5, 250}, {NOTE_E5, 250}, {NOTE_G5, 500}, {NOTE_REST, 250},
        {NOTE_A4, 250}, {NOTE_E5, 500}, {NOTE_REST, 500}, {NOTE_REST, 250}, {NOTE_E5, 250}, {NOTE_D5, 500},
        {NOTE_C5, 250}, {NOTE_D5, 250}, {NOTE_E5, 250}, {NOTE_G5, 250}, {NOTE_E5, 250}, {NOTE_G5, 500}, {NOTE_REST, 250},
        {NOTE_A4, 250}, {NOTE_F5, 500}, {NOTE_REST, 500}, {NOTE_REST, 250}, {NOTE_F5, 250}, {NOTE_E5, 500},
        {NOTE_D5, 250}, {NOTE_F5, 250}, {NOTE_G5, 250}, {NOTE_A5, 250}, {NOTE_C6, 250}, {NOTE_A5, 500}, {NOTE_REST, 250},
        {NOTE_B4, 250}, {NOTE_B4, 250}, {NOTE_A4, 250}, {NOTE_A4, 250}, {NOTE_G4, 1000},
        {NOTE_G4, 250}, {NOTE_F4, 250}, {NOTE_E4, 250}, {NOTE_D4, 750}, {NOTE_REST, 500},
        {NOTE_C6, 250}, {NOTE_C6, 250}, {NOTE_C6, 250}, {NOTE_C6, 250},
        {NOTE_REST, 250}, {NOTE_G5, 250}, {NOTE_E5, 250}, {NOTE_G5, 250}, {NOTE_E5, 250}, {NOTE_G5, 500}, {NOTE_REST, 250},
        {NOTE_A5, 1000}, {NOTE_G5, 300}, {NOTE_G5, 100}, {NOTE_A5, 200}, {NOTE_G5, 400},
        {NOTE_REST, 250}, {NOTE_G5, 250}, {NOTE_E5, 250}, {NOTE_G5, 250}, {NOTE_E5, 250}, {NOTE_G5, 500}, {NOTE_REST, 250},
        {NOTE_A5, 1000}, {NOTE_E5, 300}, {NOTE_E5, 100}, {NOTE_D5, 200}, {NOTE_C5, 400},
        {NOTE_REST, 250}, {NOTE_A4, 250}, {NOTE_A4, 250}, {NOTE_C5, 250}, {NOTE_REST, 250}, {NOTE_C6, 250}, {NOTE_A5, 250}, {NOTE_REST, 250},
        {NOTE_C6, 1000}, {NOTE_A5, 250}, {NOTE_C6, 250}, {NOTE_A5, 500},
        {NOTE_C6, 250}, {NOTE_C6, 250}, {NOTE_C6, 250}, {NOTE_G5, 250}, {NOTE_A5, 250}, {NOTE_C6, 250}, {NOTE_A5, 500},
        {NOTE_C6, 250}, {NOTE_C6, 250}, {NOTE_A5, 250}, {NOTE_C6, 250}, {NOTE_A5, 250}, {NOTE_G5, 250}, {NOTE_G5, 500},
        {NOTE_A5, 250}, {NOTE_C6, 250}, {NOTE_A5, 250}, {NOTE_C6, 250}, {NOTE_A5, 250}, {NOTE_C6, 500}, {NOTE_REST, 250}
    };
    uint32_t num_notes = sizeof(song) / sizeof(song[0]);
    for (uint32_t i = 0; i < num_notes; i++) {
        if (song[i].frequency == NOTE_REST) {
            sub2_horn_off();
            system_delay_ms(song[i].duration_ms);
        } else {
            sub2_horn_on(song[i].frequency);
            system_delay_ms(song[i].duration_ms - 40);
            sub2_horn_off();
            system_delay_ms(40);
        }
    }
    sub2_horn_off();
}

void voice_text_clear(void) {
    memset(g_voice_text, 0, sizeof(g_voice_text));
}
// 关键词匹配状态机 (宽频谱模糊匹配)
void voice_command_parse(const char* text) {
    if (strstr(text, "左转向灯") != NULL || strstr(text, "左转向") != NULL || strstr(text, "左转灯") != NULL)       g_active_voice_cmd = CMD_LIGHT_LEFT;
    else if (strstr(text, "右转向灯") != NULL || strstr(text, "右转向") != NULL || strstr(text, "右转灯") != NULL)  g_active_voice_cmd = CMD_LIGHT_RIGHT;
    else if (strstr(text, "远光灯") != NULL || strstr(text, "远") != NULL)    g_active_voice_cmd = CMD_LIGHT_HIGH;
    else if (strstr(text, "近光灯") != NULL || strstr(text, "近") != NULL)    g_active_voice_cmd = CMD_LIGHT_LOW;
    else if (strstr(text, "车内照明") != NULL || strstr(text, "车内灯") != NULL || strstr(text, "照明灯") != NULL) g_active_voice_cmd = CMD_LIGHT_INNER;
    else if (strstr(text, "双闪灯") != NULL || strstr(text, "双闪") != NULL || strstr(text, "双") != NULL)    g_active_voice_cmd = CMD_LIGHT_DOUBLE;
    else if (strstr(text, "雾灯") != NULL || strstr(text, "雾") != NULL)    g_active_voice_cmd = CMD_LIGHT_FOG;
    else if (strstr(text, "雨刷") != NULL || strstr(text, "刷") != NULL || strstr(text, "雨") != NULL)      g_active_voice_cmd = CMD_WIPER;

    else if (strstr(text, "鸣笛三秒") != NULL || strstr(text, "3秒") != NULL || strstr(text, "三秒") != NULL)  g_active_voice_cmd = CMD_HORN_3S;
    else if (strstr(text, "鸣笛两秒") != NULL || strstr(text, "2秒") != NULL || strstr(text, "两秒") != NULL || strstr(text, "二秒") != NULL)  g_active_voice_cmd = CMD_HORN_2S;
    else if (strstr(text, "鸣笛一声") != NULL || strstr(text, "1秒") != NULL || strstr(text, "一秒") != NULL || strstr(text, "一声") != NULL)  g_active_voice_cmd = CMD_HORN_1S;

    // ======= 物理宽频谱防丢字模糊匹配 =======
    else if (strstr(text, "鸣笛四声") != NULL || strstr(text, "4声") != NULL || strstr(text, "四声") != NULL || strstr(text, "四生") != NULL)  g_active_voice_cmd = CMD_HORN_QUAD;
    else if (strstr(text, "鸣笛三声") != NULL || strstr(text, "3声") != NULL || strstr(text, "三声") != NULL || strstr(text, "三生") != NULL)  g_active_voice_cmd = CMD_HORN_TRIPLE;
    else if (strstr(text, "鸣笛两声") != NULL || strstr(text, "2声") != NULL || strstr(text, "二声") != NULL || strstr(text, "二生") != NULL)  g_active_voice_cmd = CMD_HORN_DOUBLE;
    else if (strstr(text, "长短") != NULL || strstr(text, "长") != NULL || strstr(text, "短") != NULL)                                   g_active_voice_cmd = CMD_HORN_LONG_SHORT;
    else if (strstr(text, "急促") != NULL || strstr(text, "急速") != NULL || strstr(text, "极速") != NULL  || strstr(text, "急") != NULL)    g_active_voice_cmd = CMD_HORN_URGENT;
    else if (strstr(text, "警报") != NULL || strstr(text, "警") != NULL)                                     g_active_voice_cmd = CMD_HORN_ALARM;
    else if (strstr(text, "两只老虎") != NULL || strstr(text, "唱首歌") != NULL) g_active_voice_cmd = CMD_HORN_MUSIC;
    else if (strstr(text, "念张师") != NULL || strstr(text, "张雪峰") != NULL) g_active_voice_cmd = CMD_HORN_ZHANG_SN;
    else if (strstr(text, "科比") != NULL || strstr(text, "牢大") != NULL || strstr(text, "闪电") != NULL) g_active_voice_cmd = CMD_HORN_KOBE;
    else if (strstr(text, "川普") != NULL || strstr(text, "特朗普") != NULL) g_active_voice_cmd = CMD_HORN_TRUMP;

    else if (strstr(text, "门") != NULL)
    {
        bool is_return = (strstr(text, "返回") != NULL || strstr(text, "回程") != NULL ||
                          strstr(text, "返程") != NULL || strstr(text, "回") != NULL ||
                          strstr(text, "反") != NULL || strstr(text, "返") != NULL);
        bool is_gate2 = (strstr(text, "2") != NULL || strstr(text, "二") != NULL ||
                         strstr(text, "贰") != NULL || strstr(text, "02") != NULL);
        bool is_gate3 = (strstr(text, "3") != NULL || strstr(text, "三") != NULL ||
                         strstr(text, "叁") != NULL || strstr(text, "03") != NULL);
        bool is_left = (strstr(text, "left") != NULL || strstr(text, "左") != NULL);
        bool is_right = (strstr(text, "right") != NULL || strstr(text, "右") != NULL);

        bool is_gate1 = (strstr(text, "1") != NULL ||
                         strstr(text, "\344\270\200") != NULL ||
                         strstr(text, "\345\243\271") != NULL ||
                         strstr(text, "01") != NULL);
        uint8 gate_number_count = (uint8)is_gate1 + (uint8)is_gate2 + (uint8)is_gate3;

        if (gate_number_count != 1U)
        {
            g_active_voice_cmd = CMD_NONE;
            ips200_show_string(0, 16*5, "Err: Gate No Miss");
            return;
        }

        if (is_gate2)
        {
            g_target_wp_idx = WP_GATE2;
        }
        else if (is_gate3)
        {
            // WP_GATE3_R is named from the outbound viewing direction.
            g_target_wp_idx = ((!is_return && is_right) || (is_return && is_left)) ?
                              WP_GATE3_R : WP_GATE3;
        }
        else if (is_gate1)
        {
            // The outbound-left opening is the return-right opening physically.
            g_target_wp_idx = ((!is_return && is_left) || (is_return && is_right)) ?
                              WP_GATE1_L : WP_GATE1;
        }
        else
        {
            // A lost gate number must not silently select Gate 1.
            g_active_voice_cmd = CMD_NONE;
            ips200_show_string(0, 16*5, "Err: Gate No Miss");
            return;
        }

        g_active_voice_cmd = is_return ? CMD_GATE_RET : CMD_GATE_GO;
    }

    else if (strstr(text, "蛇形前进") != NULL || strstr(text, "蛇行前进") != NULL || strstr(text, "蛇型前进") != NULL ||
             strstr(text, "蛇形前行") != NULL || strstr(text, "S形前进") != NULL || strstr(text, "s形前进") != NULL) g_active_voice_cmd = CMD_MOVE_S_FORWARD;
    else if (strstr(text, "蛇形后退") != NULL || strstr(text, "蛇行后退") != NULL || strstr(text, "蛇型后退") != NULL ||
             strstr(text, "蛇形倒车") != NULL || strstr(text, "S形后退") != NULL || strstr(text, "s形后退") != NULL) g_active_voice_cmd = CMD_MOVE_S_BACKWARD;
    else if (strstr(text, "前行10米") != NULL || strstr(text, "前进10米") != NULL || strstr(text, "前进") != NULL || strstr(text, "前行") != NULL) g_active_voice_cmd = CMD_MOVE_FORWARD_10M;
    else if (strstr(text, "后退10米") != NULL || strstr(text, "倒车10米") != NULL || strstr(text, "后退") != NULL || strstr(text, "倒车") != NULL) g_active_voice_cmd = CMD_MOVE_BACKWARD_10M;
    else if (strstr(text, "逆时针") != NULL || strstr(text, "逆") != NULL)   g_active_voice_cmd = CMD_MOVE_ROTATE_CCW;
    else if (strstr(text, "顺时针") != NULL || strstr(text, "顺") != NULL)   g_active_voice_cmd = CMD_MOVE_ROTATE_CW;
    else if (strstr(text, "左转") != NULL || strstr(text, "向左转弯") != NULL || strstr(text, "向左转") != NULL)     g_active_voice_cmd = CMD_MOVE_LEFT;
    else if (strstr(text, "右转") != NULL || strstr(text, "向右转弯") != NULL || strstr(text, "向右转") != NULL)     g_active_voice_cmd = CMD_MOVE_RIGHT;
    else {
        g_active_voice_cmd = CMD_NONE;
        ips200_show_string(0, 16*5, "Act: No Match!  ");
    }
}


static bool audio_time_string_valid(const char *time_string)
{
    size_t length;

    if(NULL == time_string)
    {
        return false;
    }

    length = strlen(time_string);
    return (length >= 25U) && (length < sizeof(time_now_data)) &&
           (NULL != strstr(time_string, "GMT"));
}

static void audio_drain_pending_wifi_data(void)
{
    for(uint8 attempt = 0; attempt < 2U; attempt++)
    {
        uint32 received = wifi_spi_read_buffer(wifi_spi_get_data_buffer,
                                                sizeof(wifi_spi_get_data_buffer));
        if(0U == received)
        {
            break;
        }
    }
}

static bool audio_refresh_network_time(void)
{
    char candidate_time[sizeof(time_now_data)];

    for(uint8 attempt = 0; attempt < ASR_TIME_RETRY_COUNT; attempt++)
    {
        memset(candidate_time, 0, sizeof(candidate_time));
        if((0 == wifi_spi_get_time(WIFI_SPI_GMT, candidate_time, sizeof(candidate_time))) &&
           audio_time_string_valid(candidate_time))
        {
            memcpy(time_now_data, candidate_time, sizeof(time_now_data));
            audio_time_cache_valid = 1;
            audio_time_cache_ms = g_system_time_ms;
            return true;
        }

        if(0U == attempt)
        {
            audio_drain_pending_wifi_data();
        }
        system_delay_ms(ASR_TIME_RETRY_DELAY_MS);
    }

    if(audio_time_cache_valid &&
       ((uint32)(g_system_time_ms - audio_time_cache_ms) <= ASR_TIME_CACHE_MAX_AGE_MS))
    {
        printf("ASR time request failed, using recent cached GMT\r\n");
        return true;
    }

    return false;
}

// 获取时间戳
static bool time_get_data(void)
{
    if(wifi_spi_init(ASR_WIFI_SSID, ASR_WIFI_PASSWORD))
    {
        return false;
    }

    return audio_refresh_network_time();
}

// 解析语音识别字段
void extract_content_fields(const char* input)
{
    const char* ptr = input;
    char w_value[256];
    while(*ptr)
    {
        // 查找 "w":" 模式
        const char* w_start = strstr(ptr, "\"w\":\"");
        if(!w_start)
        {
            break;
        }
        w_start += 5; // 跳过 "\"w\":\""
        const char* w_end = strchr(w_start, '"');
        if(!w_end)
        {
            break;
        }
        // 提取 w 字段的值
        size_t length = (size_t)(w_end - w_start);
        if(length >= sizeof(w_value))
        {
            length = sizeof(w_value) - 1U;
        }
        memcpy(w_value, w_start, length);
        w_value[length] = '\0';
        // 打印 w 字段的值
        // ======= 【新增】：在串口打印的同时，自动拼装中文字符至全局变量中 =======
        size_t text_length = strlen(g_voice_text);
        size_t text_space = sizeof(g_voice_text) - text_length - 1U;
        size_t append_length = (length < text_space) ? length : text_space;
        if(append_length > 0U)
        {
            memcpy(g_voice_text + text_length, w_value, append_length);
            g_voice_text[text_length + append_length] = '\0';
        }
        printf("%s", w_value);
        ptr = w_end + 1;
    }
}
// 计算websocket url
static bool websocket_get_url(const char* base_url, char* url_out, uint32 url_out_size)
{
    uint8_t APISecret[] = {ASR_APISecret};
    uint8_t APIKey[] = {ASR_APIKey};
    uint8_t indata[128] = {0};
    uint8_t sha256_out[128];
    char base64_out[512];

    char host_domain[256] = {0};
    char path[256] = {0};

    const char* domain_start;

    if((NULL == base_url) || (NULL == url_out) || (0U == url_out_size))
    {
        return false;
    }

    domain_start = strstr(base_url, "://");
    if(domain_start)
    {
        domain_start += 3;
        const char* path_start = strchr(domain_start, '/');
        if(path_start)
        {
            if(((size_t)(path_start - domain_start) >= sizeof(host_domain)) ||
               (strlen(path_start) >= sizeof(path)))
            {
                return false;
            }
            strncpy(host_domain, domain_start, path_start - domain_start);
            host_domain[path_start - domain_start] = '\0';
            strcpy(path, path_start);
        }
    }

    if((host_domain[0] == '\0') || (path[0] == '\0'))
    {
        return false;
    }

    int written = snprintf((char*)indata, sizeof(indata),
                           "host: %s\n"
                           "date: %s\n"
                           "GET %s HTTP/1.1", host_domain, time_now_data, path);
    if((written < 0) || ((uint32)written >= sizeof(indata)))
    {
        return false;
    }
    
    hmac_sha256(APISecret, strlen((char*)APISecret),
                indata, strlen((char*)indata),
                sha256_out);

    base64_encode((uint8_t*)sha256_out, base64_out, SHA256_DIGESTLEN);


    char authorization_origin[512] = {0};
    written = snprintf(authorization_origin, sizeof(authorization_origin),
                       "api_key=\"%s\", algorithm=\"hmac-sha256\", headers=\"host date request-line\", signature=\"%s\"",
                       APIKey, base64_out);
    if((written < 0) || ((uint32)written >= sizeof(authorization_origin)))
    {
        return false;
    }

    base64_encode((uint8_t*)authorization_origin, base64_out, strlen(authorization_origin));

    char encoded_authorization[512];
    char encoded_date[128];
    char encoded_host[256];

    if(!url_encode(base64_out, encoded_authorization, sizeof(encoded_authorization)) ||
       !url_encode(time_now_data, encoded_date, sizeof(encoded_date)) ||
       !url_encode(host_domain, encoded_host, sizeof(encoded_host)))
    {
        return false;
    }
    written = snprintf(url_out, url_out_size, "%s?authorization=%s&date=%s&host=%s",
                       base_url, encoded_authorization, encoded_date, encoded_host);
    if((written < 0) || ((uint32)written >= url_out_size))
    {
        url_out[0] = '\0';
        return false;
    }
//    printf("WebSocket URL: %s \t %d\n", url_out, strlen(url_out));
    return true;
}

static void audio_runtime_reset(void)
{
    audio_start_flag = 0;
    audio_need_net_flag = 0;
    audio_server_link_flag = 0;
    audio_send_data_flag = 0;
    audio_network_error_flag = 0;
    audio_network_error_code = AUDIO_NET_ERR_NONE;
    audio_get_count = -1;
    asr_max_time = 0;
    fifo_clear(&adc_data_fifo);
    memset(websocket_receive_buffer, 0, sizeof(websocket_receive_buffer));
}

static void audio_network_failed(AudioNetworkError_e error_code)
{
    pit_disable(ASR_PIT);
    if(audio_server_link_flag)
    {
        websocket_client_close();
    }
    audio_wifi_reconnect_required = 1;
    audio_runtime_reset();
    audio_network_error_flag = 1;
    audio_network_error_code = error_code;
    pit_enable(ASR_PIT);
    ips200_show_string(0, 16 * 4, "VAD: Net Error  ");
    printf("ASR network error E%d\r\n", (int)error_code);
}

void audio_session_begin(void)
{
    pit_disable(ASR_PIT);
    audio_runtime_reset();
    audio_get_count = 0;
    audio_start_flag = 1;
    pit_enable(ASR_PIT);
}

void audio_session_request_finish(void)
{
    pit_disable(ASR_PIT);
    audio_start_flag = 0;
    audio_send_data_flag = 0;
    audio_need_net_flag = audio_server_link_flag ? 1U : 0U;
}

void audio_session_cancel(void)
{
    pit_disable(ASR_PIT);
    if(audio_server_link_flag)
    {
        websocket_client_close();
    }
    audio_runtime_reset();
}

// 语音识别定时器回调函数
void audio_callback(void)
{
    if(!audio_start_flag || !audio_server_link_flag)
    {
        return;
    }

    adc_get_one_data = (adc_convert(ASR_AUDIO_ADC) - 1870);
    if(FIFO_SUCCESS != fifo_write_buffer(&adc_data_fifo, &adc_get_one_data, 1))
    {
        audio_start_flag = 0;
        audio_send_data_flag = 0;
        audio_need_net_flag = 1;
        return;
    }

    audio_get_count++;
    if((audio_get_count % (ASR_SEND_DATA_MAX_LENTH - 500)) == 0)
    {
        asr_max_time += (ASR_SEND_DATA_MAX_LENTH - 500);
        audio_send_data_flag = 1;
    }

    if(asr_max_time >= (60U * 8000U))
    {
        audio_start_flag = 0;
        audio_send_data_flag = 0;
        audio_need_net_flag = 1;
    }
}

// 语音数据发送
void audio_data_send(uint8 status)
{
    if(status != 0)
    {
        fifo_data_count = fifo_used(&adc_data_fifo);
        if(fifo_data_count > ASR_SEND_DATA_MAX_LENTH)
        {
            fifo_data_count = ASR_SEND_DATA_MAX_LENTH;
        }
        memset(fifo_get_data, 0, sizeof(fifo_get_data));
        fifo_read_buffer(&adc_data_fifo, fifo_get_data, &fifo_data_count, FIFO_READ_AND_CLEAN);
        memset(temp_data, 0, sizeof(temp_data));
        base64_encode((uint8*)fifo_get_data, temp_data, fifo_data_count * 2);
    }
    else
    {
        memset(temp_data, 0, sizeof(temp_data));
    }
    memset(json_data, 0, sizeof(json_data));
    snprintf(json_data, sizeof(json_data),
             "{  \"common\":                                                                                                            \
                        {\"app_id\": \"%s\"},                                                                                           \
                    \"business\":                                                                                                       \
                        {\"domain\": \"iat\", \"language\": \"zh_cn\", \"accent\": \"mandarin\", \"vinfo\": 1, \"vad_eos\": 10000},     \
                    \"data\":                                                                                                           \
                        {\"status\": %d, \"format\": \"audio/L16;rate=8000\", \"audio\": \"%s\", \"encoding\": \"raw\"}}",              \
             ASR_APIID, status, temp_data);
}

// 语音识别处理
void audio_loop()
{
    if(audio_start_flag && !audio_server_link_flag)
    {
        pit_disable(ASR_PIT);

        if(audio_wifi_reconnect_required)
        {
            if(wifi_spi_init(ASR_WIFI_SSID, ASR_WIFI_PASSWORD))
            {
                audio_network_failed(AUDIO_NET_ERR_WIFI_INIT);
                return;
            }
            audio_wifi_reconnect_required = 0;
        }

        if(!audio_refresh_network_time())
        {
            audio_network_failed(AUDIO_NET_ERR_GET_TIME);
            return;
        }

        if(!websocket_get_url(ASR_DOMAIN, asr_url_out, sizeof(asr_url_out)))
        {
            audio_network_failed(AUDIO_NET_ERR_AUTH_URL);
            return;
        }

        ips200_show_string(0, 16*4, "VAD: Connecting ");
        if(!websocket_client_connect(asr_url_out))
        {
            audio_network_failed(AUDIO_NET_ERR_WS_CONNECT);
            return;
        }

        audio_server_link_flag = 1;
        audio_data_send(0);
        if(!websocket_client_send((uint8_t*)json_data, strlen(json_data)))
        {
            audio_network_failed(AUDIO_NET_ERR_START_SEND);
            return;
        }

        printf("服务器连接成功，已开始识别语音，最长识别时长为60秒，再次按下按键停止识别并显示识别结果\r\n");
        printf("语音识别结果:");
        pit_enable(ASR_PIT);
    }

    if(audio_start_flag && audio_server_link_flag && audio_send_data_flag)
    {
        pit_disable(ASR_PIT);
        audio_send_data_flag = 0;
        audio_data_send(1);
        pit_enable(ASR_PIT);

        if(!websocket_client_send((uint8_t*)json_data, strlen(json_data)))
        {
            audio_network_failed(AUDIO_NET_ERR_AUDIO_SEND);
            return;
        }

        if(websocket_client_receive(websocket_receive_buffer, sizeof(websocket_receive_buffer)))
        {
            extract_content_fields((const char*)websocket_receive_buffer);
        }
        return;
    }

    if(audio_need_net_flag && !audio_send_data_flag)
    {
        bool send_success;
        bool receive_success = false;
        AudioNetworkError_e final_error = AUDIO_NET_ERR_NONE;

        pit_disable(ASR_PIT);
        audio_data_send(2);
        send_success = websocket_client_send((uint8_t*)json_data, strlen(json_data));
        if(send_success)
        {
            receive_success = websocket_client_speak_receive(websocket_receive_buffer,
                                                              sizeof(websocket_receive_buffer));
        }

        if(receive_success)
        {
            extract_content_fields((const char*)websocket_receive_buffer);
            voice_command_parse(g_voice_text);
            audio_wifi_reconnect_required = 0;
        }
        else
        {
            g_active_voice_cmd = CMD_NONE;
            audio_wifi_reconnect_required = 1;
            final_error = send_success ? AUDIO_NET_ERR_RESULT_TIMEOUT : AUDIO_NET_ERR_FINAL_SEND;
            ips200_show_string(0, 16 * 4, "VAD: Net Timeout");
        }

        websocket_client_close();
        audio_runtime_reset();
        if(final_error != AUDIO_NET_ERR_NONE)
        {
            audio_network_error_flag = 1;
            audio_network_error_code = final_error;
            printf("ASR network error E%d\r\n", (int)final_error);
        }
        pit_enable(ASR_PIT);
        printf("\r\n语音识别完成，可再次按下按键开始识别\r\n");
        return;
    }

}

// 语音识别初始化
void audio_init()
{
    printf("等待初始化...\r\n");
    adc_init(RANDOM_NUM_ADC, ADC_12BIT);                                                            // 使用ADC生成随机数
    adc_init(ASR_AUDIO_ADC, ADC_12BIT);                                                             // 初始化麦克风ADC
    gpio_init(ASR_BUTTON, GPI, GPIO_HIGH, GPI_PULL_UP);                                             // 初始话录音按键

    if(time_get_data())
    {
        if(!websocket_get_url(ASR_DOMAIN, asr_url_out, sizeof(asr_url_out)))
        {
            audio_wifi_reconnect_required = 1;
        }
    }
    else
    {
        audio_wifi_reconnect_required = 1;
    }

    pit_us_init(ASR_PIT, 125);                                                                      // 开启麦克风接收定时器，频率为8K
    fifo_init(&adc_data_fifo, FIFO_DATA_16BIT, adc_get_data, (ASR_SEND_DATA_MAX_LENTH * 2));        // 初始化音频接收fifo

//    pit_disable(ASR_PIT);                                                                         // 连接wifi前先关闭定时器
//    while(wifi_spi_init(ASR_WIFI_SSID, ASR_WIFI_PASSWORD))                                        // 连接 WiFi 模块到指定的 WiFi 网络
//    {
//        printf("wifi连接失败，开始重连...\r\n");
//        system_delay_ms(500);                                                                     // 初始化失败，等待 500ms
//    }
    pit_enable(ASR_PIT);                                                                            // 连接wifi后打开定时器
    printf("按下P11.3按键开始连接服务器，连接完成后自动开始识别，开始识别后再次按下按键停止识别\r\n");          // 按下按键开始录音识别，最长仅能录制60秒
}



// ======================================================================
// 在 CPU0 上运行的非灯光任务执行函数
// 放入 Core 0 主循环 while(1) 中轮询调用
// ======================================================================
void Sub2_Execute_Task_CPU0(void)
{
    // 如果是灯光和雨刷任务，CPU0 直接忽略，交给 CPU2 处理 [1]
    if (g_active_voice_cmd >= CMD_LIGHT_LEFT && g_active_voice_cmd <= CMD_WIPER) {
        return;
    }

    if (g_active_voice_cmd == CMD_NONE) return;

    g_voice_state = VOICE_STATE_EXECUTING;

    ips200_show_string(0, 16*5, "Act:                ");

    g_start_mileage_task = encoder_get_total_mileage();
    g_start_yaw = imu_ahrs_get_yaw();
    g_start_time = g_system_time_ms;

    g_wp_arrived = 0;
    g_sub2_stage = 0;

    switch ((int)g_active_voice_cmd)
    {
        // ==================== CPU0 负责：鸣笛命令执行 ====================
        case CMD_HORN_1S:
            ips200_show_string(0, 16*5, "Act: Horn 1 Sec ");
            sub2_horn_on(1000);
            system_delay_ms(1000);
            sub2_horn_off();
            break;

        case CMD_HORN_2S:
            ips200_show_string(0, 16*5, "Act: Horn 2 Sec ");
            sub2_horn_on(1000);
            system_delay_ms(2000);
            sub2_horn_off();
            break;

        case CMD_HORN_3S:
            ips200_show_string(0, 16*5, "Act: Horn 3 Sec ");
            sub2_horn_on(1000);
            system_delay_ms(3000);
            sub2_horn_off();
            break;

        case CMD_HORN_DOUBLE:
            ips200_show_string(0, 16*5, "Act: Horn Double");
            sub2_horn_on(1000); system_delay_ms(1000);
            sub2_horn_off();    system_delay_ms(1000);
            sub2_horn_on(1000); system_delay_ms(1000);
            sub2_horn_off();
            break;

        case CMD_HORN_TRIPLE:
            ips200_show_string(0, 16*5, "Act: Horn Triple");
            for(int k=0; k<3; k++) {
                sub2_horn_on(1000); system_delay_ms(1000);
                sub2_horn_off();
                if(k < 2) system_delay_ms(1000);
            }
            break;

        case CMD_HORN_QUAD:
            ips200_show_string(0, 16*5, "Act: Horn Quad  ");
            for(int k=0; k<4; k++) {
                sub2_horn_on(1000); system_delay_ms(1000);
                sub2_horn_off();
                if(k < 3) system_delay_ms(1000);
            }
            break;

        case CMD_HORN_LONG_SHORT:
            ips200_show_string(0, 16*5, "Act: Horn L & S ");
            sub2_horn_on(1000); system_delay_ms(1000);
            sub2_horn_off();    system_delay_ms(1000);
            sub2_horn_on(1000); system_delay_ms(3000);
            sub2_horn_off();
            break;

        case CMD_HORN_URGENT:
            ips200_show_string(0, 16*5, "Act: Horn Urgent");
            for(int k=0; k<5; k++) {
                sub2_horn_on(1500);
                system_delay_ms(500);
                sub2_horn_off();
                system_delay_ms(500);
            }
            break;

        case CMD_HORN_ALARM:
            ips200_show_string(0, 16*5, "Act: Horn Alarm ");
            for(int k=0; k<5; k++) {
                sub2_horn_on(500);
                system_delay_ms(1000);
                sub2_horn_on(1000);
                system_delay_ms(1000);
            }
            sub2_horn_off();
            break;

        case CMD_HORN_MUSIC:
            ips200_show_string(0, 16*5, "Act: 2 Tigers   ");
            play_two_tigers();
            break;

        case CMD_HORN_ZHANG_SN:
            ips200_show_string(0, 16*5, "Act: Nian Zhang ");
            play_remembering_zhang();
            break;
        case CMD_HORN_KOBE:
            ips200_show_string(0, 16*5, "Act: What Can I SAY !");
            play_see_you_again();
            break;
        case CMD_HORN_TRUMP:
            ips200_show_string(0, 16*5, "Act: Trump");
            play_trump_march();
            break;

        // ==================== CPU0 负责：门洞与运动导航配置 ====================
        case CMD_GATE_GO:
            if ((g_sub2_waypoints[g_target_wp_idx].isValid != 1U) ||
                (g_sub2_waypoints[WP_ACTION].isValid != 1U)) {
                 ips200_show_string(0, 16*5, "Err: WP Invalid! ");
                 g_active_voice_cmd = CMD_NONE;
                 current_car_mode = MODE_STOP;
                 return;
            }
            ips200_show_string(0, 16*5, "Act: Go Gate WP:");
            ips200_show_uint(16U * 8U, 16U * 5U, (uint32)g_target_wp_idx, 1U);
            ins_start_waypoint_navigation(g_sub2_waypoints[g_target_wp_idx].x,
                                          g_sub2_waypoints[g_target_wp_idx].y,
                                          g_sub2_waypoints[g_target_wp_idx].yaw);
            g_ins_mode = 1;
            g_sub2_stage = 1;
            current_car_mode = MODE_INS_TRACK;
            break;
        case CMD_GATE_RET: {
            float return_gate_yaw;

            if ((g_sub2_waypoints[g_target_wp_idx].isValid != 1U) ||
                (g_sub2_waypoints[WP_PARK].isValid != 1U)) {
                 ips200_show_string(0, 16*5, "Err: WP Invalid! ");
                 g_active_voice_cmd = CMD_NONE;
                 current_car_mode = MODE_STOP;
                 return;
            }

            return_gate_yaw = g_sub2_waypoints[g_target_wp_idx].yaw + 180.0f;
            if (return_gate_yaw > 180.0f) return_gate_yaw -= 360.0f;
            if (return_gate_yaw <= -180.0f) return_gate_yaw += 360.0f;

            ips200_show_string(0, 16*5, "Act: Return WP: ");
            ips200_show_uint(15U * 8U, 16U * 5U, (uint32)g_target_wp_idx, 1U);
            ins_start_waypoint_navigation(g_sub2_waypoints[g_target_wp_idx].x,
                                          g_sub2_waypoints[g_target_wp_idx].y,
                                          return_gate_yaw);
            g_ins_mode = 1;
            g_sub2_stage = 3;
            current_car_mode = MODE_INS_TRACK;
            break;
        }

        case CMD_MOVE_FORWARD_10M:
            ips200_show_string(0, 16*5, "Act: Foward 10M ");
            ins_start_sub2_task(2);
            current_car_mode = MODE_INS_TRACK;
            break;

        case CMD_MOVE_BACKWARD_10M:
            ips200_show_string(0, 16*5, "Act: Backward10M");
            ins_start_sub2_task(3);
            current_car_mode = MODE_INS_TRACK;
            break;

        case CMD_MOVE_S_FORWARD:
            ips200_show_string(0, 16*5, "Act: S-Forward  ");
            ins_start_sub2_task(4);
            current_car_mode = MODE_INS_TRACK;
            break;

        case CMD_MOVE_S_BACKWARD:
            ips200_show_string(0, 16*5, "Act: S-Backward ");
            ins_start_sub2_task(5);
            current_car_mode = MODE_INS_TRACK;
            break;

        case CMD_MOVE_ROTATE_CCW:
            ips200_show_string(0, 16*5, "Act: Rotate CCW ");
            ins_start_sub2_task(6);
            current_car_mode = MODE_INS_TRACK;
            break;

        case CMD_MOVE_ROTATE_CW:
            ips200_show_string(0, 16*5, "Act: Rotate CW  ");
            ins_start_sub2_task(7);
            current_car_mode = MODE_INS_TRACK;
            break;

        case CMD_MOVE_LEFT:
             ips200_show_string(0, 16*5, "Act: Turn Left  ");
             ins_start_sub2_task(8);
             g_sub2_stage = 1;
             current_car_mode = MODE_INS_TRACK;
             break;

        case CMD_MOVE_RIGHT:
             ips200_show_string(0, 16*5, "Act: Turn Right ");
             ins_start_sub2_task(9);
             g_sub2_stage = 1;
             current_car_mode = MODE_INS_TRACK;
             break;

        default:
            break;
    }

    g_active_voice_cmd = CMD_NONE;
    ips200_show_string(0, 16*5, "Act: Running... ");
}

void Sub2_Execute_Task_CPU2(void)
{
    // 如果不是灯光和雨刷任务，CPU2 直接忽略，留给 CPU0 的运动控制 [1]
    if (g_active_voice_cmd < CMD_LIGHT_LEFT || g_active_voice_cmd > CMD_WIPER) {
        return;
    }

    // 跨核握手锁：由 CPU2 标记状态机进入动作执行中
    g_voice_state = VOICE_STATE_EXECUTING;

    // 将大面积图案阵列静态分配到核 2 局部的 Stack 中，不仅速度最快，而且极大节省了 Core 0 的变量空间 [1]
    static const uint16 arrow_left[7] = {
            0x0018, 0x001C, 0x001E, 0x7FFF, 0x001E, 0x001C, 0x0018
    };

    static const uint16 arrow_right[7] = {
            0x0C00, 0x1C00, 0x3C00, 0x7FFF, 0x3C00, 0x1C00, 0x0C00
    };

    static const uint16 hazard_arrows[7] = {
            0x0C18, 0x1C1C, 0x3C1E, 0x7FFF, 0x3C1E, 0x1C1C, 0x0C18
    };

    static const uint16 beam_high[7] = {
            0x01F0, 0x3D08, 0x0104, 0x3D04, 0x0104, 0x3D08, 0x01F0
    };

    static const uint16 beam_low[7] = {
            0x05F0, 0x0908, 0x1504, 0x0904, 0x1504, 0x0908, 0x05F0
    };

    static const uint16 beam_fog[7] = {
            0x09F0, 0x1D08, 0x0904, 0x1D04, 0x0904, 0x1D08, 0x09F0
    };

    static const uint16 cabin_diamond[7] = {
            0x0080, 0x03E0, 0x0FF8, 0x3FFE, 0x0FF8, 0x03E0, 0x0080
    };

    static const uint16 wiper_frame1[7] = {0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040};
    static const uint16 wiper_frame2[7] = {0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080};
    static const uint16 wiper_frame3[7] = {0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100};

    switch ((int)g_active_voice_cmd)
    {
        case CMD_LIGHT_LEFT:
            dot_matrix_screen_set_brightness(5000);
            for (int k = 0; k < 3; k++) {
                dot_matrix_show_pattern(arrow_left);
                system_delay_ms(450);
                dot_matrix_clear();
                system_delay_ms(300);
            }
            break;

        case CMD_LIGHT_RIGHT:
            dot_matrix_screen_set_brightness(5000);
            for (int k = 0; k < 3; k++) {
                dot_matrix_show_pattern(arrow_right);
                system_delay_ms(450);
                dot_matrix_clear();
                system_delay_ms(300);
            }
            break;

        case CMD_LIGHT_HIGH:
            dot_matrix_screen_set_brightness(8500);
            dot_matrix_show_pattern(beam_high);
            system_delay_ms(1500);
            dot_matrix_clear();
            break;

        case CMD_LIGHT_LOW:
            dot_matrix_screen_set_brightness(3500);
            dot_matrix_show_pattern(beam_low);
            system_delay_ms(1500);
            dot_matrix_clear();
            break;

        case CMD_LIGHT_FOG:
            dot_matrix_screen_set_brightness(6000);
            dot_matrix_show_pattern(beam_fog);
            system_delay_ms(1500);
            dot_matrix_clear();
            break;

        case CMD_LIGHT_DOUBLE:
            dot_matrix_screen_set_brightness(8000);
            for (int k = 0; k < 3; k++) {
                dot_matrix_show_pattern(hazard_arrows);
                system_delay_ms(450);
                dot_matrix_clear();
                system_delay_ms(300);
             }
            break;

        case CMD_LIGHT_INNER:
            dot_matrix_screen_set_brightness(9900);
            dot_matrix_show_pattern(cabin_diamond); // 载入高亮实心菱形图案
            system_delay_ms(1500);
            dot_matrix_clear();
            break;

        case CMD_WIPER:
            dot_matrix_screen_set_brightness(5000);
            for (int k = 0; k < 3; k++) {
                dot_matrix_show_pattern(wiper_frame1); system_delay_ms(150);
                dot_matrix_show_pattern(wiper_frame2); system_delay_ms(120);
                dot_matrix_show_pattern(wiper_frame3); system_delay_ms(150);
                dot_matrix_show_pattern(wiper_frame2); system_delay_ms(120);
            }
            dot_matrix_clear();
            break;

        default:
            break;
    }

    // 执行结束后清空指令，跨核握手：通知 CPU0 的菜单主循环回到收音模式 [2]
    g_active_voice_cmd = CMD_NONE;
}
