/**
 * @file vesc_uart.c
 * @brief VESC UART communication driver for ESP-IDF
 * 
 * Ported from Arduino VescUart library for ESP32-S3
 */

#include "vesc_uart.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <math.h>

static const char *TAG = "vesc_uart";

// VESC Communication Commands
typedef enum {
    COMM_FW_VERSION = 0,
    COMM_JUMP_TO_BOOTLOADER,
    COMM_ERASE_NEW_APP,
    COMM_WRITE_NEW_APP_DATA,
    COMM_GET_VALUES = 4,
    COMM_SET_DUTY = 5,
    COMM_SET_CURRENT = 6,
    COMM_SET_CURRENT_BRAKE = 7,
    COMM_SET_RPM = 8,
    COMM_SET_POS = 9,
    COMM_SET_HANDBRAKE = 10,
    COMM_ALIVE = 30,
    COMM_FORWARD_CAN = 34,
} vesc_comm_packet_id_t;

// CRC16 lookup table
static const uint16_t crc16_tab[] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

// Buffer helper functions
static void buffer_append_int32(uint8_t *buffer, int32_t number, int32_t *index) {
    buffer[(*index)++] = (uint8_t)(number >> 24);
    buffer[(*index)++] = (uint8_t)(number >> 16);
    buffer[(*index)++] = (uint8_t)(number >> 8);
    buffer[(*index)++] = (uint8_t)(number);
}

static int16_t buffer_get_int16(const uint8_t *buffer, int32_t *index) {
    int16_t res = ((uint16_t)buffer[*index]) << 8 | ((uint16_t)buffer[*index + 1]);
    *index += 2;
    return res;
}

static int32_t buffer_get_int32(const uint8_t *buffer, int32_t *index) {
    int32_t res = ((uint32_t)buffer[*index]) << 24 |
                  ((uint32_t)buffer[*index + 1]) << 16 |
                  ((uint32_t)buffer[*index + 2]) << 8 |
                  ((uint32_t)buffer[*index + 3]);
    *index += 4;
    return res;
}

static float buffer_get_float16(const uint8_t *buffer, float scale, int32_t *index) {
    return (float)buffer_get_int16(buffer, index) / scale;
}

static float buffer_get_float32(const uint8_t *buffer, float scale, int32_t *index) {
    return (float)buffer_get_int32(buffer, index) / scale;
}

static uint16_t crc16(const uint8_t *buf, uint16_t len) {
    uint16_t cksum = 0;
    for (uint16_t i = 0; i < len; i++) {
        cksum = crc16_tab[(((cksum >> 8) ^ buf[i]) & 0xFF)] ^ (cksum << 8);
    }
    return cksum;
}

// Send payload with framing
static int vesc_pack_send_payload(const uint8_t *payload, int len_pay) {
    uint16_t crc_payload = crc16(payload, len_pay);
    uint8_t message[VESC_UART_BUF_SIZE];
    int count = 0;

    if (len_pay <= 256) {
        message[count++] = 2;
        message[count++] = (uint8_t)len_pay;
    } else {
        message[count++] = 3;
        message[count++] = (uint8_t)(len_pay >> 8);
        message[count++] = (uint8_t)(len_pay & 0xFF);
    }

    memcpy(message + count, payload, len_pay);
    count += len_pay;

    message[count++] = (uint8_t)(crc_payload >> 8);
    message[count++] = (uint8_t)(crc_payload & 0xFF);
    message[count++] = 3;

    return uart_write_bytes(VESC_UART_NUM, message, count);
}

// Receive and unpack UART message
static int vesc_receive_uart_message(uint8_t *payload_received) {
    uint8_t message[VESC_UART_BUF_SIZE];
    uint16_t counter = 0;
    uint16_t end_message = 256;
    uint16_t len_payload = 0;
    bool message_read = false;

    int64_t start_time = esp_timer_get_time();
    int64_t timeout_us = VESC_UART_TIMEOUT_MS * 1000;

    while ((esp_timer_get_time() - start_time) < timeout_us && !message_read) {
        int len = uart_read_bytes(VESC_UART_NUM, message + counter, 1, pdMS_TO_TICKS(10));
        if (len > 0) {
            counter++;

            if (counter == 2) {
                switch (message[0]) {
                    case 2:
                        end_message = message[1] + 5;
                        len_payload = message[1];
                        break;
                    case 3:
                        ESP_LOGW(TAG, "Message > 256 bytes not supported");
                        break;
                    default:
                        ESP_LOGW(TAG, "Invalid start byte: %d", message[0]);
                        break;
                }
            }

            if (counter >= sizeof(message)) {
                break;
            }

            if (counter == end_message && message[end_message - 1] == 3) {
                message_read = true;
                break;
            }
        }
    }

    if (!message_read) {
        ESP_LOGD(TAG, "VESC UART timeout");
        return 0;
    }

    // Verify CRC
    uint16_t crc_message = (message[end_message - 3] << 8) | message[end_message - 2];
    memcpy(payload_received, &message[2], message[1]);
    uint16_t crc_payload = crc16(payload_received, message[1]);

    if (crc_payload == crc_message) {
        return len_payload;
    } else {
        ESP_LOGW(TAG, "CRC mismatch: got %04X, expected %04X", crc_payload, crc_message);
        return 0;
    }
}

// Public API implementation

