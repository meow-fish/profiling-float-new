# Linear Actuator Migration Guide

This document describes the code changes required to replace the **stepper motor + A4988 driver** with a **DC linear actuator controlled via an H-bridge** (e.g. L298N or BTS7960).

---

## Hardware Change Summary

| Aspect | Current (Stepper) | New (Linear Actuator) |
|--------|-------------------|----------------------|
| Driver | A4988 step/dir driver | H-bridge (L298N / BTS7960) |
| Control signals | STEP pulse + DIR pin | IN1 + IN2 direction pins + EN (PWM speed) |
| Motion | Discrete step pulses | Continuous DC drive |
| Speed control | Microsecond pulse timing | PWM duty cycle (0–255) |
| Position tracking | Step counter | Limit switches only (potentiometer feedback optional) |

---

## Files That Must Change

### 1. Motor Driver — `lib/Stepper/FloatStepper.h` and `FloatStepper.cpp`

**Replace entirely** with a new linear actuator driver (e.g. `lib/Actuator/LinearActuator.h/.cpp`).

The entire HAL is stepper-specific: LEDC pulse generation, step counting, microsecond timing. The new driver needs:

- **Init**: configure IN1, IN2, EN (PWM), and limit switch pins
- **extend() / retract() / stop()**: basic directional control via H-bridge
- **runToLimit(direction)**: drive continuously until a limit switch triggers (same concept, different implementation — set direction pins instead of starting LEDC pulses)
- **setSpeed(dutyCycle)**: PWM duty cycle 0–255 on the EN pin (replaces microsecond step delay)
- **isAtLimit(bottom)**: unchanged — same limit switch logic
- **Remove**: `stepBatch()`, `getStepCount()`, `resetStepCount()`, `pulseStep()`, all LEDC code

### 2. Configuration — `include/config.h`

**Lines 8–10** — Replace pin definitions:
```
PIN_STEP / PIN_DIR  →  PIN_ACTUATOR_IN1 / PIN_ACTUATOR_IN2 / PIN_ACTUATOR_EN
```

**Lines 31–33** — Replace motor parameters:
```
MOTOR_SPEED_DEFAULT (600 µs)   →  ACTUATOR_SPEED_DEFAULT (PWM duty, e.g. 200)
MOTOR_SPEED_HOLD (5000 µs)     →  ACTUATOR_SPEED_HOLD (PWM duty, e.g. 80)
MOTOR_STEP_BATCH (10 steps)    →  ACTUATOR_PULSE_MS (timed burst, e.g. 100 ms)
```

**Lines 56–57** — Rework or remove step-based constants:
```
DEPTHHOLD_RANGE_STEPS (300)    →  remove (no step counter)
DEPTHHOLD_MIDPOINT (3300)      →  remove (no step-based positioning)
```

### 3. Dive Controller — `lib/DepthControl/DepthControl.h` and `DepthControl.cpp`

**Header changes:**
- Forward declaration: `FloatStepper` → `LinearActuator`
- `DiveParams`: remove `targetDepthSteps` and `holdRangeSteps` (step-count fields)
- Member: `FloatStepper* _stepper` → `LinearActuator* _actuator`

**Simple dive** (lines 47–56) — straightforward swap:
```
_stepper->runToLimit(true)   →  _actuator->runToLimit(true)
_stepper->runToLimit(false)  →  _actuator->runToLimit(false)
```

**Depth-hold dive** (lines 60–112) — **most significant change:**
- Remove step-count-based descent to midpoint (lines 66–69) — instead, drive until target pressure is reached
- Remove step counter bounds checking (`minSteps`, `maxSteps`, `getStepCount()`)
- Feedback loop corrections change from `stepBatch(10, dir)` to **timed bursts**: drive actuator for N milliseconds, then re-check pressure
- Keep pressure deadband logic (5 mbar) — works regardless of actuator type
- Keep limit switch safety checks

### 4. Main Firmware — `src/main.cpp`

Minimal changes:
- **Line 8**: `#include "FloatStepper.h"` → `#include "LinearActuator.h"`
- **Line 15**: `FloatStepper stepper` → `LinearActuator actuator`
- **Line 103**: `stepper.init(PIN_STEP, PIN_DIR, ...)` → `actuator.init(PIN_ACTUATOR_IN1, PIN_ACTUATOR_IN2, PIN_ACTUATOR_EN, ...)`
- **Line 107**: `depthController.init(stepper, sensor)` → `depthController.init(actuator, sensor)`

### 5. Web Server — `lib/Network/FloatWebServer.h` and `FloatWebServer.cpp`

- **Header line 15**: Remove `class FloatStepper` forward declaration (unused here)
- **HTML strings**: Change "Stepper sequence started" → "Dive sequence started" (cosmetic, lines 95 and 116–119)
- `/status` JSON: no change needed (doesn't expose step count)

### 6. Debug Firmware — `test/wokwi_stepper_debug/main.cpp`

Replace stepper-specific commands:
```
f [N]  (step N pulses CW)    →  e [N]  (extend for N ms, default 500)
b [N]  (step N pulses CCW)   →  r [N]  (retract for N ms, default 500)
v N    (speed in µs)          →  v N    (PWM duty 0–255)
z      (reset step counter)  →  remove (no step counter)
```

Keep: `r` (run to bottom), `h` (home to top), `s` (status), `?` (help).
Status display: remove step count, show actuator state (extending/retracting/stopped).

### 7. Auto Test Firmware — `test/wokwi_stepper_auto/main.cpp`

Replace STEP/DIR pulse toggling with extend/retract cycling using the new driver.

### 8. Documentation — `TESTING_GUIDE.md`, `README.md`

- A4988 wiring instructions → H-bridge wiring instructions
- Step/direction terminology → extend/retract terminology
- Pin assignment tables → updated GPIO mapping
- Current limit calibration (A4988 potentiometer) → H-bridge specific setup

---

## Key Design Decision: Depth-Hold Without Step Counting

The stepper's step counter provided a secondary position reference. With a DC linear actuator and no position encoder, the depth-hold controller must:

1. **Use timed bursts** instead of discrete steps — e.g., drive for 100 ms then check pressure
2. **Rely entirely on the pressure sensor** for position feedback
3. **Use limit switches only as safety stops**, not as position calibration points

This simplifies the control loop in some ways — the pressure sensor becomes the sole feedback source, eliminating the dual step-count/pressure loop. However, it requires careful tuning of the burst duration and deadband to avoid oscillation.

---

## Optional Future Enhancement

If the linear actuator has a **built-in potentiometer** (3rd feedback wire), an `analogRead()` on a spare GPIO can provide continuous position feedback, restoring the ability to:
- Track intermediate position
- Set position-based descent targets
- Implement position + pressure dual-loop control (similar to the current step-count approach)
