# Profiling Float New

Based on Ryan Yeung's Profilling Float
Improved by Li Jiale Darren (Vibe-coding)

An ESP32-based automated profiling float that sinks and resurfaces in water while collecting pressure and temperature data. A stepper motor drives a syringe/piston mechanism to change the float's buoyancy, and an MS5837 sensor records depth and temperature throughout each dive cycle.

## Table of Contents

- [How It Works](#how-it-works)
- [Hardware Requirements](#hardware-requirements)
- [Pin Connections](#pin-connections)
- [Software Setup](#software-setup)
  - [Option A: PlatformIO (Recommended)](#option-a-platformio-recommended)
  - [Option B: Arduino IDE](#option-b-arduino-ide)
- [Flashing the ESP32](#flashing-the-esp32)
  - [USB Upload](#usb-upload)
  - [Over-The-Air (OTA) Upload](#over-the-air-ota-upload)
- [Using the Float](#using-the-float)
  - [Connecting to the Float](#connecting-to-the-float)
  - [Web Endpoints](#web-endpoints)
  - [Simple Dive Sequence](#simple-dive-sequence)
  - [Depth-Hold Dive](#depth-hold-dive)
  - [Reading Sensor Data](#reading-sensor-data)
- [Dashboard (Graph Generator)](#dashboard-graph-generator)
- [Configuration](#configuration)
- [Project Structure](#project-structure)
- [Running Test Sketches](#running-test-sketches)
- [Troubleshooting](#troubleshooting)

---

## How It Works

1. The ESP32 creates a WiFi access point you can connect to from any device.
2. A pressure/temperature sensor (MS5837) takes readings every second and stores them in a circular buffer of 120 entries.
3. When a dive is triggered via the web interface, a stepper motor drives a syringe mechanism:
   - **Simple mode**: Motor spins clockwise (descend) until the bottom limit switch is hit, waits 45 seconds for data collection, then spins counter-clockwise (ascend) until the top limit switch is hit.
   - **Depth-hold mode**: Motor homes to the top, descends to a target position, then actively adjusts up/down using pressure feedback to maintain a target depth for a configurable duration.
4. Sensor data can be retrieved at any time via the `/data` endpoint or visualised using the companion Flask dashboard.

## Hardware Requirements

| Component | Notes |
|---|---|
| ESP32 Development Board | Any ESP32-WROOM-32 based board |
| MS5837-30BA Pressure/Temperature Sensor | **3.3V only** — do not connect to 5V |
| Stepper Motor (NEMA 17 or similar) | With A4988 or similar driver |
| 2x Limit Switches | Wired normally-open to GND |
| Power Supply | Match your motor driver requirements |
| Waterproof Housing | For the sensor and float body |
| Syringe/Piston Mechanism | Driven by the stepper to change buoyancy |

## Pin Connections

```
ESP32 Pin    Function              Wire Colour    Notes
─────────    ────────              ───────────    ─────
GPIO4  (D4)  Stepper STEP pulse    Green          → Driver STEP input
GPIO5  (D5)  Stepper DIRECTION     Blue           → Driver DIR input
GPIO21 (D21) I2C SDA               White          → MS5837 SDA (3.3V!)
GPIO22 (D22) I2C SCL               Green          → MS5837 SCL (3.3V!)
GPIO13       Bottom limit switch   —              → Switch to GND
GPIO14       Top limit switch      —              → Switch to GND
```

Limit switches are wired between the GPIO pin and GND. The ESP32's internal pull-up resistors are enabled in software, so no external resistors are needed. When the switch is pressed, the pin reads LOW.

## Software Setup

### Option A: PlatformIO (Recommended)

PlatformIO handles all dependencies and board configuration automatically.

1. Install PlatformIO:
   - **VS Code**: Install the "PlatformIO IDE" extension from the marketplace.
   - **CLI**: `pip install platformio`

2. Clone and build:
   ```bash
   git clone <this-repo-url>
   cd profiling-float
   pio run                 # Build the firmware
   pio run -t upload       # Build and upload via USB
   pio device monitor      # Open serial monitor (115200 baud)
   ```

All library dependencies (MS5837, WiFi, etc.) are declared in `platformio.ini` and installed automatically on first build.

### Option B: Arduino IDE

If you prefer the Arduino IDE:

1. Open Arduino IDE and go to **File > Preferences**.
2. In "Additional Board Manager URLs", add:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
3. Go to **Tools > Board > Board Manager**, search for "esp32", and install the **ESP32** package by Espressif.
4. Install the required library:
   - Go to **Sketch > Include Library > Manage Libraries**
   - Search for and install: **BlueRobotics MS5837 Library**
5. Board settings:
   - Board: "ESP32 Dev Module"
   - Upload Speed: 921600 (or lower if uploads fail)
   - Flash Frequency: 80MHz
   - Partition Scheme: "Minimal SPIFFS"
6. To use the modular code with Arduino IDE, you need to either:
   - Flatten the `src/main.cpp` and all `lib/` files into a single `.ino` sketch, **or**
   - Copy the `lib/` folders into your Arduino libraries directory (`~/Arduino/libraries/`).

   PlatformIO is strongly recommended to avoid this hassle.

## Flashing the ESP32

### USB Upload

1. Connect the ESP32 to your computer via USB.
2. With PlatformIO:
   ```bash
   pio run -t upload
   ```
3. With Arduino IDE: click the Upload button (or Ctrl+U).
4. If the upload fails, hold the **BOOT** button on the ESP32 while the upload starts.

### Over-The-Air (OTA) Upload

Once the firmware is running, you can flash updates over WiFi without disassembling the float:

1. Connect your computer to the float's WiFi network (see [Connecting to the Float](#connecting-to-the-float)).
2. With PlatformIO, uncomment these lines in `platformio.ini`:
   ```ini
   upload_protocol = espota
   upload_port = 192.168.4.1
   ```
   Then run:
   ```bash
   pio run -t upload
   ```
3. With Arduino IDE:
   - Go to **Tools > Port** and select the network port that appears (e.g., `192.168.4.1`).
   - Upload normally.

**Important**: Always keep the OTA code in the firmware. If you upload firmware without OTA support, you will need USB access to recover.

## Using the Float

### Connecting to the Float

1. Power on the ESP32. Wait a few seconds for it to boot.
2. On your phone/laptop, connect to the WiFi network:
   - **SSID**: `SSCFloat`
   - **Password**: `DT1234dt`
3. The float's IP address is `192.168.4.1`.

### Web Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `/control` | GET | Control panel — buttons to start dives |
| `/control?action=start` | GET | Trigger a simple dive sequence |
| `/depthhold?depth=N` | GET | Start depth-hold dive at N meters |
| `/data` | GET | Download sensor data as CSV |
| `/status` | GET | JSON: motor busy state, current pressure/temp |

### Simple Dive Sequence

1. Open `http://192.168.4.1/control` in a browser.
2. Click **"Start Stepper Sequence"**.
3. The float will:
   - Spin the motor clockwise (descend) until the bottom limit switch is triggered.
   - Wait 45 seconds at the bottom while collecting data.
   - Spin the motor counter-clockwise (ascend) until the top limit switch is triggered.
4. While the motor is running, the button is disabled. Refresh the page to check status.

### Depth-Hold Dive

1. Open `http://192.168.4.1/control`.
2. Enter a target depth in meters and duration in seconds in the form fields.
3. Click **"Start Depth Hold"**.
4. The float will:
   - Ascend to the top limit switch (home position).
   - Descend to the target step position.
   - Actively hold depth using pressure feedback for the specified duration.
   - Ascend back to the top when done.

You can also trigger it directly: `http://192.168.4.1/depthhold?depth=2`

### Reading Sensor Data

- **Raw CSV**: Visit `http://192.168.4.1/data` — returns up to 120 lines of `pressure_mbar,temperature_c`.
- **JSON status**: Visit `http://192.168.4.1/status` — returns current readings and motor state.
- **Graphs**: Use the [Dashboard](#dashboard-graph-generator) to paste CSV data and generate charts.

## Dashboard (Graph Generator)

A companion Flask web app for visualising sensor data.

### Setup

```bash
cd dashboard
pip install flask
python controller.py
```

This starts a web server at `http://localhost:3900`.

### Usage

1. Open `http://localhost:3900` in a browser.
2. Click **"Sink Float"** to trigger a dive (opens the ESP32's `/control` page).
3. Click **"Float Pressure Data"** to view raw CSV data from the ESP32.
4. Click **"Generate Graph"** to open the graph tool:
   - Copy the CSV data from `/data` and paste it into the text box.
   - Click **"Generate Graphs"** to plot depth and temperature over time.

## Configuration

All configurable parameters are in [`include/config.h`](include/config.h). Edit this single file to change any hardware or operational setting:

```c
// Pin assignments
#define PIN_STEP           4      // Stepper step pulse
#define PIN_DIR            5      // Stepper direction
#define PIN_SDA            21     // I2C data
#define PIN_SCL            22     // I2C clock
#define PIN_LIMIT_BOTTOM   13     // Bottom limit switch
#define PIN_LIMIT_TOP      14     // Top limit switch

// WiFi
#define WIFI_SSID          "SSCFloat"
#define WIFI_PASSWORD      "DT1234dt"

// Motor
#define MOTOR_SPEED_DEFAULT   600   // us between steps (normal speed)
#define MOTOR_SPEED_HOLD      5000  // us between steps (depth-hold corrections)

// Dive behaviour
#define BOTTOM_DWELL_MS       45000   // Wait time at bottom (simple mode)
#define DEPTHHOLD_TIMEOUT_MS  120000  // Max depth-hold duration
#define DEPTHHOLD_MIDPOINT    3300    // Default descent in steps
#define DEPTHHOLD_RANGE_STEPS 300     // +/- correction range
```

**If you are running multiple floats, change `WIFI_SSID` to a unique name for each one** — otherwise the SSIDs will clash.

## Project Structure

```
profiling-float/
  platformio.ini              # Build config and dependencies
  include/
    config.h                  # All hardware/operational constants
  src/
    main.cpp                  # Entry point: setup, loop, FreeRTOS tasks
  lib/
    Stepper/
      FloatStepper.h/.cpp     # Stepper motor driver (step, direction, limits)
    Sensor/
      PressureSensor.h/.cpp   # MS5837 wrapper, circular buffer, depth calc
    Network/
      FloatWebServer.h/.cpp   # WiFi AP, HTTP endpoints, OTA
    DepthControl/
      DepthControl.h/.cpp     # Dive strategies (simple & depth-hold)
  test/
    manual_stepper/main.cpp   # Test: manual button motor control
    pressure_only/main.cpp    # Test: sensor + WiFi data endpoint
    button_test/main.cpp      # Test: button and LED verification
  dashboard/
    controller.py             # Flask graph generator / control panel
  data/static/js/
    chart.js                  # Chart.js library for graphing
  archive/
    esp8266/                  # Old ESP8266 firmware (reference only)
    legacy/                   # Original Arduino Uno + Raspberry Pi code
    pi_serial_tests/          # Serial communication test scripts
```

### Module Overview

| Module | Responsibility |
|---|---|
| **FloatStepper** | Low-level motor control: stepping, direction, limit switch reading, position tracking |
| **PressureSensor** | Sensor initialisation, reading, circular buffer, target pressure calculation |
| **FloatWebServer** | WiFi access point, OTA updates, all HTTP endpoint handlers |
| **DepthController** | High-level dive logic: simple sequence and depth-hold with pressure feedback |
| **main.cpp** | Wires modules together, creates FreeRTOS tasks |

## Running Test Sketches

Test sketches let you verify individual components without running the full firmware.

```bash
# Manual stepper control (two buttons)
pio run -e test_manual_stepper -t upload

# Pressure sensor only (with WiFi data endpoint)
pio run -e test_pressure -t upload

# Button and LED test
pio run -e test_buttons -t upload
```

Each test environment is defined in `platformio.ini` and builds only the relevant test file.

## Troubleshooting

| Problem | Cause | Fix |
|---|---|---|
| WiFi network doesn't appear | Sensor not detected — system halts in `setup()` | Check MS5837 wiring. **Sensor is 3.3V only** — 5V will give bad readings or damage it. |
| Pressure readings are negative or >1900 | Sensor power issue | Verify sensor is powered from 3.3V, not 5V. |
| Motor doesn't stop at top/bottom | Limit switch wiring inverted | Swap the two switch connections (GPIO13 ↔ GPIO14). |
| Motor doesn't move at all | Driver not powered or wired wrong | Check motor driver power supply and STEP/DIR connections to GPIO4/GPIO5. |
| OTA upload fails | Not connected to float WiFi, or port wrong | Connect to `SSCFloat` WiFi first. Ensure `upload_port = 192.168.4.1` in `platformio.ini`. |
| OTA upload hangs | Previous OTA attempt corrupted flash | Reflash via USB. |
| Dashboard can't reach float | Computer not on float's WiFi | The dashboard at `localhost:3900` links to `192.168.4.1` — you must be connected to the float's WiFi. |
| Multiple floats interfere | Same SSID | Change `WIFI_SSID` in `config.h` to a unique name per float. |
| Build fails: "MS5837.h not found" | Library not installed | PlatformIO: run `pio lib install`. Arduino IDE: install "BlueRobotics MS5837 Library" from Library Manager. |
