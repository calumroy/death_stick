# Wiring Table for the Stick Controller

How to wire up the ESP32-S3-LCD-1.47B to the VESC UART and the digital inputs?

VESC_UART
-Red: 5V_OUT
-Black: GND
-Yellow: TX (VESC_TX)
-White: RX (VESC_RX)
--
ESP32_S3_UART
-5V: Power_Input
-GND: Ground
-RX: UART0_RX
-TX: UART0_TX
--
Connections_UART
-Red    -> 5V
-Black  -> GND
-Yellow -> RX (ESP32_S3)
-White  -> TX (ESP32_S3)
--
ESP32_S3_DIGITAL_INPUTS
-GP2: Digital_Input_1
-GP3: Digital_Input_2
-GP4: Digital_Input_3
-3V3: Provides pull-up if needed
-GND: Common ground for all switches
--
Switch_Wiring
-SW1 -> GP2 (other side to GND)
-SW2 -> GP3 (other side to GND)
-SW3 -> GP4 (other side to GND)
--
Notes
-Configure these GPIOs as INPUT_PULLUP
-Switches pull the pin LOW when pressed
-All switches must share the ESP32 GND