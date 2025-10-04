#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "esp_lcd_touch_axs5106.h"

#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "bsp_touch";
#include "bsp_touch.h"

void bsp_touch_init(esp_lcd_touch_handle_t *touch_handle, i2c_master_bus_handle_t bus_handle, uint16_t xmax, uint16_t ymax, uint16_t rotation)
{
    static i2c_master_dev_handle_t dev_handle;
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ESP_LCD_TOUCH_IO_I2C_AXS5106_ADDRESS,
        .scl_speed_hz = 50000,
    };

    ESP_LOGI(TAG, "Probing touch I2C at 0x%02x", dev_cfg.device_address);
    bool found = false;
    if (i2c_master_probe(bus_handle, dev_cfg.device_address, 100) == ESP_OK) {
        ESP_LOGI(TAG, "Found touch at 0x%02x", dev_cfg.device_address);
        found = true;
    } else {
        const uint8_t candidates[] = { 0x38, 0x14, 0x63, 0x31 };
        for (size_t i = 0; i < sizeof(candidates); ++i) {
            ESP_LOGW(TAG, "Probe 0x%02x failed, trying 0x%02x", dev_cfg.device_address, candidates[i]);
            if (i2c_master_probe(bus_handle, candidates[i], 100) == ESP_OK) {
                dev_cfg.device_address = candidates[i];
                ESP_LOGW(TAG, "Using fallback touch address 0x%02x", candidates[i]);
                found = true;
                break;
            }
        }
    }
    if (!found) {
        ESP_LOGE(TAG, "No touch device responded on I2C bus; skipping touch init");
        return;
    }
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    esp_lcd_touch_config_t tp_cfg = {};
    tp_cfg.x_max = xmax < ymax ? xmax : ymax;
    tp_cfg.y_max = xmax < ymax ? ymax : xmax;;
    tp_cfg.rst_gpio_num = EXAMPLE_PIN_TP_RST;
    tp_cfg.int_gpio_num = EXAMPLE_PIN_TP_INT;

    if (90 == rotation)
    {
        tp_cfg.flags.swap_xy = 1;
        tp_cfg.flags.mirror_x = 0;
        tp_cfg.flags.mirror_y = 0;
    }
    else if (180 == rotation)
    {
        tp_cfg.flags.swap_xy = 0;
        tp_cfg.flags.mirror_x = 0;
        tp_cfg.flags.mirror_y = 1;
    }
    else if (270 == rotation)
    {
        tp_cfg.flags.swap_xy = 1;
        tp_cfg.flags.mirror_x = 1;
        tp_cfg.flags.mirror_y = 1;
    }
    else
    {
        tp_cfg.flags.swap_xy = 0;
        tp_cfg.flags.mirror_x = 1;
        tp_cfg.flags.mirror_y = 0;
    }

    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_axs5106(dev_handle, &tp_cfg, touch_handle));
}
