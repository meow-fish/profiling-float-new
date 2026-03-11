# Getting Started with PlatformIO — A Guide for Arduino IDE Users

This guide is for people who have used the **Arduino IDE** before and want to set up
this project using **PlatformIO in VS Code**. It explains PlatformIO concepts by
relating them to things you already know from the Arduino IDE.

No prior PlatformIO experience is needed. Every step is explained.

---

## Table of Contents

- [Why PlatformIO Instead of Arduino IDE?](#why-platformio-instead-of-arduino-ide)
- [What You Need](#what-you-need)
- [Step 1: Install VS Code](#step-1-install-vs-code)
- [Step 2: Install the PlatformIO Extension](#step-2-install-the-platformio-extension)
- [Step 3: Open the Project](#step-3-open-the-project)
- [Step 4: Understanding the Project Layout](#step-4-understanding-the-project-layout)
- [Step 5: Build the Firmware](#step-5-build-the-firmware)
- [Step 6: Upload to the ESP32](#step-6-upload-to-the-esp32)
- [Step 7: Open the Serial Monitor](#step-7-open-the-serial-monitor)
- [Step 8: Connect to the Float](#step-8-connect-to-the-float)
- [Running Test Sketches](#running-test-sketches)
- [Over-The-Air (OTA) Updates](#over-the-air-ota-updates)
- [Editing the Code](#editing-the-code)
- [PlatformIO vs Arduino IDE — Quick Reference](#platformio-vs-arduino-ide--quick-reference)
- [Troubleshooting](#troubleshooting)

---

## Why PlatformIO Instead of Arduino IDE?

If you have used the Arduino IDE, you know that:
- You have to manually install board packages and libraries.
- All your code usually lives in one `.ino` file (or you have to manually manage
  multiple tabs).
- Board settings (speed, flash size, partition scheme) are selected from dropdown
  menus, and it is easy to forget one.

PlatformIO solves these problems:

| Pain point in Arduino IDE | How PlatformIO fixes it |
|---|---|
| Manually installing ESP32 board package | PlatformIO downloads it automatically when you build |
| Manually installing libraries (Library Manager) | Libraries are listed in `platformio.ini` and installed automatically |
| Choosing board settings from dropdown menus | All settings are saved in `platformio.ini` — everyone gets the same config |
| One big `.ino` file or awkward multi-tab setup | Code is split into clean folders (`src/`, `lib/`, `include/`) and it just works |
| Switching between board types requires re-selecting settings | Define multiple environments in `platformio.ini` and switch with one command |

**In short**: you clone the project, open it in VS Code, and everything is
already configured. No manual setup needed.

---

## What You Need

- A computer (Windows, macOS, or Linux)
- An ESP32 development board
- A USB cable that carries data (some cheap cables are charge-only)
- All hardware wired up as described in the [main README](README.md#pin-connections)
  or the [Testing Guide](TESTING_GUIDE.md#part-3-wiring-step-by-step)

---

## Step 1: Install VS Code

PlatformIO runs as an extension inside **Visual Studio Code** (VS Code). VS Code
is a free code editor from Microsoft — it is not the same as Visual Studio.

1. Go to https://code.visualstudio.com and download the installer for your
   operating system.
2. Run the installer and follow the prompts. The defaults are fine.
3. Open VS Code.

> **If you already have VS Code installed**, skip this step.

---

## Step 2: Install the PlatformIO Extension

This is like installing the ESP32 board package in Arduino IDE, except PlatformIO
handles all boards and libraries in one go.

1. Open VS Code.
2. Click the **Extensions** icon in the left sidebar (it looks like four small
   squares, or press **Ctrl+Shift+X**).
3. In the search box at the top, type **PlatformIO IDE**.
4. Find the extension by **PlatformIO** (it should be the first result) and
   click **Install**.
5. Wait for the installation to complete. This may take a few minutes — PlatformIO
   downloads its core tools in the background.
6. When it finishes, you will see a small **house icon** (PlatformIO Home) appear
   in the bottom toolbar. You may also be prompted to reload VS Code — click
   **Reload** if so.

That is it. You do not need to install the ESP32 board package separately — PlatformIO
will download it automatically the first time you build.

---

## Step 3: Open the Project

1. If you have not already, download or clone this repository to your computer.
   You can use the green **Code** button on the GitHub page and choose
   **Download ZIP**, then unzip it. Or if you have Git installed:
   ```bash
   git clone <this-repo-url>
   ```

2. In VS Code, go to **File > Open Folder** (on macOS: **File > Open...**).

3. Navigate to the `profiling-float-new` folder and click **Open** (or
   **Select Folder** on Windows).

4. VS Code will detect the `platformio.ini` file and recognise this as a
   PlatformIO project. You may see a notification: *"PlatformIO: Installing
   platform espressif32..."*. Let it finish — this is PlatformIO automatically
   downloading the ESP32 toolchain and libraries. This only happens once.

> **Arduino IDE equivalent**: This is like opening a `.ino` file, but instead of
> one file, you are opening the entire project folder. PlatformIO reads the
> `platformio.ini` file to know what board, libraries, and settings to use.

---

## Step 4: Understanding the Project Layout

In the Arduino IDE, your code lives in a single `.ino` file (or multiple tabs
in the same sketch). In PlatformIO, the project is organised into folders:

```
profiling-float-new/
├── platformio.ini              ← The "settings" file (like Tools menu in Arduino IDE)
├── include/
│   └── config.h                ← Pin assignments, WiFi credentials, constants
├── src/
│   └── main.cpp                ← The main sketch (like your .ino file)
├── lib/
│   ├── Stepper/                ← Motor control library
│   ├── Sensor/                 ← Pressure sensor library
│   ├── Network/                ← WiFi and web server library
│   └── DepthControl/           ← Dive logic library
└── test/
    ├── manual_stepper/         ← Test sketch for motor
    ├── pressure_only/          ← Test sketch for sensor
    └── button_test/            ← Test sketch for buttons
```

Here is how each part maps to what you know:

| PlatformIO | Arduino IDE equivalent |
|---|---|
| `platformio.ini` | **Tools** menu (board, speed, partition scheme) + **Library Manager** |
| `src/main.cpp` | Your `.ino` sketch file |
| `lib/` folders | Libraries you install via **Sketch > Include Library > Manage Libraries** |
| `include/config.h` | Constants you would put at the top of your `.ino` file |
| `test/` folders | Separate sketches you would open in a new Arduino IDE window |

### A closer look at `platformio.ini`

Open `platformio.ini` in VS Code. You will see something like this:

```ini
[env:esp32]
platform = espressif32          ; ← Like selecting "ESP32" in Boards Manager
board = esp32dev                ; ← Like selecting "ESP32 Dev Module" in Tools > Board
framework = arduino             ; ← We are using the Arduino framework (same as Arduino IDE)
monitor_speed = 115200          ; ← Serial monitor baud rate
upload_speed = 921600           ; ← Upload speed
lib_deps =
    bluerobotics/BlueRobotics MS5837 Library@^1.1.1   ; ← Like installing a library
board_build.partitions = min_spiffs.csv                ; ← Like Tools > Partition Scheme
```

Everything you would normally set through dropdown menus in the Arduino IDE is
written here as text. The advantage is that anyone who opens this project gets
the exact same settings — no need to remember which partition scheme to pick.

---

## Step 5: Build the Firmware

"Building" means compiling the code, just like clicking **Verify** (the checkmark)
in the Arduino IDE. It checks for errors but does not upload anything yet.

### Using the toolbar (GUI way)

Look at the **bottom toolbar** in VS Code (the blue or dark bar at the very
bottom of the window). You should see these PlatformIO icons:

```
 ✓ (Build)    → (Upload)    🔌 (Serial Monitor)    🏠 (PlatformIO Home)
```

Click the **checkmark** (✓) to build. You can also hover over the icons to
see their labels.

### Using the terminal (command way)

Open the VS Code terminal with **Ctrl+`** (backtick) or **Terminal > New
Terminal**, then type:

```bash
pio run
```

### What to expect

The first build takes longer because PlatformIO downloads the ESP32 toolchain
and the MS5837 library. Subsequent builds are much faster.

When the build succeeds, you will see:

```
========================= [SUCCESS] Took X.XXs =========================
```

If you see errors, check the [Troubleshooting](#troubleshooting) section.

> **Arduino IDE equivalent**: `pio run` = clicking the **Verify** button (✓).

---

## Step 6: Upload to the ESP32

### Connect the board

1. Plug the ESP32 into your computer via USB.
2. PlatformIO should automatically detect the serial port. If not, see
   [Troubleshooting](#troubleshooting).

### Upload

**GUI way**: Click the **right arrow** (→) in the bottom toolbar.

**Terminal way**:
```bash
pio run -t upload
```

You will see progress output as the firmware is compiled (if needed) and
flashed to the board. When it finishes:

```
========================= [SUCCESS] Took X.XXs =========================
```

### If the upload fails

- **"Failed to connect to ESP32"**: Hold the **BOOT** button on the ESP32
  while the upload starts, then release it once you see upload progress.
- **Timeout errors**: Try lowering the upload speed. Open `platformio.ini` and
  change `upload_speed = 921600` to `upload_speed = 115200`.
- **Port not found**: Check that your USB cable carries data. Try a different
  cable. On Windows, you may need to install the CP2102 or CH340 USB driver.

> **Arduino IDE equivalent**: `pio run -t upload` = clicking the **Upload**
> button (→) or pressing Ctrl+U.

---

## Step 7: Open the Serial Monitor

The serial monitor works just like in the Arduino IDE — it shows text that the
ESP32 prints via `Serial.println()`.

**GUI way**: Click the **plug icon** (🔌) in the bottom toolbar.

**Terminal way**:
```bash
pio device monitor
```

You should see:

```
Profiling Float — initialising...
MS5837 sensor OK
Stepper motor OK
All tasks started. System ready.
```

To exit the serial monitor, press **Ctrl+C**.

> **Arduino IDE equivalent**: `pio device monitor` = **Tools > Serial Monitor**.
> The baud rate is already set to 115200 in `platformio.ini` (`monitor_speed`),
> so you do not need to select it manually.

---

## Step 8: Connect to the Float

Once the firmware is running:

1. On your phone or laptop, open WiFi settings.
2. Connect to the network:
   - **SSID**: `SSCFloat`
   - **Password**: `DT1234dt`
3. Open a browser and go to `http://192.168.4.1/control`.
4. You should see the control panel with buttons for starting dives.

See the [main README](README.md#using-the-float) for full details on the web
endpoints.

---

## Running Test Sketches

In the Arduino IDE, you would open a different `.ino` file to run a test sketch.
In PlatformIO, the project defines multiple **environments** in `platformio.ini`,
one for each test sketch. You switch between them by name.

The available environments are:

| Environment name | What it does |
|---|---|
| `esp32` | Full production firmware (motor + sensor + WiFi) |
| `test_manual_stepper` | Manual motor control via serial commands |
| `test_pressure` | Pressure sensor test with WiFi data endpoint |
| `test_buttons` | Button and LED verification |
| `wokwi_stepper_debug` | Interactive motor debug (for Wokwi simulation or real hardware) |
| `wokwi_stepper_auto` | Automatic motor test (for Wokwi simulation or real hardware) |

### To build and upload a specific test

Add `-e <environment_name>` to the command. For example, to upload the pressure
sensor test:

```bash
pio run -e test_pressure -t upload
```

To upload the manual stepper test:

```bash
pio run -e test_manual_stepper -t upload
```

After uploading, open the serial monitor as usual:

```bash
pio device monitor
```

### Using the GUI to switch environments

1. Look at the bottom toolbar in VS Code. You should see the current
   environment name (e.g., `esp32`).
2. Click on it. A list of available environments appears at the top of the
   window.
3. Select the one you want.
4. Now the build/upload buttons will use that environment.

> **Arduino IDE equivalent**: Switching environments is like opening a completely
> different sketch, except here all the sketches live in the same project and
> share the same libraries and settings.

---

## Over-The-Air (OTA) Updates

Once the firmware is running, you can upload updates over WiFi instead of USB.
This is useful when the float is assembled and you cannot easily reach the USB
port.

1. Connect your computer to the float's WiFi network (`SSCFloat`).
2. Open `platformio.ini` and uncomment these two lines (remove the `;`):
   ```ini
   upload_protocol = espota
   upload_port = 192.168.4.1
   ```
3. Upload as normal:
   ```bash
   pio run -t upload
   ```

The firmware will be sent over WiFi instead of USB.

**To switch back to USB upload**, comment those two lines out again (add `;` at
the start).

> **Arduino IDE equivalent**: In the Arduino IDE, you would select the network
> port under **Tools > Port**. In PlatformIO, you set it in `platformio.ini`.

---

## Editing the Code

VS Code is a full code editor with features the Arduino IDE does not have:

- **Autocomplete**: Start typing a function name and VS Code suggests completions.
- **Go to definition**: Ctrl+click (Cmd+click on macOS) on any function or
  variable name to jump to where it is defined.
- **Find all references**: Right-click a function name and select
  **"Find All References"** to see everywhere it is used.
- **Syntax errors**: Red underlines appear as you type, before you even build.
- **Integrated terminal**: The terminal is built into the editor — no need to
  switch windows.

### Where to make changes

| What you want to change | File to edit |
|---|---|
| Pin assignments, WiFi name/password, motor speed, dive timings | `include/config.h` |
| Main program flow (setup, loop, task creation) | `src/main.cpp` |
| Motor control logic | `lib/Stepper/FloatStepper.cpp` |
| Pressure sensor readings and buffering | `lib/Sensor/PressureSensor.cpp` |
| WiFi, web endpoints, OTA | `lib/Network/FloatWebServer.cpp` |
| Dive sequences (simple and depth-hold) | `lib/DepthControl/DepthControl.cpp` |

After editing, just build and upload again (`pio run -t upload`).

---

## PlatformIO vs Arduino IDE — Quick Reference

This table translates everything you know from the Arduino IDE into PlatformIO
commands and concepts.

### Common actions

| Arduino IDE | PlatformIO (toolbar) | PlatformIO (terminal) |
|---|---|---|
| Click **Verify** (✓) | Click ✓ in bottom bar | `pio run` |
| Click **Upload** (→) | Click → in bottom bar | `pio run -t upload` |
| **Tools > Serial Monitor** | Click 🔌 in bottom bar | `pio device monitor` |
| **Tools > Board > ESP32 Dev Module** | Already set in `platformio.ini` | — |
| **Tools > Port > COM3** | Auto-detected | `pio run -t upload --upload-port COM3` |
| **Sketch > Include Library > Manage Libraries** | Listed in `platformio.ini` under `lib_deps` | `pio lib install <name>` |
| Open a different `.ino` sketch | Switch environment (`-e <name>`) | `pio run -e test_pressure` |

### Settings

| Arduino IDE (Tools menu) | PlatformIO (`platformio.ini`) |
|---|---|
| Board: ESP32 Dev Module | `board = esp32dev` |
| Upload Speed: 921600 | `upload_speed = 921600` |
| Partition Scheme: Minimal SPIFFS | `board_build.partitions = min_spiffs.csv` |
| Serial monitor baud: 115200 | `monitor_speed = 115200` |
| Framework: Arduino | `framework = arduino` |

### Project layout

| Arduino IDE | PlatformIO |
|---|---|
| `MySketch.ino` | `src/main.cpp` |
| Extra tabs in the sketch | Separate files in `src/` or `lib/` folders |
| `~/Arduino/libraries/` | `lib/` folder in the project (auto-detected) |
| Constants at top of `.ino` | `include/config.h` |
| Board package URL in Preferences | `platform = espressif32` in `platformio.ini` (auto-downloaded) |

---

## Troubleshooting

### PlatformIO extension is not loading

- Close and reopen VS Code.
- Check the bottom toolbar — if you see the PlatformIO icons (house, checkmark,
  arrow), it is loaded.
- If not, go to Extensions, find PlatformIO IDE, and check that it says
  **Enabled**. Try clicking **Disable** then **Enable** and reload.

### First build is very slow

This is normal. PlatformIO is downloading the ESP32 toolchain (~300 MB) and
the MS5837 library. Subsequent builds are much faster (typically a few seconds
for incremental changes).

### "No such file or directory: platformio.ini"

You opened the wrong folder. Go to **File > Open Folder** and select the
`profiling-float-new` folder (the one that contains `platformio.ini`).

### Build succeeds but upload fails

- Check that the ESP32 is plugged in via USB.
- Hold the **BOOT** button while the upload starts.
- Try a different USB cable (charge-only cables will not work).
- On Windows, install the CP2102 or CH340 USB-to-serial driver.

### "Error: Please specify upload_port"

PlatformIO could not auto-detect the serial port. Plug in the ESP32 and try
again. If it still fails, find the port manually:

- **Windows**: Open Device Manager and look under "Ports (COM & LPT)"
- **macOS**: Run `ls /dev/cu.usb*` in a terminal
- **Linux**: Run `ls /dev/ttyUSB*` or `ls /dev/ttyACM*` in a terminal

Then add it to `platformio.ini`:
```ini
upload_port = COM3        ; Windows example
; upload_port = /dev/ttyUSB0  ; Linux example
```

### "ERROR: MS5837 sensor not detected. Halting."

This is a hardware issue, not a PlatformIO issue. The pressure sensor is not
responding. Check:
- SDA is connected to GPIO21, SCL to GPIO22.
- The sensor is powered from **3.3V**, not 5V.
- The I2C wires are not loose.

If you only want to test the motor without the sensor, upload a test sketch
instead:
```bash
pio run -e test_manual_stepper -t upload
```

### Serial monitor shows garbage characters

The baud rate might not match. PlatformIO reads it from `platformio.ini`
(`monitor_speed = 115200`), so this should be automatic. If you opened the
serial monitor from outside PlatformIO (e.g., a separate terminal app), make
sure to set 115200 baud manually.

### WiFi network does not appear

The firmware probably halted during setup. Open the serial monitor
(`pio device monitor`) to see the error message. The most common cause is the
pressure sensor not being detected.

### I want to go back to using the Arduino IDE

See the [Arduino IDE instructions](README.md#option-b-arduino-ide) in the main
README. Note that you will need to manually copy the library files and rename
`main.cpp` to a `.ino` file — PlatformIO handles all of this automatically,
which is why it is recommended.
