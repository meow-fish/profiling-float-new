#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <MS5837.h>
#include "config.h"

class PressureSensor {
public:
    /// Initialize I2C and sensor. Returns false if sensor not found.
    bool init(int sdaPin, int sclPin, float fluidDensity);

    /// Take a new reading from the sensor hardware.
    void read();

    /// Latest pressure in mbar.
    float getPressure() const;

    /// Latest temperature in Celsius.
    float getTemperature() const;

    /// Compute the target pressure (mbar) for a given depth in meters.
    /// Formula: atmospheric + (density * g * depth) / 100
    float calculateTargetPressure(float depthMeters) const;

    /// Store current reading into the circular buffer.
    /// Call this from the sensor task after read().
    void bufferReading();

    /// Copy buffered data into caller-provided arrays.
    /// Returns the number of valid entries written.
    int getBufferedData(float* pressures, float* temperatures, int maxEntries) const;

    /// How many readings have been buffered in total.
    int getTotalReadings() const;

private:
    MS5837 _sensor;
    float  _fluidDensity;
    float  _lastPressure;
    float  _lastTemperature;

    // Circular buffer
    float _pressures[SENSOR_BUFFER_SIZE];
    float _temperatures[SENSOR_BUFFER_SIZE];
    int   _bufIdx;
    int   _totalReadings;
};

#endif // PRESSURE_SENSOR_H
