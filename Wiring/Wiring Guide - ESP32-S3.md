# Wiring Guide - ESP32-S3

This document details the pin assignments (GPIO) for the ESP32-S3 microcontroller. 

## 🕹️ Joysticks (Analog Axes)

The controller uses 4 analog joysticks (Hall effect or traditional potentiometers). Since each has 2 axes, we need to connect 8 analog signals in total.

The analog pins defined in the code are as follows:
* `GPIO 4`
* `GPIO 5`
* `GPIO 6`
* `GPIO 7`
* `GPIO 8`
* `GPIO 9`
* `GPIO 10`
* `GPIO 11`

*(Note: Make sure to power the joysticks with the 3.3V pin of the ESP32, not 5V. The analog pins on the ESP32-S3 are only 3.3V tolerant and applying 5V will damage the board).*

## 🔘 Buttons (SpaceMouse Pro Reverse Engineering)

The firmware uses internal pull-up resistors (`INPUT_PULLUP`) for all buttons. This means **each button must be wired between the indicated GPIO pin and Ground (GND)**. No external resistors are needed.

The SpaceMouse Pro uses a 32-bit button report. After reverse-engineering the original hardware, the exact positions of each button were mapped. The following table indicates which physical GPIO pin triggers which virtual button inside the 3DxWare driver.

Unassigned bits (empty spaces in the original hardware) are defined in the code as `NC` (Not Connected).

| Driver Bit (0-31) | 3DxWare Function | ESP32-S3 GPIO | Data Byte |
| :---: | :--- | :---: | :---: |
| **0** | MENU | `1` * | Byte 1 |
| **1** | FIT | `2` * | Byte 1 |
| **2** | T (Top) | `3` | Byte 1 |
| **4** | R (Right) | `12` | Byte 1 |
| **5** | F (Front) | `13` | Byte 1 |
| **8** | ROTATE (Roll+) | `14` | Byte 2 |
| **12** | Button 1 | `15` | Byte 2 |
| **13** | Button 2 | `16` | Byte 2 |
| **14** | Button 3 | `17` | Byte 2 |
| **15** | Button 4 | `18` | Byte 2 |
| **22** | ESC | `38` | Byte 3 |
| **23** | ALT | `39` | Byte 3 |
| **24** | SHIFT | `40` | Byte 4 |
| **25** | CTRL | `41` | Byte 4 |
| **26** | LOCK (Rotation Toggle) | `42` | Byte 4 |

> **⚠️ Hardware Warning (Strapping Pins):** Pins `GPIO 1` and `GPIO 2` work perfectly as inputs with Pull-Ups during normal operation, but they are special boot pins (Strapping Pins) on the ESP32. **Avoid holding down the MENU and FIT buttons exactly when powering on / plugging in the board**, as the ESP32 might try to enter bootloader/flashing mode.