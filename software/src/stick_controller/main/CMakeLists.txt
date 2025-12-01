idf_component_register(
    SRCS
        "main.c"
        "LCD_Driver/ST7789.c"
        "LCD_Driver/Vernon_ST7789T.c"
        "LVGL_Driver/LVGL_Driver.c"
        "Button_Driver/multi_button.c"
        "Button_Driver/Button_Driver.c"
        "images/pictures.c"
        "images/screenshot1.c"
    INCLUDE_DIRS
        "."
        "./LCD_Driver"
        "./LVGL_Driver"
        "./Button_Driver"
        "./images"
)


