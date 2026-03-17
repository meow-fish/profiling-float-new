# Linear Actuator Migration Guide

This document describes the code changes required to replace the **stepper motor + A4988 driver** with a **DC linear actuator controlled via a TB6612FNG dual H-bridge driver**.

---

## TB6612FNG Motor Driver

The [TB6612FNG](https://www.sparkfun.com/products/14450) is a dual-channel MOSFET H-bridge driver. We use one channel (A) to drive the linear actuator.

### Specifications

| Parameter | Value |
|-----------|-------|
| Motor supply voltage (VM) | 2.7 V – 13.5 V |
| Logic supply voltage (VCC) | 2.7 V – 5.5 V |
| Max output current (per channel) | 1.2 A continuous, 3.2 A peak |
| Efficiency | 91 – 95 % |
| Control method | MOSFET H-bridge |

### TB6612FNG Pinout (Channel A only)

| TB6612FNG Pin | Function | Connect to |
|---------------|----------|------------|
| **VM** | Motor power supply | Battery positive (up to 13.5 V) |
| **VCC** | Logic power | ESP32 3.3 V |
| **GND** | Ground | Common ground (battery + ESP32) |
| **STBY** | Standby (active LOW = sleep) | Tie to VCC or a GPIO (must be HIGH to operate) |
| **AIN1** | Direction input 1 | ESP32 GPIO |
| **AIN2** | Direction input 2 | ESP32 GPIO |
| **PWMA** | Speed control (PWM) | ESP32 GPIO (LEDC PWM output) |
| **A01** | Motor output 1 | Linear actuator wire 1 |
| **A02** | Motor output 2 | Linear actuator wire 2 |

### Control Truth Table

| AIN1 | AIN2 | PWMA | Actuator Action |
|------|------|------|-----------------|
| HIGH | LOW | PWM | **Extend** (or retract — depends on wiring) |
| LOW | HIGH | PWM | **Retract** (or extend — depends on wiring) |
| LOW | LOW | — | **Coast** (motor freewheels) |
| HIGH | HIGH | — | **Brake** (motor shorts, active stop) |
| — | — | LOW | **Stop** (regardless of AIN1/AIN2) |

### Suggested ESP32 GPIO Mapping

| Signal | GPIO | Notes |
|--------|------|-------|
| AIN1 | GPIO4 (was PIN_STEP) | Reuses existing PCB trace if possible |
| AIN2 | GPIO5 (was PIN_DIR) | Reuses existing PCB trace if possible |
| PWMA | GPIO16 | Any PWM-capable GPIO; uses ESP32 LEDC channel |
| STBY | Tied to VCC | Or GPIO if sleep mode is desired |
| Limit Bottom | GPIO13 | Unchanged |
| Limit Top | GPIO14 | Unchanged |

### Wiring Notes

- **STBY must be HIGH** for the driver to operate. Tie directly to VCC for simplicity, or connect to a GPIO if you want software-controlled sleep mode.
- **VM and VCC** need separate decoupling capacitors (100 µF electrolytic on VM, 100 nF ceramic on VCC recommended).
- The motor direction (which wire = extend vs retract) depends on actuator wiring to A01/A02 — swap the wires if the direction is reversed.
- Channel B (BIN1/BIN2/PWMB/B01/B02) is unused and can be left unconnected.

---

## Hardware Change Summary

| Aspect | Current (Stepper + A4988) | New (Linear Actuator + TB6612FNG) |
|--------|---------------------------|----------------------------------|
| Driver IC | A4988 stepper driver | TB6612FNG MOSFET H-bridge |
| Control signals | STEP pulse + DIR pin (2 GPIOs) | AIN1 + AIN2 + PWMA (3 GPIOs) |
| Motion | Discrete step pulses | Continuous DC drive |
| Speed control | Microsecond pulse timing | PWM duty cycle (0–255) on PWMA pin |
| Position tracking | Step counter | Limit switches only (potentiometer feedback optional) |
| Standby/enable | Not applicable | STBY pin (tie HIGH or use GPIO) |
| Max current | 2 A (A4988) | 1.2 A continuous / 3.2 A peak (TB6612FNG) |

---

## Files That Must Change

### 1. Motor Driver — `lib/Stepper/FloatStepper.h` and `FloatStepper.cpp`

**Replace entirely** with a new linear actuator driver (e.g. `lib/Actuator/LinearActuator.h/.cpp`).

The entire HAL is stepper-specific: LEDC pulse generation, step counting, microsecond timing. The new driver needs:

- **Init**: configure AIN1, AIN2, PWMA, and limit switch pins
- **extend() / retract() / stop()**: set AIN1/AIN2 per the TB6612FNG truth table, control speed via PWMA
- **runToLimit(direction)**: drive continuously until a limit switch triggers (same concept, different implementation — set AIN1/AIN2 direction + PWMA duty instead of LEDC step pulses)
- **setSpeed(dutyCycle)**: PWM duty cycle 0–255 on the PWMA pin (replaces microsecond step delay)
- **isAtLimit(bottom)**: unchanged — same limit switch logic
- **Remove**: `stepBatch()`, `getStepCount()`, `resetStepCount()`, `pulseStep()`, all LEDC code

### 2. Configuration — `include/config.h`

**Lines 8–10** — Replace pin definitions:
```
PIN_STEP  →  PIN_AIN1   (TB6612FNG AIN1)
PIN_DIR   →  PIN_AIN2   (TB6612FNG AIN2)
(new)        PIN_PWMA   (TB6612FNG PWMA — speed PWM)
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
- **Line 103**: `stepper.init(PIN_STEP, PIN_DIR, ...)` → `actuator.init(PIN_AIN1, PIN_AIN2, PIN_PWMA, ...)`
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

- A4988 wiring instructions → TB6612FNG wiring (AIN1/AIN2/PWMA/STBY, VM/VCC decoupling)
- Step/direction terminology → extend/retract terminology
- Pin assignment tables → updated GPIO mapping (3 control pins instead of 2)
- Current limit calibration (A4988 potentiometer) → remove (TB6612FNG has built-in current limiting)
- Add note about STBY pin (must be HIGH for operation)

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
