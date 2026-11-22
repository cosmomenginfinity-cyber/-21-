#include "zf_common_headfile.h"
#include "websocket_client.h"
#include "sha1.h"
#include "base64.h"
#include "asr_ctrl.h"

extern char temp_data[WEBSOCKET_SEND_BUFFER_SIZE];

#define WEBSOCKET_FINAL_TIMEOUT_MS    (8000U)
#define WEBSOCKET_MAX_FRAME_OVERHEAD  (14U)

bool url_encode(const char* str, char* encoded, uint32_t encoded_size)
{
    const char* hex_chars = "0123456789ABCDEF";
    size_t len;
    char* ptr = encoded;

    if((NULL == str) || (NULL == encoded) || (0U == encoded_size))
    {
        return false;
    }

    len = strlen(str);

    for(size_t i = 0; i < len; i++)
    {
        unsigned char c = str[i];
        uint32_t required = 1U;

        if(!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
             (c >= 'a' && c <= 'z') || c == '-' || c == '_' || c == '.' || c == '~') && c != ' ')
        {
            required = 3U;
        }
        if(((uint32_t)(ptr - encoded) + required) >= encoded_size)
        {
            encoded[0] = '\0';
            return false;
        }

        if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-' || c == '_' || c == '.' || c == '~')
        {
            *ptr++ = c;
        }
        else if(c == ' ')
        {
            *ptr++ = '+';
        }
        else
        {
            *ptr++ = '%';
            *ptr++ = hex_chars[(c >> 4) & 0x0F];
            *ptr++ = hex_chars[c & 0x0F];
        }
    }
    *ptr = '\0';
    return true;
}

static void create_handshake_request(const char* host, const char* path, const char* key, char* request)
{
    snprintf(request, 1024,
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Key: %s\r\n"
             "Sec-WebSocket-Version: 13\r\n\r\n",
             path, host, key);
}

static bool parse_handshake_response(const char* response, const char* expected_accept_key)
{
    const char* accept_header = "Sec-WebSocket-Accept: ";
    const char* pos = strstr(response, accept_header);
    if(!pos) return false;

    pos += strlen(accept_header);
    const char* end = strchr(pos, '\r');
    if(!end) return false;

    size_t accept_length = (size_t)(end - pos);
    char accept_key[128];
    if(accept_length >= sizeof(accept_key))
    {
        return false;
    }
    memcpy(accept_key, pos, accept_length);
    accept_key[accept_length] = '\0';

    return strcmp(accept_key, expected_accept_key) == 0;
}

bool websocket_client_connect(const char* url)
{
    const char* protocol = "wss://";
    if(strncmp(url, protocol, strlen(protocol)) != 0)
    {
        return false;
    }

    const char* host_start = url + strlen(protocol);
    const char* path_start = strchr(host_start, '/');
    if(!path_start)
    {
        return false;
    }

    char host[128], path[512];
    char hostname[256];
    memset(host, 0, sizeof(host));
    memset(path, 0, sizeof(path));
    memset(hostname, 0, sizeof(hostname));
    strncpy(host, host_start, path_start - host_start);
    host[path_start - host_start] = '\0';
    strcpy(path, path_start);

    const char* port_pos = strchr(host, ':');
    if(port_pos)
    {
        strncpy(hostname, host, port_pos - host);
        hostname[port_pos - host] = '\0';
    }
    else
    {
        strcpy(hostname, host);
    }

//    printf("hostname:%s\r\n",hostname);
    if(wifi_spi_socket_connect("TCP", hostname, "80", "6060"))
    {
        //printf("Failed to connect to TCP server");
        return false;
    }
    //printf("TCP server connected\r\n");

    char sec_websocket_key[32] = "dGhlIHNhbXBsZSBub25jZQ==";
    memset(sec_websocket_key, 0, sizeof(sec_websocket_key));
    uint8_t random_bytes[16];
    for(int i = 0; i < 16; i++) random_bytes[i] = rand() % 256;
    base64_encode(random_bytes, sec_websocket_key, 16);

    const char* guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char combined_key[512];
    memset(combined_key, 0, sizeof(combined_key));
    snprintf(combined_key, sizeof(combined_key), "%s%s", sec_websocket_key, guid);

    uint8_t hash[SHA1_HASH_SIZE];
    memset(hash, 0, sizeof(hash));
    sha1_hash((const uint8_t*)combined_key, strlen(combined_key), hash);

    char sec_websocket_accept[128];
    memset(sec_websocket_accept, 0, sizeof(sec_websocket_accept));
    base64_encode(hash, sec_websocket_accept, SHA1_HASH_SIZE);

    char handshake_request[1024];
    memset(handshake_request, 0, sizeof(handshake_request));

    create_handshake_request(hostname, path, sec_websocket_key, handshake_request);

    if(wifi_spi_send_buffer((uint8_t*)handshake_request, strlen(handshake_request)) != 0)
    {
        wifi_spi_socket_disconnect();
        return false;
    }
//    system_delay_ms(500);
    uint8_t handshake_response[1024];
    memset(handshake_response, 0, sizeof(handshake_response));
//    uint32_t bytes_received = wifi_spi_read_buffer(handshake_response, sizeof(handshake_response));

    uint32_t bytes_received = 0;
    uint8 time_out = 0;
    while (1)
    {
        uint32_t received;

        time_out++;
        if(time_out > 3U)break;
        received = wifi_spi_read_buffer(handshake_response + bytes_received,
                                        sizeof(handshake_response) - bytes_received - 1U);
        bytes_received += received;
        handshake_response[bytes_received] = '\0';
        if(strstr((const char*)handshake_response, "\r\n\r\n") != NULL)break;
        if(bytes_received >= (sizeof(handshake_response) - 1U))break;
        system_delay_ms(100);
    }

    if(bytes_received <= 0)
    {
        printf("Handshake receive failed");
        wifi_spi_socket_disconnect();
        return false;
    }
    handshake_response[bytes_received] = '\0';

    if(!parse_handshake_response((char*)handshake_response, sec_websocket_accept))
    {
        wifi_spi_socket_disconnect();
        return false;
    }

    return true;
}

