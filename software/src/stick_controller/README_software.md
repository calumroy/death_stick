## Death Stick Controller Software

ESP32-S3-LCD-1.47B based motor controller with VESC UART communication.

### Features

- **VESC UART Communication**: Communicates with Flipsky VESC motor controller at 115200 baud
- **Three Speed Levels**: SLOW, MEDIUM, FAST buttons to control motor current
- **LCD Telemetry Display**: Shows voltage, current, RPM, temperature, and fault status
- **Emergency Stop**: Long-press any button to immediately stop the motor

### Hardware Connections

#### VESC UART (UART0)
| ESP32 Pin | VESC Wire | Function |
|-----------|-----------|----------|
| TX (GPIO43) | Yellow | VESC RX |
| RX (GPIO44) | White | VESC TX |
| 5V | Red | Power |
| GND | Black | Ground |

#### Speed Control Buttons (Active LOW with internal pull-ups)
| ESP32 Pin | Button | Function |
|-----------|--------|----------|
| GP2 | SW1 | SLOW speed |
| GP3 | SW2 | MEDIUM speed |
| GP4 | SW3 | FAST speed |
| GND | All switches | Common ground |

### Current Settings

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

Inside the ESP-IDF container:

```bash
cd /workspace/software/src/stick_controller
idf.py fullclean
idf.py build
idf.py -p ${TTY_DEVICE:-/dev/ttyACM0} flash monitor
```

### Usage

1. **Power on** - The LCD will show "VESC: Connecting..." until communication is established
2. **Select speed** - Press GP2 (SLOW), GP3 (MEDIUM), or GP4 (FAST) to set motor current
3. **Toggle off** - Press the same button again to stop the motor
4. **Emergency stop** - Long-press any button to immediately stop

### LCD Display Information

- **SPEED**: Current speed setting (OFF/SLOW/MEDIUM/FAST)
- **VOLT**: Battery voltage from VESC
- **CMD**: Commanded current (what you requested)
- **MOTOR**: Actual motor current (what's flowing)
- **RPM**: Motor RPM
- **TEMP**: MOSFET temperature
- **VESC**: Connection status or fault code

### File Structure

```
main/
├── main.c                    # Main application
├── Button_Driver/
│   ├── Button_Driver.c/h     # Internal BOOT button
│   ├── Speed_Buttons.c/h     # External speed buttons (GP2,GP3,GP4)
│   └── multi_button.c/h      # Button debounce library
├── VESC_Driver/
│   └── vesc_uart.c/h         # VESC UART communication driver
├── LCD_Driver/
│   └── ST7789.c/h            # LCD driver
├── LVGL_Driver/
│   └── LVGL_Driver.c/h       # LVGL graphics driver
└── images/
    └── *.c                   # Image assets
```

### VESC Configuration

The VESC must be configured for UART communication:

1. Open VESC Tool
2. Go to **App Settings → UART**
3. Set **Baud rate**: 115200
4. Set **UART Mode**: UART
5. Go to **App Settings → General**
6. Set **App to Use**: UART

### Troubleshooting

- **"VESC: No Connection"** - Check UART wiring (TX→RX crossover), verify VESC is powered and configured for UART mode
- **Motor doesn't respond** - Verify current limits in VESC Tool, check fault codes on display
- **Buttons not working** - Verify GP2/GP3/GP4 are connected to buttons that pull to GND when pressed
- **Display garbled** - May need to clean build: `idf.py fullclean && idf.py build`

### Safety Notes

⚠️ **CAUTION**: This controls a potentially dangerous motor. Always:
- Test with motor disconnected first
- Start with low current values
- Have emergency stop ready (long-press any button)
- Never leave unattended while powered
