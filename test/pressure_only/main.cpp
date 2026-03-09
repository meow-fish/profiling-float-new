// Standalone test: MS5837 pressure/temperature sensor with WiFi data endpoint.
// Reads sensor every 1 second, serves CSV at http://192.168.4.1/data

#include <Arduino.h>
#include <Wire.h>
#include <MS5837.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"

MS5837 sensor;
WebServer server(80);

float pressures[10];
float temperatures[10];
int idx = 0;
String iterationData = "";

void handleData() {
    server.send(200, "text/plain", iterationData);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Wire.begin(PIN_SDA, PIN_SCL);

    if (!sensor.init()) {
        Serial.println("ERROR: MS5837 sensor not detected!");
        while (1);
    }
    sensor.setFluidDensity(FLUID_DENSITY);
    Serial.println("MS5837 sensor OK");

    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    server.on("/data", handleData);
    server.begin();
}

void loop() {
    sensor.read();
    pressures[idx]    = sensor.pressure();
    temperatures[idx] = sensor.temperature();
    idx++;

    if (idx >= 10) {
        iterationData = "";
        for (int i = 0; i < 10; i++) {
            iterationData += String(pressures[i]) + "," +
                             String(temperatures[i]) + "\n";
        }
        idx = 0;
    }

    server.handleClient();
    delay(1000);
}
