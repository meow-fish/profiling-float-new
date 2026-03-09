#include "FloatWebServer.h"
#include "PressureSensor.h"
#include "FloatStepper.h"

// ── WiFi & Server ────────────────────────────────────────────

void FloatWebServer::init(const char* ssid, const char* password) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    Serial.print("Access Point started. IP: ");
    Serial.println(WiFi.softAPIP());
}

void FloatWebServer::initOTA() {
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nUpdate Complete");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress * 100) / total);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if      (error == OTA_AUTH_ERROR)    Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)     Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("OTA Ready");
}

void FloatWebServer::registerEndpoints(PressureSensor& sensor,
                                        SemaphoreHandle_t sensorLock,
                                        MotorState& motor) {
    _sensor     = &sensor;
    _sensorLock = sensorLock;
    _motor      = &motor;

    _server.on("/data",      [this]() { handleData(); });
    _server.on("/control",   [this]() { handleControl(); });
    _server.on("/depthhold", [this]() { handleDepthHold(); });
    _server.on("/status",    [this]() { handleStatus(); });

    _server.begin();
    Serial.println("Web server started");
}

void FloatWebServer::handleClient() {
    _server.handleClient();
}

void FloatWebServer::handleOTA() {
    ArduinoOTA.handle();
}

// ── Endpoint: /data ──────────────────────────────────────────

void FloatWebServer::handleData() {
    if (xSemaphoreTake(_sensorLock, portMAX_DELAY)) {
        float pressures[SENSOR_BUFFER_SIZE];
        float temperatures[SENSOR_BUFFER_SIZE];
        int count = _sensor->getBufferedData(pressures, temperatures, SENSOR_BUFFER_SIZE);
        xSemaphoreGive(_sensorLock);

        String csv;
        csv.reserve(count * 24);
        for (int i = 0; i < count; i++) {
            csv += String(pressures[i]) + "," + String(temperatures[i]) + "\n";
        }
        _server.send(200, "text/plain", csv);
    }
}

// ── Endpoint: /control?action=start ──────────────────────────

void FloatWebServer::handleControl() {
    if (_server.hasArg("action") && _server.arg("action") == "start") {
        bool canStart = false;
        if (xSemaphoreTake(_motor->lock, portMAX_DELAY)) {
            canStart = !_motor->busy;
            xSemaphoreGive(_motor->lock);
        }

        if (canStart) {
            // Command value: 0 = simple dive sequence
            int command = 0;
            xQueueSend(_motor->commandQueue, &command, portMAX_DELAY);
            _server.send(200, "text/html",
                "<html><body>"
                "<p>Stepper sequence started.</p>"
                "<p>Motor is busy. Please wait for completion.</p>"
                "<a href='/control'>Back</a>"
                "</body></html>");
        } else {
            _server.send(200, "text/html",
                "<html><body>"
                "<p>Motor is currently busy. Please wait.</p>"
                "<a href='/control'>Back</a>"
                "</body></html>");
        }
    } else {
        bool isBusy = false;
        if (xSemaphoreTake(_motor->lock, portMAX_DELAY)) {
            isBusy = _motor->busy;
            xSemaphoreGive(_motor->lock);
        }

        String html = "<html><body>";
        if (isBusy) {
            html += "<p>Motor is currently running...</p>";
            html += "<button disabled>Start Stepper Sequence</button>";
        } else {
            html += "<button onclick=\"location.href='/control?action=start'\">"
                    "Start Stepper Sequence</button>";
        }
        html += "<br><br>";
        html += "<form action='/depthhold' method='get'>"
                "Depth (meters): <input type='number' name='depth' value='1'>"
                " Duration (s): <input type='number' name='duration' value='120'>"
                " <button type='submit'>Start Depth Hold</button>"
                "</form>";
        html += "</body></html>";
        _server.send(200, "text/html", html);
    }
}

// ── Endpoint: /depthhold?depth=N&duration=S ──────────────────

void FloatWebServer::handleDepthHold() {
    if (!_server.hasArg("depth")) {
        _server.send(400, "text/html",
            "<html><body>"
            "<p>Missing 'depth' parameter.</p>"
            "<a href='/control'>Back</a>"
            "</body></html>");
        return;
    }

    bool canStart = false;
    if (xSemaphoreTake(_motor->lock, portMAX_DELAY)) {
        canStart = !_motor->busy;
        xSemaphoreGive(_motor->lock);
    }

    if (!canStart) {
        _server.send(200, "text/html",
            "<html><body>"
            "<p>Motor is currently busy. Please wait.</p>"
            "<a href='/control'>Back</a>"
            "</body></html>");
        return;
    }

    int depth = _server.arg("depth").toInt();
    // Encode depth as a positive command value (0 = simple, >0 = depth-hold in meters)
    // We add 1 to ensure depth=0 is distinguishable from simple mode
    int command = depth + 1;
    xQueueSend(_motor->commandQueue, &command, portMAX_DELAY);

    _server.send(200, "text/html",
        "<html><body>"
        "<p>Depth hold started for " + String(depth) + " meters.</p>"
        "<a href='/control'>Back</a>"
        "</body></html>");
}

// ── Endpoint: /status (JSON) ─────────────────────────────────

void FloatWebServer::handleStatus() {
    bool isBusy = false;
    if (xSemaphoreTake(_motor->lock, portMAX_DELAY)) {
        isBusy = _motor->busy;
        xSemaphoreGive(_motor->lock);
    }

    float pressure = 0, temperature = 0;
    if (xSemaphoreTake(_sensorLock, portMAX_DELAY)) {
        pressure    = _sensor->getPressure();
        temperature = _sensor->getTemperature();
        xSemaphoreGive(_sensorLock);
    }

    String json = "{";
    json += "\"motor_busy\":" + String(isBusy ? "true" : "false") + ",";
    json += "\"pressure_mbar\":" + String(pressure) + ",";
    json += "\"temperature_c\":" + String(temperature);
    json += "}";

    _server.send(200, "application/json", json);
}
