## Background image: how it works and how to change it

This project shows a background image behind the two toggle images using an LVGL image compiled into the firmware as a C array.

What’s in the code
- `main/images/screenshot1.c` defines a `const lv_img_dsc_t screenshot1` with RGB565 pixels (stored in flash).
- `main/main.c` creates an LVGL image object and sets that descriptor as the source:
  - `lv_img_set_src(bg_img_obj, &screenshot1);`
  - It’s aligned top-left, under the two foreground images.
- Build is wired in `main/CMakeLists.txt` by listing `images/screenshot1.c` among the component sources.

Replace the background (recommended)
1) Prepare your image
- Target screen size: 172x320 (portrait) for the ST7789 panel.
- Use the LVGL Image Converter: [LVGL Image Converter](https://lvgl.io/tools/imageconverter)
- Settings:
  - Color format: RGB565
  - Output: C array
  - Dithering/Compression: Off
  - Variable name: `screenshot1` (to avoid any code changes)

2) Drop into the project
- Save the generated C file as `software/src/stick_controller/main/images/screenshot1.c`, replacing the existing file.
- Nothing else needs changing if you keep the symbol name `screenshot1`.

3) Rebuild/flash inside your container
```bash
cd /workspace/software/src/stick_controller
idf.py fullclean
idf.py build
idf.py -p ${TTY_DEVICE:-/dev/ttyACM0} flash monitor
```

If you insist on a different symbol/file name
- Add your new file (e.g. `images/my_bg.c`) to `main/CMakeLists.txt` SRCS.
- In `main/main.c` change:
  - `extern const lv_img_dsc_t screenshot1;` → `extern const lv_img_dsc_t my_bg;`
  - `lv_img_set_src(bg_img_obj, &screenshot1);` → `lv_img_set_src(bg_img_obj, &my_bg);`

Troubleshooting
- White screen or “No data”:
  - Your descriptor is wrong. Ensure it’s `lv_img_dsc_t` with `header.cf = LV_IMG_CF_TRUE_COLOR` and RGB565 data.
  - Width/height must match the data, and `data_size` should be `width * height * 2` for RGB565.
- Looks cropped/offset:
  - We align the background top-left: adjust with `lv_obj_align(bg_img_obj, LV_ALIGN_TOP_LEFT, 0, 0);`
  - You can scale with `lv_img_set_zoom(bg_img_obj, 256);` (256 = 1.0x; 128 = 0.5x; 512 = 2.0x).
- Out of memory with PNG:
  - Don’t use large PNGs on this device unless you enjoy boot loops. The C-array route is lighter and faster.

Optional: PNG route (not recommended on low RAM)
- Enable `CONFIG_LV_USE_PNG=y`, embed the file via `EMBED_FILES` in `main/CMakeLists.txt`, and decode at runtime. This costs RAM and is more fragile; stick to the C array unless you know what you’re doing.


