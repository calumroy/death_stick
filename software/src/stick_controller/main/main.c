#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"

#include "LCD_Driver/ST7789.h"
#include "LVGL_Driver/LVGL_Driver.h"
#include "Button_Driver/Button_Driver.h"
#include <stdint.h>

// Images
extern lv_img_dsc_t stick_img1;
extern lv_img_dsc_t stick_img2;
void stick_images_init(void);
extern const lv_img_dsc_t dark_retro_sea_small;

static lv_img_dsc_t dark_retro_sea_small_rot;
static uint16_t dark_retro_sea_small_rot_map[EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES];

static void rotate_bg_90_cw(void) {
    const lv_img_dsc_t *src = &dark_retro_sea_small;
    const uint16_t *src16 = (const uint16_t *)src->data;
    uint16_t *dst16 = dark_retro_sea_small_rot_map;
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
    dark_retro_sea_small_rot.header.always_zero = 0;
    dark_retro_sea_small_rot.header.w = src_h;
    dark_retro_sea_small_rot.header.h = src_w;
    dark_retro_sea_small_rot.header.cf = LV_IMG_CF_TRUE_COLOR;
    dark_retro_sea_small_rot.data_size = (uint32_t)src_w * src_h * 2;
    dark_retro_sea_small_rot.data = (const uint8_t *)dark_retro_sea_small_rot_map;
}

static const char *TAG = "stick_controller";

static lv_obj_t *bg_img_obj = NULL;
static lv_obj_t *img_obj = NULL;
static bool show_first = true;

static void ui_create(void) {
    // Fill screen background
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

    // Background image (PNG, embedded)
    bg_img_obj = lv_img_create(lv_scr_act());
    rotate_bg_90_cw();  // The background image is rotated 90 degrees clockwise.
    lv_img_set_src(bg_img_obj, &dark_retro_sea_small_rot);
    lv_obj_center(bg_img_obj);
    // Optionally scale if needed for screen size (commented out by default)
    lv_obj_align(bg_img_obj, LV_ALIGN_TOP_LEFT, 0, 0);
    // lv_img_set_zoom(bg_img_obj, 256); // 256 = 1.0x

    img_obj = lv_img_create(lv_scr_act());
    lv_obj_center(img_obj);
    lv_img_set_src(img_obj, &stick_img1);
}

static void toggle_picture(void) {
    show_first = !show_first;
    lv_img_set_src(img_obj, show_first ? (const void *)&stick_img1
                                       : (const void *)&stick_img2);
}

static void button_task(void *arg) {
    (void)arg;
    while (1) {
        if (BOOT_KEY_State == SINGLE_CLICK) {
            BOOT_KEY_State = NONE_PRESS;
            ESP_LOGI(TAG, "Button pushed");
            toggle_picture();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    // Init peripherals
    button_Init();
    LCD_Init();
    LVGL_Init();
    stick_images_init();

    // Create UI
    ui_create();

    // Background task to check button and toggle images
    xTaskCreatePinnedToCore(button_task, "button_task", 2048, NULL, 3, NULL, 0);

    // LVGL main handler loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler();
    }
}


