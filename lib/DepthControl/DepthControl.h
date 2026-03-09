#ifndef DEPTH_CONTROL_H
#define DEPTH_CONTROL_H

#include <Arduino.h>
#include "config.h"

// Forward declarations
class FloatStepper;
class PressureSensor;

/// Dive mode selector.
enum class DiveMode {
    SIMPLE,      // Descend to limit switch, dwell, ascend
    DEPTH_HOLD   // Descend to target, hold depth using pressure feedback
};

/// Parameters for a dive operation.
struct DiveParams {
    DiveMode      mode;
    int           targetDepthSteps;     // Steps to descend before holding
    float         targetDepthMeters;    // Physical depth for pressure target
    unsigned long holdDurationMs;       // How long to maintain depth
    unsigned long dwellAtBottomMs;      // Pause at bottom (SIMPLE mode)
    int           holdRangeSteps;       // +/- tolerance around target position
};

class DepthController {
public:
    /// Store references to stepper and sensor. Call once.
    void init(FloatStepper& stepper, PressureSensor& sensor);

    /// Execute a dive with the given parameters. Blocks until complete.
    void startDive(const DiveParams& params);

    /// Build default params for a simple dive (limit-to-limit).
    static DiveParams simpleParams();

    /// Build default params for a depth-hold dive.
    static DiveParams depthHoldParams(float depthMeters);

private:
    FloatStepper*   _stepper;
    PressureSensor* _sensor;

    void runSimpleDive(const DiveParams& params);
    void runDepthHold(const DiveParams& params);
};

#endif // DEPTH_CONTROL_H
