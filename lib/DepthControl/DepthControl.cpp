#include "DepthControl.h"
#include "FloatStepper.h"
#include "PressureSensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void DepthController::init(FloatStepper& stepper, PressureSensor& sensor) {
    _stepper = &stepper;
    _sensor  = &sensor;
}

void DepthController::startDive(const DiveParams& params) {
    switch (params.mode) {
        case DiveMode::SIMPLE:
            runSimpleDive(params);
            break;
        case DiveMode::DEPTH_HOLD:
            runDepthHold(params);
            break;
    }
}

DiveParams DepthController::simpleParams() {
    DiveParams p;
    p.mode              = DiveMode::SIMPLE;
    p.targetDepthSteps  = 0;
    p.targetDepthMeters = 0;
    p.holdDurationMs    = 0;
    p.dwellAtBottomMs   = BOTTOM_DWELL_MS;
    p.holdRangeSteps    = 0;
    return p;
}

DiveParams DepthController::depthHoldParams(float depthMeters) {
    DiveParams p;
    p.mode              = DiveMode::DEPTH_HOLD;
    p.targetDepthSteps  = DEPTHHOLD_MIDPOINT;
    p.targetDepthMeters = depthMeters;
    p.holdDurationMs    = DEPTHHOLD_TIMEOUT_MS;
    p.dwellAtBottomMs   = 0;
    p.holdRangeSteps    = DEPTHHOLD_RANGE_STEPS;
    return p;
}

// ── Simple dive: descend → dwell → ascend ────────────────────

void DepthController::runSimpleDive(const DiveParams& params) {
    // Descend (clockwise) until bottom limit switch
    _stepper->runToLimit(true);

    // Dwell at bottom
    delay(params.dwellAtBottomMs);

    // Ascend (counter-clockwise) until top limit switch
    _stepper->runToLimit(false);
}

// ── Depth-hold dive: home → descend → hold → ascend ─────────

void DepthController::runDepthHold(const DiveParams& params) {
    // 1. Home: ascend to top limit switch to establish reference
    _stepper->runToLimit(false);
    _stepper->resetStepCount();

    // 2. Descend to target step position
    while (!_stepper->isAtLimit(true) &&
           _stepper->getStepCount() < params.targetDepthSteps) {
        _stepper->stepBatch(MOTOR_STEP_BATCH, true);
    }

    // 3. Switch to slower speed for fine corrections
    int savedSpeed = _stepper->getSpeed();
    _stepper->setSpeed(MOTOR_SPEED_HOLD);

    // 4. Compute target pressure from physical depth
    float targetPressure = _sensor->calculateTargetPressure(params.targetDepthMeters);

    // 5. Depth-hold feedback loop
    //    Runs at a fixed interval (default 5 Hz).  A pressure deadband
    //    prevents the motor from oscillating when the float is near the
    //    target depth — the MS5837 noise floor is ~0.2 mbar, so the
    //    deadband must be comfortably above that.
    unsigned long startTime = millis();
    int minSteps = params.targetDepthSteps - params.holdRangeSteps;
    int maxSteps = params.targetDepthSteps + params.holdRangeSteps;

    while ((millis() - startTime) < params.holdDurationMs) {
        // Use cached pressure from sensorTask (Core 0) instead of calling
        // _sensor->read() here — concurrent I2C access from two cores can
        // corrupt data or lock the bus.
        float error    = _sensor->getPressure() - targetPressure;
        int   stepCount = _stepper->getStepCount();

        if (error > DEPTHHOLD_DEADBAND_MBAR && stepCount > minSteps) {
            // Too deep — retract (ascend) if not at top limit
            if (!_stepper->isAtLimit(false)) {
                _stepper->stepBatch(MOTOR_STEP_BATCH, false);
            }
        } else if (error < -DEPTHHOLD_DEADBAND_MBAR && stepCount < maxSteps) {
            // Too shallow — extend (descend) if not at bottom limit
            if (!_stepper->isAtLimit(true)) {
                _stepper->stepBatch(MOTOR_STEP_BATCH, true);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(DEPTHHOLD_LOOP_INTERVAL_MS));
    }

    // 6. Restore speed and ascend home
    _stepper->setSpeed(savedSpeed);
    _stepper->runToLimit(false);
}
