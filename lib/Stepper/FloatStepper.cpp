#include "FloatStepper.h"
#include "config.h"

void FloatStepper::init(int stepPin, int dirPin, int limitBottomPin, int limitTopPin) {
    _stepPin       = stepPin;
    _dirPin        = dirPin;
    _limitBottomPin = limitBottomPin;
    _limitTopPin   = limitTopPin;
    _speed         = MOTOR_SPEED_DEFAULT;
    _stepCount     = 0;

    pinMode(_stepPin, OUTPUT);
    pinMode(_dirPin, OUTPUT);
    pinMode(_limitBottomPin, INPUT_PULLUP);
    pinMode(_limitTopPin, INPUT_PULLUP);
}

void FloatStepper::stepBatch(int steps, bool clockwise) {
    digitalWrite(_dirPin, clockwise ? LOW : HIGH);
    for (int i = 0; i < steps; i++) {
        pulseStep();
        yield();
    }
    _stepCount += clockwise ? steps : -steps;
}

void FloatStepper::runToLimit(bool clockwise) {
    digitalWrite(_dirPin, clockwise ? LOW : HIGH);
    // clockwise (descend) → stop when bottom limit is hit
    // counter-clockwise (ascend) → stop when top limit is hit
    int limitPin = clockwise ? _limitBottomPin : _limitTopPin;

    while (digitalRead(limitPin) == LOW) {
        pulseStep();
        taskYIELD();
    }

    // Reset position reference when we reach a known limit
    if (!clockwise) {
        _stepCount = 0; // top = home position
    }
}

void FloatStepper::setSpeed(int microsBetweenSteps) {
    _speed = microsBetweenSteps;
}

int FloatStepper::getSpeed() const {
    return _speed;
}

int FloatStepper::getStepCount() const {
    return _stepCount;
}

void FloatStepper::resetStepCount() {
    _stepCount = 0;
}

bool FloatStepper::isAtLimit(bool bottom) const {
    int pin = bottom ? _limitBottomPin : _limitTopPin;
    // Limit switch is active LOW (pulled up, grounded when pressed)
    return digitalRead(pin) == HIGH;
}

void FloatStepper::pulseStep() {
    digitalWrite(_stepPin, HIGH);
    delayMicroseconds(_speed);
    digitalWrite(_stepPin, LOW);
    delayMicroseconds(_speed);
}
