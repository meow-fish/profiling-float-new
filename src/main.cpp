#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "config.h"
#include "FloatStepper.h"
#include "PressureSensor.h"
#include "FloatWebServer.h"
#include "DepthControl.h"

// ── Module instances ─────────────────────────────────────────

FloatStepper    stepper;
PressureSensor  sensor;
FloatWebServer  webServer;
DepthController depthController;

// ── Shared state ─────────────────────────────────────────────

SemaphoreHandle_t sensorLock      = NULL;
MotorState        motorState;

// ── FreeRTOS task handles ────────────────────────────────────

TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t webTaskHandle    = NULL;
TaskHandle_t motorTaskHandle  = NULL;

// ── Sensor reading task (Core 0) ─────────────────────────────

void sensorTask(void* parameter) {
    while (1) {
        sensor.read();

        if (xSemaphoreTake(sensorLock, portMAX_DELAY)) {
            sensor.bufferReading();
            xSemaphoreGive(sensorLock);
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL));
    }
}

// ── Web server task (Core 0) ─────────────────────────────────

void webTask(void* parameter) {
    while (1) {
        webServer.handleClient();
        vTaskDelay(pdMS_TO_TICKS(WEB_TASK_DELAY_MS));
    }
}

// ── Motor task (Core 1) ──────────────────────────────────────
// Waits for commands on the queue.
//   command == 0  → simple dive (limit-to-limit)
//   command >= 1  → depth-hold at (command - 1) meters

void motorTask(void* parameter) {
    int command;
    while (1) {
        if (xQueueReceive(motorState.commandQueue, &command, portMAX_DELAY)) {
            // Mark busy
            if (xSemaphoreTake(motorState.lock, portMAX_DELAY)) {
                motorState.busy = true;
                xSemaphoreGive(motorState.lock);
            }

            // Execute the requested dive
            if (command == 0) {
                depthController.startDive(DepthController::simpleParams());
            } else {
                float depthMeters = (float)(command - 1);
                depthController.startDive(DepthController::depthHoldParams(depthMeters));
            }

            // Mark idle
            if (xSemaphoreTake(motorState.lock, portMAX_DELAY)) {
                motorState.busy = false;
                xSemaphoreGive(motorState.lock);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ── Arduino setup ────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Profiling Float — initialising...");

    // Pressure sensor
    if (!sensor.init(PIN_SDA, PIN_SCL, FLUID_DENSITY)) {
        Serial.println("ERROR: MS5837 sensor not detected. Halting.");
        while (1);
    }
    Serial.println("MS5837 sensor OK");

    // Stepper motor
    stepper.init(PIN_STEP, PIN_DIR, PIN_LIMIT_BOTTOM, PIN_LIMIT_TOP);
    Serial.println("Stepper motor OK");

    // Depth controller
    depthController.init(stepper, sensor);

    // WiFi & web server
    webServer.init(WIFI_SSID, WIFI_PASSWORD);
    webServer.initOTA();

    // Create synchronisation primitives
    sensorLock              = xSemaphoreCreateMutex();
    motorState.lock         = xSemaphoreCreateMutex();
    motorState.commandQueue = xQueueCreate(1, sizeof(int));
    motorState.busy         = false;

    // Register web endpoints (needs sensor lock & motor state)
    webServer.registerEndpoints(sensor, sensorLock, motorState);

    // Launch FreeRTOS tasks
    xTaskCreatePinnedToCore(sensorTask, "SensorTask", TASK_STACK_SIZE, NULL,
                            SENSOR_TASK_PRIORITY, &sensorTaskHandle, SENSOR_TASK_CORE);

    xTaskCreatePinnedToCore(webTask, "WebTask", TASK_STACK_SIZE, NULL,
                            WEB_TASK_PRIORITY, &webTaskHandle, WEB_TASK_CORE);

    xTaskCreatePinnedToCore(motorTask, "MotorTask", TASK_STACK_SIZE, NULL,
                            MOTOR_TASK_PRIORITY, &motorTaskHandle, MOTOR_TASK_CORE);

    Serial.println("All tasks started. System ready.");
}

// ── Arduino loop ─────────────────────────────────────────────

void loop() {
    webServer.handleOTA();
    vTaskDelay(pdMS_TO_TICKS(1000));
}
