#include "PressureSensor.h"

bool PressureSensor::init(int sdaPin, int sclPin, float fluidDensity) {
    _fluidDensity   = fluidDensity;
    _lastPressure   = 0.0f;
    _lastTemperature = 0.0f;
    _bufIdx         = 0;
    _totalReadings  = 0;

    Wire.begin(sdaPin, sclPin);

    if (!_sensor.init()) {
        return false;
    }

    _sensor.setFluidDensity(fluidDensity);
    return true;
}

void PressureSensor::read() {
    _sensor.read();
    _lastPressure    = _sensor.pressure();
    _lastTemperature = _sensor.temperature();
}

float PressureSensor::getPressure() const {
    return _lastPressure;
}

float PressureSensor::getTemperature() const {
    return _lastTemperature;
}

float PressureSensor::calculateTargetPressure(float depthMeters) const {
    // Convert physical depth to expected pressure reading in mbar
    return ATMOSPHERIC_PRESSURE + (_fluidDensity * GRAVITY * depthMeters) / 100.0f;
}

void PressureSensor::bufferReading() {
    _pressures[_bufIdx]    = _lastPressure;
    _temperatures[_bufIdx] = _lastTemperature;
    _bufIdx = (_bufIdx + 1) % SENSOR_BUFFER_SIZE;
    _totalReadings++;
}

int PressureSensor::getBufferedData(float* pressures, float* temperatures, int maxEntries) const {
    int count = (_totalReadings < SENSOR_BUFFER_SIZE) ? _totalReadings : SENSOR_BUFFER_SIZE;
    if (count > maxEntries) count = maxEntries;

    // Read from oldest to newest in the circular buffer
    for (int i = 0; i < count; i++) {
        int idx = (_bufIdx + i) % SENSOR_BUFFER_SIZE;
        pressures[i]    = _pressures[idx];
        temperatures[i] = _temperatures[idx];
    }
    return count;
}

int PressureSensor::getTotalReadings() const {
    return _totalReadings;
}
