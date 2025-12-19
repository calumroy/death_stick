/**
 * @file Speed_Buttons.h
 * @brief External speed control buttons driver
 * 
 * MOMENTARY OPERATION:
 * - GP2: SLOW button - hold for low current
 * - GP3: MEDIUM button - hold for medium current
 * - GP4: FAST button - hold for high current
 * - Release all = motor stops
 * 
 * All buttons use internal pull-up resistors and are active LOW
 * (pulled to GND when pressed)
 */

#ifndef SPEED_BUTTONS_H
#define SPEED_BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// GPIO pins for speed buttons (active LOW with internal pull-up)
#define SPEED_BTN_SLOW_PIN      2   // GP2 - SLOW speed
#define SPEED_BTN_MEDIUM_PIN    3   // GP3 - MEDIUM speed
#define SPEED_BTN_FAST_PIN      4   // GP4 - FAST speed

// GPIO pins for the button LEDs (active HIGH)
#define SPEED_LED_SLOW_PIN      7   // LED for SLOW button
#define SPEED_LED_MEDIUM_PIN    8   // LED for MEDIUM button
#define SPEED_LED_FAST_PIN      9   // LED for FAST button

// Speed level enumeration
typedef enum {
    SPEED_LEVEL_OFF = 0,
    SPEED_LEVEL_SLOW,
    SPEED_LEVEL_MEDIUM,
    SPEED_LEVEL_FAST,
} speed_level_t;

/**
 * @brief Initialize the speed control buttons
 * 
 * Sets up GP2, GP3, GP4 as inputs with internal pull-ups
 */
void speed_buttons_init(void);

/**
 * @brief Get the current speed level based on button state
 * 
 * Reads GPIO directly - returns the speed for whichever button is held.
 * Priority: FAST > MEDIUM > SLOW
 * Returns SPEED_LEVEL_OFF if no button is pressed.
 * 
 * @return Current speed_level_t value
 */
speed_level_t speed_buttons_get_level(void);

/**
 * @brief Get speed level as string
 * @param level Speed level
 * @return String description of speed level
 */
const char* speed_level_to_string(speed_level_t level);

/**
 * @brief Update LED outputs to match the requested speed level
 *
 * Lights only the LED that corresponds to the active speed. All LEDs are
 * turned off when SPEED_LEVEL_OFF is selected.
 *
 * @param level Current speed level
 */
void speed_buttons_set_leds(speed_level_t level);

#ifdef __cplusplus
}
#endif

#endif // SPEED_BUTTONS_H
