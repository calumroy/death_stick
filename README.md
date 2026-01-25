# Death Stick

A fun death stick.

## Button Controls

Three speed buttons (active LOW, active while held):
- **SLOW** (GP2): 20A - Low power
- **MEDIUM** (GP3): 40A - Normal cruising  
- **FAST** (GP4): 70A - Full power (ramps up over 1 second)

Release all buttons to stop.

**Emergency Stop:** Hold all 3 buttons for 2 seconds. Current cuts immediately when all buttons are pressed.

**Exit Emergency Stop:** Press and hold each button in sequence (SLOW → MEDIUM → FAST), holding each for 500ms. If multiple buttons are pressed simultaneously, the sequence resets.

## Software 

The controller is an [ESP32-S3-LCD-1.47B](https://www.waveshare.com/wiki/ESP32-S3-LCD-1.47B) board.

Building and flashing the software is done using the containerized ESP-IDF build (Podman).

### Containerized ESP-IDF build (Podman)

Build and flash ESP-IDF projects in a container with access to `/dev/ttyACM0`.

Prereqs: Podman installed; user can access serial devices (often group `dialout`).

1) Initialize ESP-IDF submodule

```bash
git submodule update --init --recursive software/esp_idf
```

2) Build image

```bash
bash scripts/podman-build.sh
```

3) Run container (maps repo, caches tools, exposes `/dev/ttyACM0`)

Check that once the device is plugged in, the device is recognized by the host and is on the `/dev/ttyACM0` path.
If it comes up as something else, you can try to run the container with a different device:

```bash
bash scripts/podman-run.sh
```

Use another device:

```bash
TTY_DEVICE=/dev/ttyUSB1 bash scripts/podman-run.sh
```

Inside the container (ESP-IDF is already exported by entrypoint):

```bash
idf.py --version
idf.py set-target esp32s3
cd ./software/src/stick_controller
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```c
Quit the serial monitor by typing Ctrl-]
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H

Reattach to a running container:

```bash
bash scripts/podman-exec.sh bash
```

Troubleshooting:
- If serial permissions fail, ensure the host user is in `dialout` and the device exists. You can try `TTY_DEVICE=/dev/ttyACM0`.
- Tools cache persists in `.espressif`. Delete it to force re-install.

- If you power cycle the ESP or disconnct the USB and reconnct the podman container will need to be restarted otherwise it won't get permission to use the device serial port anymore.


## Flipsky Uart settings

App Settings → UART
    Baud rate: 115200
    UART Mode: UART
App to Use: UART

We use Uart0 to communicate from the ESP32 to the Flipsky ESC.