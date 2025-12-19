/**
 * @file main.c
 * @brief Death Stick Controller - Main Application
 * 
 * Controls a VESC motor controller via UART with three speed levels
 * and displays telemetry on the built-in LCD with background image.
 * 
 * Speed Buttons (active LOW with internal pull-ups):
 *   GP2 - SLOW:   Low current
 *   GP3 - MEDIUM: Medium current
 *   GP4 - FAST:   High current
 * 
 * VESC Communication:
 *   UART0 at 115200 baud
 *   TX -> VESC RX (Yellow wire)
 *   RX -> VESC TX (White wire)
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"

#include "LCD_Driver/ST7789.h"
#include "LVGL_Driver/LVGL_Driver.h"
#include "Button_Driver/Button_Driver.h"
#include "Button_Driver/Speed_Buttons.h"
#include "VESC_Driver/vesc_uart.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "stick_controller";

// =============================================================================
// Background Image (rotated 90° for landscape view)
// =============================================================================

// Original portrait background image (172 wide x 320 tall)
extern const lv_img_dsc_t dark_retro_sea_small;

// Rotated image buffer and descriptor
static lv_img_dsc_t bg_img_rotated;
static uint16_t bg_img_rotated_buf[EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES];

// Rotate image 90 degrees clockwise
static void rotate_bg_90_cw(void) {
    const lv_img_dsc_t *src = &dark_retro_sea_small;
    const uint16_t *src16 = (const uint16_t *)src->data;
    uint16_t *dst16 = bg_img_rotated_buf;
    uint16_t src_w = src->header.w;
    uint16_t src_h = src->header.h;
    uint16_t dst_w = src_h;
    
    for (uint16_t y = 0; y < src_h; y++) {
        for (uint16_t x = 0; x < src_w; x++) {
            uint32_t src_idx = (uint32_t)y * src_w + x;
            uint16_t new_x = (uint16_t)(src_h - 1 - y);
            uint16_t new_y = x;
            uint32_t dst_idx = (uint32_t)new_y * dst_w + new_x;
            dst16[dst_idx] = src16[src_idx];
        }
    }
    
    bg_img_rotated.header.always_zero = 0;
    bg_img_rotated.header.w = src_h;
    bg_img_rotated.header.h = src_w;
    bg_img_rotated.header.cf = LV_IMG_CF_TRUE_COLOR;
    bg_img_rotated.data_size = (uint32_t)src_w * src_h * 2;
    bg_img_rotated.data = (const uint8_t *)bg_img_rotated_buf;
}

// =============================================================================
// MOTOR CURRENT SETTINGS (Adjustable)
// =============================================================================

#define CURRENT_SLOW    10.0f   // Low power mode (Amps)
#define CURRENT_MEDIUM  30.0f   // Normal cruising (Amps)
#define CURRENT_FAST    70.0f   // Full power (Amps)

#define VESC_POLL_INTERVAL_MS   200

// =============================================================================
// UI Elements
// =============================================================================

static lv_obj_t *bg_img_obj = NULL;
static lv_obj_t *lbl_title = NULL;
static lv_obj_t *lbl_voltage = NULL;
static lv_obj_t *lbl_current = NULL;
static lv_obj_t *lbl_amp_hours = NULL;
static lv_obj_t *lbl_rpm = NULL;
static lv_obj_t *lbl_speed_level = NULL;
static lv_obj_t *lbl_fault = NULL;
static lv_obj_t *lbl_temp = NULL;
static lv_obj_t *lbl_emergency = NULL;

static speed_level_t commanded_speed = SPEED_LEVEL_OFF;
static float commanded_current = 0.0f;
static vesc_data_t vesc_data = {0};
static bool vesc_connected = false;
static bool emergency_stop_active = false;

// =============================================================================
// UI Creation - Portrait layout with rotated background
// Screen: 172 wide x 320 tall (portrait)
// =============================================================================

static void ui_create(void) {
    // Rotate and display background image
    rotate_bg_90_cw();
    bg_img_obj = lv_img_create(lv_scr_act());
    lv_img_set_src(bg_img_obj, &bg_img_rotated);
    lv_obj_align(bg_img_obj, LV_ALIGN_TOP_LEFT, 0, 0);

    // Semi-transparent overlay for text readability
    lv_obj_t *overlay = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, 172, 320);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_40, 0);
    lv_obj_align(overlay, LV_ALIGN_TOP_LEFT, 0, 0);

    // Styles - using larger fonts (16 for data, 18 for title/speed)
    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0x00FFC8));
    lv_style_set_text_font(&style_title, &lv_font_montserrat_18);

    static lv_style_t style_data;
    lv_style_init(&style_data);
    lv_style_set_text_color(&style_data, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_data, &lv_font_montserrat_16);

    static lv_style_t style_speed;
    lv_style_init(&style_speed);
    lv_style_set_text_color(&style_speed, lv_color_hex(0xFFD700));
    lv_style_set_text_font(&style_speed, &lv_font_montserrat_18);

    static lv_style_t style_fault;
    lv_style_init(&style_fault);
    lv_style_set_text_color(&style_fault, lv_color_hex(0xFF4444));
    lv_style_set_text_font(&style_fault, &lv_font_montserrat_16);

    static lv_style_t style_emergency;
    lv_style_init(&style_emergency);
    lv_style_set_text_color(&style_emergency, lv_color_hex(0xFF3333));
    lv_style_set_text_font(&style_emergency, &lv_font_montserrat_18);

    // ==========================================================================
    // PORTRAIT LAYOUT (172 x 320) - Single column, vertically stacked
    // ==========================================================================
    int y = 8;
    int line_height = 26;  // Larger for bigger fonts
    int x_margin = 5;

    // Title
    lbl_title = lv_label_create(lv_scr_act());
    lv_obj_add_style(lbl_title, &style_title, 0);
    lv_label_set_text(lbl_title, "DEATH STICK");
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, y);
    y += line_height + 2;

    // Speed Level (prominent)
    lbl_speed_level = lv_label_create(lv_scr_act());
    lv_obj_add_style(lbl_speed_level, &style_speed, 0);
    lv_label_set_text(lbl_speed_level, "[ OFF ]");
    lv_obj_align(lbl_speed_level, LV_ALIGN_TOP_MID, 0, y);
    y += line_height + 8;

    // Emergency status (hidden until active)
    lbl_emergency = lv_label_create(lv_scr_act());
    lv_obj_add_style(lbl_emergency, &style_emergency, 0);
    lv_label_set_text(lbl_emergency, "");
    lv_obj_align(lbl_emergency, LV_ALIGN_TOP_MID, 0, y);
    y += line_height + 2;

    // Voltage
    lbl_voltage = lv_label_create(lv_scr_act());
    lv_obj_add_style(lbl_voltage, &style_data, 0);
    lv_label_set_text(lbl_voltage, "VOLT: --.- V");
    lv_obj_align(lbl_voltage, LV_ALIGN_TOP_LEFT, x_margin, y);
    y += line_height;

    // Current
    lbl_current = lv_label_create(lv_scr_act());
    lv_obj_add_style(lbl_current, &style_data, 0);
    lv_label_set_text(lbl_current, "AMPS: --.- A");
    lv_obj_align(lbl_current, LV_ALIGN_TOP_LEFT, x_margin, y);
    y += line_height;

    // Amp Hours
    lbl_amp_hours = lv_label_create(lv_scr_act());
    lv_obj_add_style(lbl_amp_hours, &style_data, 0);
    lv_label_set_text(lbl_amp_hours, "Ah: --.-- Ah");
    lv_obj_align(lbl_amp_hours, LV_ALIGN_TOP_LEFT, x_margin, y);
    y += line_height;

    // RPM
    lbl_rpm = lv_label_create(lv_scr_act());
    lv_obj_add_style(lbl_rpm, &style_data, 0);
    lv_label_set_text(lbl_rpm, "RPM: -----");
    lv_obj_align(lbl_rpm, LV_ALIGN_TOP_LEFT, x_margin, y);
    y += line_height;

    // Temperature
    lbl_temp = lv_label_create(lv_scr_act());
    lv_obj_add_style(lbl_temp, &style_data, 0);
    lv_label_set_text(lbl_temp, "TEMP: --.- C");
    lv_obj_align(lbl_temp, LV_ALIGN_TOP_LEFT, x_margin, y);

    // Fault Status - at bottom
    lbl_fault = lv_label_create(lv_scr_act());
    lv_obj_add_style(lbl_fault, &style_fault, 0);
    lv_label_set_text(lbl_fault, "VESC: ---");
    lv_obj_align(lbl_fault, LV_ALIGN_BOTTOM_MID, 0, -8);
}

static void ui_update(void) {
    char buf[32];

    // Speed level with brackets for visibility
    const char* speed_str = speed_level_to_string(commanded_speed);
    snprintf(buf, sizeof(buf), "[ %s ]", speed_str);
    lv_label_set_text(lbl_speed_level, buf);

    // Speed label color
    lv_color_t speed_color;
    switch (commanded_speed) {
        case SPEED_LEVEL_OFF:    speed_color = lv_color_hex(0x888888); break;
        case SPEED_LEVEL_SLOW:   speed_color = lv_color_hex(0x44FF44); break;
        case SPEED_LEVEL_MEDIUM: speed_color = lv_color_hex(0xFFD700); break;
        case SPEED_LEVEL_FAST:   speed_color = lv_color_hex(0xFF4444); break;
        default:                 speed_color = lv_color_hex(0xFFFFFF); break;
    }
    lv_obj_set_style_text_color(lbl_speed_level, speed_color, 0);

    if (emergency_stop_active) {
        lv_label_set_text(lbl_emergency, "EMERGENCY STOP");
    } else {
        lv_label_set_text(lbl_emergency, "");
    }

    if (vesc_connected) {
        snprintf(buf, sizeof(buf), "VOLT: %.1f V", vesc_data.input_voltage);
        lv_label_set_text(lbl_voltage, buf);

        snprintf(buf, sizeof(buf), "AMPS: %.1f A", vesc_data.avg_motor_current);
        lv_label_set_text(lbl_current, buf);

        snprintf(buf, sizeof(buf), "Ah: %.2f Ah", vesc_data.amp_hours);
        lv_label_set_text(lbl_amp_hours, buf);

        snprintf(buf, sizeof(buf), "RPM: %.0f", vesc_data.rpm);
        lv_label_set_text(lbl_rpm, buf);

        snprintf(buf, sizeof(buf), "TEMP: %.1f C", vesc_data.temp_mosfet);
        lv_label_set_text(lbl_temp, buf);

        if (vesc_data.fault == VESC_FAULT_NONE) {
            lv_label_set_text(lbl_fault, "VESC: OK");
            lv_obj_set_style_text_color(lbl_fault, lv_color_hex(0x44FF44), 0);
        } else {
            snprintf(buf, sizeof(buf), "%s", vesc_fault_to_string(vesc_data.fault));
            lv_label_set_text(lbl_fault, buf);
            lv_obj_set_style_text_color(lbl_fault, lv_color_hex(0xFF4444), 0);
        }
    } else {
        lv_label_set_text(lbl_voltage, "VOLT: --.- V");
        lv_label_set_text(lbl_current, "AMPS: --.- A");
        lv_label_set_text(lbl_amp_hours, "Ah: --.-- Ah");
        lv_label_set_text(lbl_rpm, "RPM: -----");
        lv_label_set_text(lbl_temp, "TEMP: --.- C");
        lv_label_set_text(lbl_fault, "NO VESC");
        lv_obj_set_style_text_color(lbl_fault, lv_color_hex(0xFF8800), 0);
    }
}

// =============================================================================
// Motor Control
// =============================================================================

static float get_current_for_speed_level(speed_level_t level) {
    switch (level) {
        case SPEED_LEVEL_SLOW:   return CURRENT_SLOW;
        case SPEED_LEVEL_MEDIUM: return CURRENT_MEDIUM;
        case SPEED_LEVEL_FAST:   return CURRENT_FAST;
        default:                 return 0.0f;
    }
}

static void apply_motor_current(float current) {
    commanded_current = current;
    if (current > 0.1f) {
        vesc_set_current(current);
    } else {
        vesc_set_current(0.0f);
    }
}

static void enter_emergency_stop(void) {
    emergency_stop_active = true;
    commanded_speed = SPEED_LEVEL_OFF;
    apply_motor_current(0.0f);
    speed_buttons_set_all_leds(false);
    ESP_LOGW(TAG, "EMERGENCY STOP ACTIVATED");
}

static void exit_emergency_stop(speed_level_t *last_speed_level) {
    emergency_stop_active = false;
    commanded_speed = SPEED_LEVEL_OFF;
    if (last_speed_level) {
        *last_speed_level = SPEED_LEVEL_OFF;
    }
    speed_buttons_set_leds(commanded_speed);
    ESP_LOGI(TAG, "Emergency stop cleared");
}

// =============================================================================
// Tasks
// =============================================================================

static void vesc_task(void *arg) {
    (void)arg;
    TickType_t last_wake = xTaskGetTickCount();
    
    while (1) {
        if (vesc_get_values(&vesc_data)) {
            vesc_connected = true;
        } else {
            vesc_connected = false;
        }

        if (vesc_connected) {
            vesc_send_keepalive();
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(VESC_POLL_INTERVAL_MS));
    }
}

// MOMENTARY: Hold button = motor runs, release = motor stops
static void control_task(void *arg) {
    (void)arg;
    speed_level_t last_speed_level = SPEED_LEVEL_OFF;
    TickType_t all_pressed_start = 0;
    bool tracking_all_pressed = false;
    TickType_t blink_last_toggle = 0;
    bool blink_state = false;
    bool prev_slow = false;
    bool prev_medium = false;
    bool prev_fast = false;

    typedef enum {
        EXIT_WAIT_SLOW_PRESS = 0,
        EXIT_WAIT_SLOW_RELEASE,
        EXIT_WAIT_MEDIUM_PRESS,
        EXIT_WAIT_MEDIUM_RELEASE,
        EXIT_WAIT_FAST_PRESS,
        EXIT_WAIT_FAST_RELEASE
    } emergency_exit_state_t;

    emergency_exit_state_t exit_state = EXIT_WAIT_SLOW_PRESS;
    
    while (1) {
        bool slow_pressed = false, medium_pressed = false, fast_pressed = false;
        speed_buttons_get_raw(&slow_pressed, &medium_pressed, &fast_pressed);

        bool all_pressed = slow_pressed && medium_pressed && fast_pressed;

        if (!emergency_stop_active) {
            if (all_pressed) {
                if (!tracking_all_pressed) {
                    tracking_all_pressed = true;
                    all_pressed_start = xTaskGetTickCount();
                } else if ((xTaskGetTickCount() - all_pressed_start) >= pdMS_TO_TICKS(2000)) {
                    enter_emergency_stop();
                    blink_last_toggle = xTaskGetTickCount();
                    blink_state = false;
                    exit_state = EXIT_WAIT_SLOW_PRESS;
                }
            } else {
                tracking_all_pressed = false;
            }

            if (!emergency_stop_active) {
                speed_level_t new_speed = speed_buttons_get_level();
                
                if (new_speed != last_speed_level) {
                    if (new_speed == SPEED_LEVEL_OFF || last_speed_level == SPEED_LEVEL_OFF) {
                        ESP_LOGI(TAG, "Speed: %s", speed_level_to_string(new_speed));
                    }
                    
                    commanded_speed = new_speed;
                    apply_motor_current(get_current_for_speed_level(new_speed));
                    speed_buttons_set_leds(commanded_speed);
                    last_speed_level = new_speed;
                }
            }
        } else {
            TickType_t now = xTaskGetTickCount();
            if ((now - blink_last_toggle) >= pdMS_TO_TICKS(500)) {
                blink_last_toggle = now;
                blink_state = !blink_state;
                speed_buttons_set_all_leds(blink_state);
            }

            if (commanded_current != 0.0f) {
                apply_motor_current(0.0f);
            }

            switch (exit_state) {
                case EXIT_WAIT_SLOW_PRESS:
                    if (!prev_slow && slow_pressed) {
                        exit_state = EXIT_WAIT_SLOW_RELEASE;
                    }
                    break;
                case EXIT_WAIT_SLOW_RELEASE:
                    if (prev_slow && !slow_pressed) {
                        exit_state = EXIT_WAIT_MEDIUM_PRESS;
                    }
                    break;
                case EXIT_WAIT_MEDIUM_PRESS:
                    if (!prev_medium && medium_pressed) {
                        exit_state = EXIT_WAIT_MEDIUM_RELEASE;
                    }
                    break;
                case EXIT_WAIT_MEDIUM_RELEASE:
                    if (prev_medium && !medium_pressed) {
                        exit_state = EXIT_WAIT_FAST_PRESS;
                    }
                    break;
                case EXIT_WAIT_FAST_PRESS:
                    if (!prev_fast && fast_pressed) {
                        exit_state = EXIT_WAIT_FAST_RELEASE;
                    }
                    break;
                case EXIT_WAIT_FAST_RELEASE:
                    if (prev_fast && !fast_pressed) {
                        exit_state = EXIT_WAIT_SLOW_PRESS;
                        blink_state = false;
                        speed_buttons_set_all_leds(false);
                        exit_emergency_stop(&last_speed_level);
                    }
                    break;
                default:
                    exit_state = EXIT_WAIT_SLOW_PRESS;
                    break;
            }
        }

        prev_slow = slow_pressed;
        prev_medium = medium_pressed;
        prev_fast = fast_pressed;
        
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void boot_button_task(void *arg) {
    (void)arg;
    
    while (1) {
        if (BOOT_KEY_State == SINGLE_CLICK) {
            BOOT_KEY_State = NONE_PRESS;
            ESP_LOGI(TAG, "Boot button (debug)");
        }
        if (BOOT_KEY_State == LONG_PRESS_START) {
            BOOT_KEY_State = NONE_PRESS;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// =============================================================================
// Main Entry Point
// =============================================================================

void app_main(void)
{
    ESP_LOGI(TAG, "=== Death Stick Controller ===");
    ESP_LOGI(TAG, "SLOW=%.1fA, MEDIUM=%.1fA, FAST=%.1fA",
             CURRENT_SLOW, CURRENT_MEDIUM, CURRENT_FAST);

    LCD_Init();
    LVGL_Init();
    button_Init();
    speed_buttons_init();
    
    esp_err_t ret = vesc_uart_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "VESC UART init failed!");
    }

    ui_create();

    xTaskCreatePinnedToCore(vesc_task, "vesc_task", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(control_task, "control_task", 2048, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(boot_button_task, "boot_btn_task", 2048, NULL, 3, NULL, 0);

    ESP_LOGI(TAG, "Ready - HOLD buttons for speed control");

    while (1) {
        ui_update();
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