esp_err_t vesc_uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = VESC_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret;
    
    ret = uart_param_config(VESC_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_set_pin(VESC_UART_NUM, VESC_UART_TX_PIN, VESC_UART_RX_PIN, 
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = uart_driver_install(VESC_UART_NUM, VESC_UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "VESC UART initialized on TX:%d RX:%d @ %d baud", 
             VESC_UART_TX_PIN, VESC_UART_RX_PIN, VESC_UART_BAUD);
    return ESP_OK;
}

void vesc_uart_deinit(void) {
    uart_driver_delete(VESC_UART_NUM);
}

bool vesc_get_values(vesc_data_t *data) {
    if (data == NULL) return false;

    uint8_t payload[1] = { COMM_GET_VALUES };
    vesc_pack_send_payload(payload, 1);

    uint8_t message[VESC_UART_BUF_SIZE];
    int msg_len = vesc_receive_uart_message(message);

    if (msg_len > 55) {
        // Parse response - skip packet ID
        int32_t index = 1;
        
        data->temp_mosfet       = buffer_get_float16(message, 10.0f, &index);
        data->temp_motor        = buffer_get_float16(message, 10.0f, &index);
        data->avg_motor_current = buffer_get_float32(message, 100.0f, &index);
        data->avg_input_current = buffer_get_float32(message, 100.0f, &index);
        index += 4; // Skip avg_id
        index += 4; // Skip avg_iq
        data->duty_cycle        = buffer_get_float16(message, 1000.0f, &index);
        data->rpm               = buffer_get_float32(message, 1.0f, &index);
        data->input_voltage     = buffer_get_float16(message, 10.0f, &index);
        data->amp_hours         = buffer_get_float32(message, 10000.0f, &index);
        data->amp_hours_charged = buffer_get_float32(message, 10000.0f, &index);
        data->watt_hours        = buffer_get_float32(message, 10000.0f, &index);
        data->watt_hours_charged= buffer_get_float32(message, 10000.0f, &index);
        data->tachometer        = buffer_get_int32(message, &index);
        data->tachometer_abs    = buffer_get_int32(message, &index);
        data->fault             = (vesc_fault_code_t)message[index++];
        data->pid_pos           = buffer_get_float32(message, 1000000.0f, &index);
        data->controller_id     = message[index++];

        return true;
    }

    return false;
}

bool vesc_get_fw_version(vesc_fw_version_t *fw) {
    if (fw == NULL) return false;

    uint8_t payload[1] = { COMM_FW_VERSION };
    vesc_pack_send_payload(payload, 1);

    uint8_t message[VESC_UART_BUF_SIZE];
    int msg_len = vesc_receive_uart_message(message);

    if (msg_len > 0 && message[0] == COMM_FW_VERSION) {
        fw->major = message[1];
        fw->minor = message[2];
        return true;
    }

    return false;
}

void vesc_set_current(float current) {
    uint8_t payload[5];
    int32_t index = 0;
    
    payload[index++] = COMM_SET_CURRENT;
    buffer_append_int32(payload, (int32_t)(current * 1000.0f), &index);
    
    vesc_pack_send_payload(payload, 5);
}

void vesc_set_brake_current(float current) {
    uint8_t payload[5];
    int32_t index = 0;
    
    payload[index++] = COMM_SET_CURRENT_BRAKE;
    buffer_append_int32(payload, (int32_t)(current * 1000.0f), &index);
    
    vesc_pack_send_payload(payload, 5);
}

void vesc_set_rpm(float rpm) {
    uint8_t payload[5];
    int32_t index = 0;
    
    payload[index++] = COMM_SET_RPM;
    buffer_append_int32(payload, (int32_t)rpm, &index);
    
    vesc_pack_send_payload(payload, 5);
}

void vesc_set_duty(float duty) {
    uint8_t payload[5];
    int32_t index = 0;
    
    payload[index++] = COMM_SET_DUTY;
    buffer_append_int32(payload, (int32_t)(duty * 100000.0f), &index);
    
    vesc_pack_send_payload(payload, 5);
}

void vesc_send_keepalive(void) {
    uint8_t payload[1] = { COMM_ALIVE };
    vesc_pack_send_payload(payload, 1);
}

const char* vesc_fault_to_string(vesc_fault_code_t fault) {
    switch (fault) {
        case VESC_FAULT_NONE:                   return "None";
        case VESC_FAULT_OVER_VOLTAGE:           return "Over Voltage";
        case VESC_FAULT_UNDER_VOLTAGE:          return "Under Voltage";
        case VESC_FAULT_DRV:                    return "DRV Fault";
        case VESC_FAULT_ABS_OVER_CURRENT:       return "Over Current";
        case VESC_FAULT_OVER_TEMP_FET:          return "FET Over Temp";
        case VESC_FAULT_OVER_TEMP_MOTOR:        return "Motor Over Temp";
        case VESC_FAULT_GATE_DRIVER_OVER_VOLTAGE: return "Gate OV";
        case VESC_FAULT_GATE_DRIVER_UNDER_VOLTAGE: return "Gate UV";
        case VESC_FAULT_MCU_UNDER_VOLTAGE:      return "MCU UV";
        case VESC_FAULT_BOOTING_FROM_WATCHDOG:  return "Watchdog Reset";
        case VESC_FAULT_ENCODER_SPI:            return "Encoder SPI";
        default:                                return "Unknown";
    }
}

