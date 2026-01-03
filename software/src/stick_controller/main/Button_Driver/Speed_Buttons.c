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

// GPIO initialization for speed indicator LEDs
static void speed_leds_gpio_init(void) {
    gpio_reset_pin(SPEED_LED_SLOW_PIN);
    // Push-pull output. Also enable pulldown to keep the NPN base off during reset/boot.
    gpio_set_pull_mode(SPEED_LED_SLOW_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_direction(SPEED_LED_SLOW_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(SPEED_LED_SLOW_PIN, SPEED_LED_OFF_LEVEL);

    gpio_reset_pin(SPEED_LED_MEDIUM_PIN);
    gpio_set_pull_mode(SPEED_LED_MEDIUM_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_direction(SPEED_LED_MEDIUM_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(SPEED_LED_MEDIUM_PIN, SPEED_LED_OFF_LEVEL);

    gpio_reset_pin(SPEED_LED_FAST_PIN);
    gpio_set_pull_mode(SPEED_LED_FAST_PIN, GPIO_PULLDOWN_ONLY);
    gpio_set_direction(SPEED_LED_FAST_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(SPEED_LED_FAST_PIN, SPEED_LED_OFF_LEVEL);

    ESP_LOGI(TAG, "Speed LEDs initialized: SLOW=GP%d, MEDIUM=GP%d, FAST=GP%d",
             SPEED_LED_SLOW_PIN, SPEED_LED_MEDIUM_PIN, SPEED_LED_FAST_PIN);
}

void speed_buttons_init(void) {
    speed_buttons_gpio_init();
    speed_leds_gpio_init();
    speed_buttons_set_leds(SPEED_LEVEL_OFF);
    ESP_LOGI(TAG, "Speed buttons initialized (momentary mode)");
}

void speed_buttons_get_raw(bool *slow_pressed, bool *medium_pressed, bool *fast_pressed) {
    if (slow_pressed) {
        *slow_pressed = (gpio_get_level(SPEED_BTN_SLOW_PIN) == 0);
    }
    if (medium_pressed) {
        *medium_pressed = (gpio_get_level(SPEED_BTN_MEDIUM_PIN) == 0);
    }
    if (fast_pressed) {
        *fast_pressed = (gpio_get_level(SPEED_BTN_FAST_PIN) == 0);
    }
}

void speed_buttons_set_leds(speed_level_t level) {
    // Active low: drive the matching LED ON level, others OFF level
    gpio_set_level(SPEED_LED_SLOW_PIN,   (level == SPEED_LEVEL_SLOW)   ? SPEED_LED_ON_LEVEL : SPEED_LED_OFF_LEVEL);
    gpio_set_level(SPEED_LED_MEDIUM_PIN, (level == SPEED_LEVEL_MEDIUM) ? SPEED_LED_ON_LEVEL : SPEED_LED_OFF_LEVEL);
    gpio_set_level(SPEED_LED_FAST_PIN,   (level == SPEED_LEVEL_FAST)   ? SPEED_LED_ON_LEVEL : SPEED_LED_OFF_LEVEL);
}

void speed_buttons_set_all_leds(bool on) {
    int level = on ? SPEED_LED_ON_LEVEL : SPEED_LED_OFF_LEVEL;
    gpio_set_level(SPEED_LED_SLOW_PIN, level);
    gpio_set_level(SPEED_LED_MEDIUM_PIN, level);
    gpio_set_level(SPEED_LED_FAST_PIN, level);
}

speed_level_t speed_buttons_get_level(void) {
    // Read raw GPIO states (active LOW - pressed = 0)
    bool slow_pressed = false;
    bool medium_pressed = false;
    bool fast_pressed = false;
    speed_buttons_get_raw(&slow_pressed, &medium_pressed, &fast_pressed);

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
