#include "FloatStepper.h"
#include "config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ── LEDC configuration ─────────────────────────────────────────
// Uses one LEDC channel to generate step pulses in hardware.
// The A4988 driver needs a minimum 1 µs STEP pulse width; at our
// frequencies (100–833 Hz) even a 1-bit duty cycle exceeds that,
// but we use 8-bit resolution for a clean 50 % waveform.

static const uint8_t  LEDC_CH         = 0;
static const uint8_t  LEDC_RES_BITS   = 8;            // 0–255
static const uint32_t LEDC_DUTY_50PCT = 128;           // 50 % of 256

// ── Limit-switch polling interval while LEDC is running ────────
// 1 FreeRTOS tick ≈ 1 ms on ESP32 — fast enough to stop the motor
// within one step at 833 Hz, and lets the RTOS schedule other work.
static const TickType_t LIMIT_POLL_TICKS = pdMS_TO_TICKS(1);

// ────────────────────────────────────────────────────────────────

void FloatStepper::init(int stepPin, int dirPin,
                         int limitBottomPin, int limitTopPin) {
    _stepPin        = stepPin;
    _dirPin         = dirPin;
    _limitBottomPin = limitBottomPin;
    _limitTopPin    = limitTopPin;
    _speed          = MOTOR_SPEED_DEFAULT;
    _stepCount      = 0;

    pinMode(_stepPin, OUTPUT);
    pinMode(_dirPin, OUTPUT);
    pinMode(_limitBottomPin, INPUT_PULLUP);
    pinMode(_limitTopPin,    INPUT_PULLUP);
}

// ── Software stepping (small batches) ──────────────────────────

void FloatStepper::stepBatch(int steps, bool clockwise) {
    digitalWrite(_dirPin, clockwise ? LOW : HIGH);
    for (int i = 0; i < steps; i++) {
        pulseStep();
        yield();
    }
    _stepCount += clockwise ? steps : -steps;
}

// ── Hardware-accelerated run to limit switch ───────────────────

void FloatStepper::runToLimit(bool clockwise) {
    digitalWrite(_dirPin, clockwise ? LOW : HIGH);

    // clockwise  (descend) → stop when bottom limit is hit
    // counter-cw (ascend)  → stop when top limit is hit
    int limitPin = clockwise ? _limitBottomPin : _limitTopPin;

    startHwPulses();

    while (digitalRead(limitPin) == LOW) {
        vTaskDelay(LIMIT_POLL_TICKS);
    }

    stopHwPulses();

    // Reset position reference when we reach the top (home)
    if (!clockwise) {
        _stepCount = 0;
    }
}

// ── Speed / position accessors ─────────────────────────────────

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
    // Limit switch is active HIGH (internal pull-up, grounded when open)
    return digitalRead(pin) == HIGH;
}

// ── Private helpers ────────────────────────────────────────────

void FloatStepper::pulseStep() {
    digitalWrite(_stepPin, HIGH);
    delayMicroseconds(_speed);
    digitalWrite(_stepPin, LOW);
    delayMicroseconds(_speed);
}

void FloatStepper::startHwPulses() {
    uint32_t freq = speedToFrequencyHz();
    ledcSetup(LEDC_CH, freq, LEDC_RES_BITS);
    ledcAttachPin(_stepPin, LEDC_CH);
    ledcWrite(LEDC_CH, LEDC_DUTY_50PCT);
}

void FloatStepper::stopHwPulses() {
    ledcWrite(LEDC_CH, 0);                // output goes LOW immediately
    ledcDetachPin(_stepPin);               // release pin from LEDC peripheral
    pinMode(_stepPin, OUTPUT);             // reconfigure as regular GPIO
    digitalWrite(_stepPin, LOW);           // ensure idle state
}

uint32_t FloatStepper::speedToFrequencyHz() const {
    // _speed is the half-period in microseconds.
    // Full period = 2 × _speed µs  →  freq = 1 000 000 / (2 × _speed).
    //
    // Default 600 µs → 833 Hz    (~4.2 rev/s at 200 steps/rev)
    // Hold   5000 µs → 100 Hz    (~0.5 rev/s)
    return 1000000UL / (2UL * (uint32_t)_speed);
}
