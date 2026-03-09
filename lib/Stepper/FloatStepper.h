#ifndef FLOAT_STEPPER_H
#define FLOAT_STEPPER_H

#include <Arduino.h>

class FloatStepper {
public:
    /// Initialize pins and state. Call once in setup().
    void init(int stepPin, int dirPin, int limitBottomPin, int limitTopPin);

    /// Step a fixed number of pulses in the given direction.
    /// clockwise=true → direction pin LOW (descend).
    void stepBatch(int steps, bool clockwise);

    /// Run continuously in one direction until the corresponding limit
    /// switch is triggered. clockwise=true runs until bottom limit,
    /// clockwise=false runs until top limit.
    void runToLimit(bool clockwise);

    /// Change the pulse delay (microseconds between step edges).
    void setSpeed(int microsBetweenSteps);

    /// Get current speed setting.
    int  getSpeed() const;

    /// Cumulative step position (increases on CW, decreases on CCW).
    int  getStepCount() const;

    /// Reset the step counter to zero (call after homing).
    void resetStepCount();

    /// Read a limit switch. bottom=true reads the bottom switch.
    bool isAtLimit(bool bottom) const;

private:
    int _stepPin;
    int _dirPin;
    int _limitBottomPin;
    int _limitTopPin;
    int _speed;
    int _stepCount;

    void pulseStep();
};

#endif // FLOAT_STEPPER_H
