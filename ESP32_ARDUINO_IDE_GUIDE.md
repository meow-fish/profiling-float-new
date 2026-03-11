# ESP32 Setup Guide for Arduino IDE

This guide walks you through setting up, building, and uploading the Profiling Float
firmware using the **Arduino IDE**. It assumes you are already comfortable with the
Arduino IDE but have not used it with an ESP32 before.

---

## Table of Contents

- [What You Need Before Starting](#what-you-need-before-starting)
- [Step 1: Add ESP32 Board Support](#step-1-add-esp32-board-support)
- [Step 2: Install the Required Library](#step-2-install-the-required-library)
- [Step 3: Configure Board Settings](#step-3-configure-board-settings)
- [Step 4: Prepare the Code for Arduino IDE](#step-4-prepare-the-code-for-arduino-ide)
  - [Option A: Copy Libraries to Arduino Libraries Folder](#option-a-copy-libraries-to-arduino-libraries-folder)
  - [Option B: Flatten Everything into One Sketch](#option-b-flatten-everything-into-one-sketch)
- [Step 5: Upload the Firmware](#step-5-upload-the-firmware)
- [Step 6: Open the Serial Monitor](#step-6-open-the-serial-monitor)
- [Step 7: Connect to the Float](#step-7-connect-to-the-float)
- [Over-The-Air (OTA) Updates](#over-the-air-ota-updates)
- [Running Test Sketches](#running-test-sketches)
- [Troubleshooting](#troubleshooting)
- [Differences from PlatformIO](#differences-from-platformio)

---

## What You Need Before Starting

- **Arduino IDE 2.x** (download from https://www.arduino.cc/en/software)
- **ESP32 development board** connected via USB
- **A USB cable that carries data** (some cheap cables are charge-only)
- All hardware wired up as described in the [main README](README.md#pin-connections)
  or the [Testing Guide](TESTING_GUIDE.md#part-3-wiring-step-by-step)

---

## Step 1: Add ESP32 Board Support

The Arduino IDE does not include ESP32 support by default. You need to add it.

1. Open Arduino IDE.
2. Go to **File > Preferences** (on macOS: **Arduino IDE > Preferences**).
3. Find the field labelled **"Additional boards manager URLs"**.
4. Paste this URL into the field:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
   If there are already other URLs in the field, click the icon to the right of
   the field and add the new URL on a separate line.
5. Click **OK**.
6. Go to **Tools > Board > Boards Manager**.
7. In the search box, type **esp32**.
8. Find **"esp32 by Espressif Systems"** and click **Install**. This downloads
   about 300 MB of tools and board definitions. Wait for it to finish.

After installation, you should see ESP32 boards listed under **Tools > Board**.

---

## Step 2: Install the Required Library

The firmware uses the BlueRobotics MS5837 library to communicate with the
pressure/temperature sensor.

1. Go to **Sketch > Include Library > Manage Libraries** (or click the library
   icon in the left sidebar on Arduino IDE 2.x).
2. In the search box, type **MS5837**.
3. Find **"BlueRobotics MS5837 Library"** by BlueRobotics.
4. Click **Install**.

That is the only external library needed. Everything else (WiFi, WebServer,
Wire, FreeRTOS) is already included with the ESP32 board package.

---

## Step 3: Configure Board Settings

Go to **Tools** and set the following:

| Setting | Value |
|---|---|
| **Board** | ESP32 Dev Module |
| **Upload Speed** | 921600 (use 115200 if uploads fail) |
| **CPU Frequency** | 240MHz (WiFi/BT) |
| **Flash Frequency** | 80MHz |
| **Flash Mode** | QIO |
| **Flash Size** | 4MB (32Mb) |
| **Partition Scheme** | Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS) |
| **Port** | Select the USB serial port your ESP32 is connected to |

The **Partition Scheme** setting is important. The firmware uses OTA updates,
which require a partition layout that reserves space for two firmware images.
"Minimal SPIFFS" is the same partition scheme used by the PlatformIO build
(`min_spiffs.csv`).

### Finding the correct Port

- **Windows**: Look for a COM port (e.g., `COM3`). If you do not see one,
  install the CP2102 or CH340 USB driver for your ESP32 board.
- **macOS**: Look for `/dev/cu.usbserial-XXXX` or `/dev/cu.SLAB_USBtoUART`.
- **Linux**: Look for `/dev/ttyUSB0` or `/dev/ttyACM0`.

If no port appears, try a different USB cable — yours may be charge-only.

---

## Step 4: Prepare the Code for Arduino IDE

This project uses a PlatformIO-style layout with separate library folders under
`lib/`. The Arduino IDE does not automatically recognise this structure, so you
need to do one of the following.

### Option A: Copy Libraries to Arduino Libraries Folder (Recommended)

This approach keeps the source files untouched and is easiest to maintain.

1. Find your Arduino libraries folder:
   - **Windows**: `C:\Users\<YourName>\Documents\Arduino\libraries\`
   - **macOS**: `~/Documents/Arduino/libraries/`
   - **Linux**: `~/Arduino/libraries/`

2. Copy each library folder from this project into that location:

   ```
   Copy:  lib/Stepper/       →  Arduino/libraries/FloatStepper/
   Copy:  lib/Sensor/        →  Arduino/libraries/PressureSensor/
   Copy:  lib/Network/       →  Arduino/libraries/FloatWebServer/
   Copy:  lib/DepthControl/  →  Arduino/libraries/DepthControl/
   ```

   Each copied folder should contain the `.h` and `.cpp` files directly
   (not inside a subfolder).

3. Copy the config header so the libraries can find it:

   ```
   Copy:  include/config.h   →  Arduino/libraries/FloatConfig/config.h
   ```

   Create the `FloatConfig` folder if it does not exist.

4. Now open the main sketch. In the Arduino IDE, go to **File > Open** and
   navigate to:

   ```
   src/main.cpp
   ```

   Arduino IDE requires `.ino` files, so **rename** (or copy) this file:

   ```
   Copy:  src/main.cpp  →  ProfilingFloat/ProfilingFloat.ino
   ```

   The `.ino` file must be inside a folder with the same name. Create a folder
   called `ProfilingFloat` and put `ProfilingFloat.ino` inside it.

5. Open `ProfilingFloat/ProfilingFloat.ino` in the Arduino IDE. It should
   compile without errors.

### Option B: Flatten Everything into One Sketch

If you prefer a single file, you can combine all the source code into one
`.ino` sketch. This is harder to maintain but avoids touching the libraries
folder.

1. Create a new sketch in Arduino IDE (**File > New Sketch**).
2. Save it as `ProfilingFloat`.
3. Delete the default contents and paste in the following order:
   - The contents of `include/config.h` (remove the `#ifndef`/`#define`/`#endif` guards)
   - The contents of `lib/Stepper/FloatStepper.h` (remove include guards)
   - The contents of `lib/Stepper/FloatStepper.cpp` (remove the `#include "FloatStepper.h"` line)
   - The contents of `lib/Sensor/PressureSensor.h` (remove include guards)
   - The contents of `lib/Sensor/PressureSensor.cpp` (remove the `#include "PressureSensor.h"` line)
   - The contents of `lib/Network/FloatWebServer.h` (remove include guards)
   - The contents of `lib/Network/FloatWebServer.cpp` (remove the `#include "FloatWebServer.h"` line)
   - The contents of `lib/DepthControl/DepthControl.h` (remove include guards)
   - The contents of `lib/DepthControl/DepthControl.cpp` (remove the `#include "DepthControl.h"` line)
   - The contents of `src/main.cpp` (remove the individual `#include` lines for
     the above headers since they are now inline)
4. Make sure `#include <Arduino.h>` appears only once, at the very top.
5. Keep all the standard library includes: `<Wire.h>`, `<WiFi.h>`,
   `<WebServer.h>`, `<ArduinoOTA.h>`, `<Update.h>`, `<MS5837.h>`, and
   the FreeRTOS headers.

This is error-prone. Option A is recommended.

---

## Step 5: Upload the Firmware

1. Connect the ESP32 to your computer via USB.
2. Make sure the correct **Board** and **Port** are selected under **Tools**.
3. Click the **Upload** button (right arrow icon) or press **Ctrl+U**
   (Cmd+U on macOS).
4. Wait for the compilation and upload to finish. You should see
   `"Done uploading"` at the bottom.

### If the upload fails

- **"Failed to connect" or "A fatal error occurred"**: Hold down the **BOOT**
  button on the ESP32 board while the IDE shows `"Connecting..."`, then release
  it once the upload starts.
- **Timeout errors**: Lower the upload speed to 115200 under
  **Tools > Upload Speed**.
- **Port not found**: Check the USB cable, try a different port, or install
  the USB-to-serial driver for your board (CP2102 or CH340).

---

## Step 6: Open the Serial Monitor

1. Go to **Tools > Serial Monitor** (or click the magnifying glass icon in the
   top-right corner).
2. Set the baud rate to **115200** (dropdown at the bottom-right of the serial
   monitor).
3. You should see:
   ```
   Profiling Float — initialising...
   MS5837 sensor OK
   Stepper motor OK
   All tasks started. System ready.
   ```

If you see `"ERROR: MS5837 sensor not detected. Halting."`, the pressure sensor
is not wired correctly or not connected. See the
[Troubleshooting](#troubleshooting) section.

---

## Step 7: Connect to the Float

Once the firmware is running:

1. On your phone or laptop, open WiFi settings.
2. Connect to the network:
   - **SSID**: `SSCFloat`
   - **Password**: `DT1234dt`
3. Open a browser and go to `http://192.168.4.1/control`.
4. You should see the control panel with buttons for starting dives.

See the [main README](README.md#using-the-float) for full details on the web
interface and available endpoints.

---

## Over-The-Air (OTA) Updates

Once the firmware is running and you are connected to the float's WiFi, you can
upload new firmware without a USB cable.

1. Connect your computer to the `SSCFloat` WiFi network.
2. In the Arduino IDE, go to **Tools > Port**.
3. You should see a network port appear, something like:
   ```
   192.168.4.1 (ESP32 OTA)
   ```
4. Select that network port.
5. Click **Upload** as normal. The firmware will be sent over WiFi.

If the network port does not appear:
- Make sure you are connected to the float's WiFi.
- Restart the Arduino IDE (it sometimes needs a restart to discover network ports).
- Make sure the currently-running firmware includes the OTA code — if you
  previously uploaded firmware without OTA, you will need to use USB to recover.

---

## Running Test Sketches

The project includes test sketches in the `test/` folder for verifying individual
components. To use these with the Arduino IDE:

### Pressure sensor test (`test/pressure_only/main.cpp`)

1. Copy `test/pressure_only/main.cpp` to a new sketch folder:
   ```
   PressureTest/PressureTest.ino
   ```
2. This test requires the MS5837 library (already installed in Step 2) and
   the Sensor library (already copied in Step 4A).
3. Upload and open the serial monitor.

### Manual stepper test (`test/manual_stepper/main.cpp`)

1. Copy `test/manual_stepper/main.cpp` to a new sketch folder:
   ```
   ManualStepperTest/ManualStepperTest.ino
   ```
2. This test needs the Stepper library copied in Step 4A.
3. Upload and use the serial monitor to send commands.

### Button test (`test/button_test/main.cpp`)

1. Copy `test/button_test/main.cpp` to a new sketch folder:
   ```
   ButtonTest/ButtonTest.ino
   ```
2. Upload and check the serial monitor for button state output.

For each test sketch, check the `#include` lines at the top of the file. If it
includes `"config.h"` or any of the project libraries, make sure you have
completed Step 4A (copied the libraries to your Arduino libraries folder).

---

## Troubleshooting

### "MS5837.h: No such file or directory"

The BlueRobotics library is not installed. Go to **Sketch > Include Library >
Manage Libraries**, search for **MS5837**, and install it.

### "config.h: No such file or directory"

You need to copy `include/config.h` into your Arduino libraries folder as
described in [Step 4A](#option-a-copy-libraries-to-arduino-libraries-folder).

### "FloatStepper.h: No such file or directory" (or any project header)

The project libraries are not in your Arduino libraries folder. Follow
[Step 4A](#option-a-copy-libraries-to-arduino-libraries-folder) to copy them.

### Compilation errors about redefined symbols

If you used Option B (flattening), you probably left in duplicate `#include`
lines or include guards. Make sure each header's content appears only once.

### Upload fails with "Failed to connect to ESP32"

1. Hold the **BOOT** button on the ESP32 while the upload starts.
2. Try a different USB cable (charge-only cables will not work).
3. Try lowering the upload speed to 115200 in **Tools > Upload Speed**.
4. On Windows, install the CP2102 or CH340 USB driver.

### "ERROR: MS5837 sensor not detected. Halting."

The pressure sensor is not responding. Check:
- SDA is connected to GPIO21, SCL to GPIO22.
- The sensor is powered from **3.3V**, not 5V.
- The I2C wires are not loose.

### WiFi network does not appear after upload

This usually means the firmware halted during setup (most likely the sensor
failed to initialise). Open the serial monitor to see what error message is
printed.

### Serial monitor shows garbage characters

The baud rate is wrong. Set it to **115200** in the serial monitor dropdown.

---

## Differences from PlatformIO

If you are reading the main README or other documentation and see PlatformIO
commands, here is what they translate to in the Arduino IDE:

| PlatformIO command | Arduino IDE equivalent |
|---|---|
| `pio run` | Click **Verify** (checkmark icon) or Ctrl+R |
| `pio run -t upload` | Click **Upload** (arrow icon) or Ctrl+U |
| `pio device monitor` | **Tools > Serial Monitor** |
| `pio run -e test_pressure -t upload` | Open the test sketch as a separate `.ino` file and upload it |
| `lib_deps` in `platformio.ini` | Install libraries via **Sketch > Include Library > Manage Libraries** |
| `board_build.partitions = min_spiffs.csv` | Set **Tools > Partition Scheme** to "Minimal SPIFFS" |
| `monitor_speed = 115200` | Set serial monitor baud rate to 115200 |
| `upload_speed = 921600` | Set **Tools > Upload Speed** to 921600 |

The key difference is that PlatformIO automatically finds the library files in
the `lib/` folder, while the Arduino IDE needs them in its own libraries
directory (or flattened into the sketch).