bool websocket_client_send(const uint8_t* data, uint32_t len)
{
    if((NULL == data) || (len > (WEBSOCKET_SEND_BUFFER_SIZE - WEBSOCKET_MAX_FRAME_OVERHEAD)))
    {
        return false;
    }

    memset(temp_data, 0, WEBSOCKET_SEND_BUFFER_SIZE);
    uint32_t frame_len = websocket_create_frame((uint8*)temp_data, data, len, 1, 1);

    return wifi_spi_send_buffer((uint8*)temp_data, frame_len) == 0;
}

bool websocket_client_receive(uint8_t* buffer, uint32_t buffer_size)
{
    uint32_t bytes_received;

    if((NULL == buffer) || (buffer_size < 2U))
    {
        return false;
    }

    memset(buffer, 0, buffer_size);
    bytes_received = wifi_spi_read_buffer(buffer, buffer_size - 1U);
    buffer[bytes_received] = '\0';

    for(uint32_t i = 0; i < bytes_received; i++)
    {
        if(buffer[i] == 0)
        {
            buffer[i] = ' ';
        }
    }

    return (bytes_received > 0U);
}
bool websocket_client_speak_receive(uint8_t* buffer, uint32_t buffer_size)
{
    uint32_t bytes_received = 0;

    if((NULL == buffer) || (buffer_size < 2U))
    {
        return false;
    }

    memset(buffer, 0, buffer_size);
    system_start();
    while(system_getval_ms() < WEBSOCKET_FINAL_TIMEOUT_MS)
    {
        uint32_t remaining = buffer_size - bytes_received - 1U;
        uint32_t received;

        if(0U == remaining)
        {
            return false;
        }

        received = wifi_spi_read_buffer(buffer + bytes_received, remaining);
        if(received > 0)
        {
            for(uint32_t i = bytes_received; i < (bytes_received + received); i++)
            {
                if(buffer[i] == 0)
                {
                    buffer[i] = ' ';
                }
            }
            bytes_received += received;
            buffer[bytes_received] = '\0';

            if (strstr((const char*)(buffer), "\"status\":2") != NULL)
            {
                return true;
            }
        }
    }

    return false;
}

void websocket_client_close(void)
{
    wifi_spi_socket_disconnect();
}

uint32_t websocket_create_frame(uint8_t* frame, const uint8_t* payload, uint64_t payload_len, uint8_t type, bool mask)
{
    uint8_t* ptr = frame;
    static uint8_t mask_key[4] = {0};
//    uint64_t masked_payload_len = payload_len;

    *ptr++ = 0x80 | (type & 0x0F);

    if(mask && (mask_key[0] + mask_key[1] + mask_key[2] + mask_key[3]) == 0)
    {

        for(int i = 0; i < 4; i++)
        {
            mask_key[i] = adc_convert(RANDOM_NUM_ADC) % 256;
        }
    }
    if(payload_len <= 125)
    {
        *ptr++ = (uint8_t)(payload_len | (mask ? 0x80 : 0x00));
    }
    else if(payload_len <= 65535)
    {
        *ptr++ = 126 | (mask ? 0x80 : 0x00);
        *ptr++ = (uint8_t)((payload_len >> 8) & 0xFF);
        *ptr++ = (uint8_t)(payload_len & 0xFF);
    }
    else
    {
        *ptr++ = 127 | (mask ? 0x80 : 0x00);

        for(int i = 7; i >= 0; i--)
        {
            *ptr++ = (uint8_t)((payload_len >> (i * 8)) & 0xFF);
        }
    }

    if(mask)
    {
        memcpy(ptr, mask_key, 4);
        ptr += 4;
    }

    uint8_t* payload_start = ptr;
    for(uint64_t i = 0; i < payload_len; i++)
    {
        if(mask)
        {
            payload_start[i] = payload[i] ^ mask_key[i % 4];
        }
        else
        {
            payload_start[i] = payload[i];
        }
    }
    return (uint32_t)((payload_start - frame) + payload_len);
}
