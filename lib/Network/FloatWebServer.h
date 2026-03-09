#ifndef FLOAT_WEB_SERVER_H
#define FLOAT_WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "config.h"

// Forward declarations — avoids circular includes
class PressureSensor;
class FloatStepper;

/// Shared state between the web server and motor task.
struct MotorState {
    volatile bool       busy;
    SemaphoreHandle_t   lock;
    QueueHandle_t       commandQueue;
};

class FloatWebServer {
public:
    /// Start WiFi AP and HTTP server.
    void init(const char* ssid, const char* password);

    /// Set up ArduinoOTA handlers.
    void initOTA();

    /// Wire up endpoints. Call after all modules are initialised.
    void registerEndpoints(PressureSensor& sensor,
                           SemaphoreHandle_t sensorLock,
                           MotorState& motor);

    /// Call from the web FreeRTOS task loop.
    void handleClient();

    /// Call from the main loop() for OTA polling.
    void handleOTA();

private:
    WebServer _server{80};

    // Pointers set during registerEndpoints()
    PressureSensor*    _sensor;
    SemaphoreHandle_t  _sensorLock;
    MotorState*        _motor;

    // Endpoint handlers
    void handleData();
    void handleControl();
    void handleDepthHold();
    void handleStatus();
};

#endif // FLOAT_WEB_SERVER_H
