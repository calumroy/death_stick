#include "lvgl.h"

// Two tiny 32x32 RGB565 images (solid colors with simple pattern) to keep binary size small.
// Image 1: red checkerboard
static const uint16_t img1_pixels[32 * 32] = {
    #define C1(a,b) (((((a)+(b))&1)?0xF800:0x0000))
    #define ROW(y) C1(0,y),C1(1,y),C1(2,y),C1(3,y),C1(4,y),C1(5,y),C1(6,y),C1(7,y),C1(8,y),C1(9,y),C1(10,y),C1(11,y),C1(12,y),C1(13,y),C1(14,y),C1(15,y),\
                    C1(16,y),C1(17,y),C1(18,y),C1(19,y),C1(20,y),C1(21,y),C1(22,y),C1(23,y),C1(24,y),C1(25,y),C1(26,y),C1(27,y),C1(28,y),C1(29,y),C1(30,y),C1(31,y)
    ROW(0),ROW(1),ROW(2),ROW(3),ROW(4),ROW(5),ROW(6),ROW(7),
    ROW(8),ROW(9),ROW(10),ROW(11),ROW(12),ROW(13),ROW(14),ROW(15),
    ROW(16),ROW(17),ROW(18),ROW(19),ROW(20),ROW(21),ROW(22),ROW(23),
    ROW(24),ROW(25),ROW(26),ROW(27),ROW(28),ROW(29),ROW(30),ROW(31)
    #undef ROW
    #undef C1
};

// Image 2: green gradient
static uint16_t img2_pixels[32 * 32];

static void init_img2(void) {
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            uint8_t g = (uint8_t)((x + y) * 4); // 0..248 approx
            // RGB565: R:5,G:6,B:5
            uint16_t pixel = ((g >> 3) << 5); // green field
            img2_pixels[y * 32 + x] = pixel;
        }
    }
}

// Public LVGL image descriptors
lv_img_dsc_t stick_img1 = {
  .header.always_zero = 0,
  .header.w = 32,
  .header.h = 32,
  .data_size = sizeof(img1_pixels),
  .header.cf = LV_IMG_CF_TRUE_COLOR, // RGB565 in LVGL
  .data = (const uint8_t *)img1_pixels,
};

lv_img_dsc_t stick_img2 = {
  .header.always_zero = 0,
  .header.w = 32,
  .header.h = 32,
  .data_size = sizeof(img2_pixels),
  .header.cf = LV_IMG_CF_TRUE_COLOR,
  .data = (const uint8_t *)img2_pixels,
};

// Simple one-time initializer to be called before first use
void stick_images_init(void) {
    static bool inited = false;
    if (inited) return;
    init_img2();
    inited = true;
}


