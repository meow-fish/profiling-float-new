#ifndef CONFIG_H
#define CONFIG_H

// =============================================================
// Pin Assignments
// =============================================================

// Stepper motor driver
#define PIN_STEP           4   // Step pulse → GPIO4 (D4)
#define PIN_DIR            5   // Direction  → GPIO5 (D5)

// I2C for MS5837 pressure/temperature sensor
#define PIN_SDA            21  // SDA → GPIO21 (D21)
#define PIN_SCL            22  // SCL → GPIO22 (D22)

// Limit switches (wired to GND, using internal pull-ups)
#define PIN_LIMIT_BOTTOM   13  // Bottom limit switch → GPIO13
#define PIN_LIMIT_TOP      14  // Top limit switch    → GPIO14

// =============================================================
// WiFi Configuration
// =============================================================

#define WIFI_SSID          "SSCFloat"
#define WIFI_PASSWORD      "DT1234dt"

// =============================================================
// Stepper Motor Parameters
// =============================================================

#define MOTOR_SPEED_DEFAULT   600    // Microseconds between step pulses (normal)
#define MOTOR_SPEED_HOLD      5000   // Microseconds between step pulses (depth-hold)
#define MOTOR_STEP_BATCH      10     // Steps per correction in depth-hold loop

// =============================================================
// Pressure Sensor Parameters
// =============================================================

#define FLUID_DENSITY          997.0f    // kg/m^3 (freshwater)
#define GRAVITY                9.80665f  // m/s^2
#define ATMOSPHERIC_PRESSURE   1013.25f  // mbar at sea level

// =============================================================
// Sensor Buffer
// =============================================================

#define SENSOR_BUFFER_SIZE     120       // Circular buffer capacity (readings)
#define SENSOR_READ_INTERVAL   1000      // Milliseconds between sensor reads

// =============================================================
// Dive Sequence Parameters
// =============================================================

#define BOTTOM_DWELL_MS            45000     // Dwell time at bottom (ms)
#define DEPTHHOLD_TIMEOUT_MS       120000    // Max depth-hold duration (ms)
#define DEPTHHOLD_RANGE_STEPS      300       // +/- step tolerance around midpoint
#define DEPTHHOLD_MIDPOINT         3300      // Default midpoint in steps
#define DEPTHHOLD_DEADBAND_MBAR    5.0f      // Pressure deadband (mbar, ≈ 5 cm water)
#define DEPTHHOLD_LOOP_INTERVAL_MS 200       // Control loop period (ms, 5 Hz)

// =============================================================
// FreeRTOS Task Configuration
// =============================================================

#define TASK_STACK_SIZE        4096

#define SENSOR_TASK_PRIORITY   2
#define SENSOR_TASK_CORE       0

#define WEB_TASK_PRIORITY      1
#define WEB_TASK_CORE          0
#define WEB_TASK_DELAY_MS      10

#define MOTOR_TASK_PRIORITY    3
#define MOTOR_TASK_CORE        1

#endif // CONFIG_H
