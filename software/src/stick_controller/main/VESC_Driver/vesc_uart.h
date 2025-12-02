/**
 * @file vesc_uart.h
 * @brief VESC UART communication driver for ESP-IDF
 * 
 * Ported from Arduino VescUart library for ESP32-S3
 * Based on VESC firmware FW5+
 */

#ifndef VESC_UART_H
#define VESC_UART_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// VESC UART configuration
#define VESC_UART_NUM           UART_NUM_0
#define VESC_UART_BAUD          115200
#define VESC_UART_TX_PIN        43  // ESP32-S3 TX pin (connects to VESC RX)
#define VESC_UART_RX_PIN        44  // ESP32-S3 RX pin (connects to VESC TX)
#define VESC_UART_TIMEOUT_MS    100
#define VESC_UART_BUF_SIZE      256

// Fault codes from VESC
typedef enum {
    VESC_FAULT_NONE = 0,
    VESC_FAULT_OVER_VOLTAGE,
    VESC_FAULT_UNDER_VOLTAGE,
    VESC_FAULT_DRV,
    VESC_FAULT_ABS_OVER_CURRENT,
    VESC_FAULT_OVER_TEMP_FET,
    VESC_FAULT_OVER_TEMP_MOTOR,
    VESC_FAULT_GATE_DRIVER_OVER_VOLTAGE,
    VESC_FAULT_GATE_DRIVER_UNDER_VOLTAGE,
    VESC_FAULT_MCU_UNDER_VOLTAGE,
    VESC_FAULT_BOOTING_FROM_WATCHDOG,
    VESC_FAULT_ENCODER_SPI,
    VESC_FAULT_ENCODER_SINCOS_BELOW_MIN,
    VESC_FAULT_ENCODER_SINCOS_ABOVE_MAX,
    VESC_FAULT_FLASH_CORRUPTION,
    VESC_FAULT_HIGH_OFFSET_CURRENT_1,
    VESC_FAULT_HIGH_OFFSET_CURRENT_2,
    VESC_FAULT_HIGH_OFFSET_CURRENT_3,
    VESC_FAULT_UNBALANCED_CURRENTS,
    VESC_FAULT_BRK,
    VESC_FAULT_RESOLVER_LOT,
    VESC_FAULT_RESOLVER_DOS,
    VESC_FAULT_RESOLVER_LOS,
    VESC_FAULT_FLASH_CORRUPTION_APP,
    VESC_FAULT_FLASH_CORRUPTION_MC,
    VESC_FAULT_ENCODER_NO_MAGNET,
    VESC_FAULT_ENCODER_MAGNET_TOO_STRONG,
    VESC_FAULT_PHASE_FILTER,
} vesc_fault_code_t;

// VESC telemetry data structure
typedef struct {
    float avg_motor_current;    // Average motor current (A)
    float avg_input_current;    // Average input current (A)
    float duty_cycle;           // Duty cycle (0.0 - 1.0)
    float rpm;                  // Motor RPM
    float input_voltage;        // Input voltage (V)
    float amp_hours;            // Amp hours consumed
    float amp_hours_charged;    // Amp hours charged
    float watt_hours;           // Watt hours consumed
    float watt_hours_charged;   // Watt hours charged
    int32_t tachometer;         // Tachometer value
    int32_t tachometer_abs;     // Absolute tachometer value
    float temp_mosfet;          // MOSFET temperature (°C)
    float temp_motor;           // Motor temperature (°C)
    float pid_pos;              // PID position
    uint8_t controller_id;      // VESC controller ID
    vesc_fault_code_t fault;    // Current fault code
} vesc_data_t;

// Firmware version
typedef struct {
    uint8_t major;
    uint8_t minor;
} vesc_fw_version_t;

/**
 * @brief Initialize VESC UART communication
 * @return ESP_OK on success
 */
esp_err_t vesc_uart_init(void);

/**
 * @brief Deinitialize VESC UART
 */
void vesc_uart_deinit(void);

/**
 * @brief Get VESC telemetry values
 * @param data Pointer to structure to fill with telemetry data
 * @return true if successful, false on timeout/error
 */
bool vesc_get_values(vesc_data_t *data);

/**
 * @brief Get VESC firmware version
 * @param fw Pointer to structure to fill with firmware version
 * @return true if successful, false on timeout/error
 */
bool vesc_get_fw_version(vesc_fw_version_t *fw);

/**
 * @brief Set motor current
 * @param current Current in Amps (positive = forward, negative = reverse)
 */
void vesc_set_current(float current);

/**
 * @brief Set brake current
 * @param current Brake current in Amps
 */
void vesc_set_brake_current(float current);

/**
 * @brief Set motor RPM (eRPM = RPM * motor poles)
 * @param rpm Target RPM
 */
void vesc_set_rpm(float rpm);

/**
 * @brief Set motor duty cycle
 * @param duty Duty cycle (0.0 - 1.0)
 */
void vesc_set_duty(float duty);

/**
 * @brief Send keepalive to prevent VESC timeout
 */
void vesc_send_keepalive(void);

/**
 * @brief Get fault code as string
 * @param fault Fault code
 * @return String description of fault
 */
const char* vesc_fault_to_string(vesc_fault_code_t fault);

#ifdef __cplusplus
}
#endif

#endif // VESC_UART_H

