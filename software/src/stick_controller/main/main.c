#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"

#include "LCD_Driver/ST7789.h"
#include "LVGL_Driver/LVGL_Driver.h"
#include "Button_Driver/Button_Driver.h"

// Images
extern lv_img_dsc_t stick_img1;
extern lv_img_dsc_t stick_img2;
void stick_images_init(void);
extern const lv_img_dsc_t screenshot1;

static const char *TAG = "stick_controller";

static lv_obj_t *bg_img_obj = NULL;
static lv_obj_t *img_obj = NULL;
static bool show_first = true;

static void ui_create(void) {
    // Fill screen background
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);

    // Background image (PNG, embedded)
    bg_img_obj = lv_img_create(lv_scr_act());
    lv_img_set_src(bg_img_obj, &screenshot1);
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


