/**
 * @file Speed_Buttons.c
 * @brief External speed control buttons driver implementation
 * 
 * MOMENTARY OPERATION:
 * - Hold button = motor runs at that speed
 * - Release = motor stops
 * - Only one button active at a time (highest priority: FAST > MEDIUM > SLOW)
 */

#include "Speed_Buttons.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "speed_buttons";

// GPIO initialization for speed buttons
static void speed_buttons_gpio_init(void) {
    // Configure GP2 (SLOW)
    gpio_reset_pin(SPEED_BTN_SLOW_PIN);
    gpio_set_direction(SPEED_BTN_SLOW_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SPEED_BTN_SLOW_PIN, GPIO_PULLUP_ONLY);

    // Configure GP3 (MEDIUM)
    gpio_reset_pin(SPEED_BTN_MEDIUM_PIN);
    gpio_set_direction(SPEED_BTN_MEDIUM_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SPEED_BTN_MEDIUM_PIN, GPIO_PULLUP_ONLY);

    // Configure GP4 (FAST)
    gpio_reset_pin(SPEED_BTN_FAST_PIN);
    gpio_set_direction(SPEED_BTN_FAST_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SPEED_BTN_FAST_PIN, GPIO_PULLUP_ONLY);

    ESP_LOGI(TAG, "Speed buttons GPIO initialized: SLOW=GP%d, MEDIUM=GP%d, FAST=GP%d",
             SPEED_BTN_SLOW_PIN, SPEED_BTN_MEDIUM_PIN, SPEED_BTN_FAST_PIN);
}

void speed_buttons_init(void) {
    speed_buttons_gpio_init();
    ESP_LOGI(TAG, "Speed buttons initialized (momentary mode)");
}

speed_level_t speed_buttons_get_level(void) {
    // Read raw GPIO states (active LOW - pressed = 0)
    bool slow_pressed   = (gpio_get_level(SPEED_BTN_SLOW_PIN) == 0);
    bool medium_pressed = (gpio_get_level(SPEED_BTN_MEDIUM_PIN) == 0);
    bool fast_pressed   = (gpio_get_level(SPEED_BTN_FAST_PIN) == 0);

    // Priority: FAST > MEDIUM > SLOW
    // If multiple buttons pressed, highest speed wins
    if (fast_pressed) {
        return SPEED_LEVEL_FAST;
    } else if (medium_pressed) {
        return SPEED_LEVEL_MEDIUM;
    } else if (slow_pressed) {
        return SPEED_LEVEL_SLOW;
    }
    
    // No button pressed
    return SPEED_LEVEL_OFF;
}

const char* speed_level_to_string(speed_level_t level) {
    switch (level) {
        case SPEED_LEVEL_OFF:    return "OFF";
        case SPEED_LEVEL_SLOW:   return "SLOW";
        case SPEED_LEVEL_MEDIUM: return "MEDIUM";
        case SPEED_LEVEL_FAST:   return "FAST";
        default:                 return "UNKNOWN";
    }
}
