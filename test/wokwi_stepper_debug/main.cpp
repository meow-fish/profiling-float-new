// ============================================================
// Wokwi Stepper Motor Debug Firmware
// ============================================================
// Interactive serial console for debugging the FloatStepper
// module. Use with the Wokwi simulator or real hardware.
//
// LIMIT SWITCH BEHAVIOR IN WOKWI:
//   Real hardware uses Normally-Closed (NC) limit switches:
//     - Closed (not at limit) -> GPIO reads LOW
//     - Open  (at limit)      -> GPIO reads HIGH (pullup)
//
//   In Wokwi, pushbuttons simulate this:
//     - Button PRESSED  -> GPIO LOW  -> "not at limit"
//     - Button RELEASED -> GPIO HIGH -> "at limit"
//
//   To use runToLimit / home commands:
//     1. Press and HOLD the target limit-switch button
//     2. Send the run / home command
//     3. RELEASE the button to simulate hitting the limit
// ============================================================

#include <Arduino.h>
#include "FloatStepper.h"
#include "config.h"

FloatStepper stepper;

// ── Helpers ─────────────────────────────────────────────────

void printStatus() {
    Serial.println("=== Stepper Status ===");
    Serial.printf("  Step count : %d\n", stepper.getStepCount());
    Serial.printf("  Speed      : %d us/pulse\n", stepper.getSpeed());
    Serial.printf("  Bottom lim : %s\n",
                  stepper.isAtLimit(true)  ? "AT LIMIT" : "clear");
    Serial.printf("  Top limit  : %s\n",
                  stepper.isAtLimit(false) ? "AT LIMIT" : "clear");
    Serial.println("======================");
}

void printHelp() {
    Serial.println();
    Serial.println("--- Stepper Debug Commands ---");
    Serial.println("  f [N]   Step N pulses CW  / descend  (default 100)");
    Serial.println("  b [N]   Step N pulses CCW / ascend   (default 100)");
    Serial.println("  r       Run CW until bottom limit");
    Serial.println("  h       Home: run CCW until top limit");
    Serial.println("  s       Print status");
    Serial.println("  v N     Set speed to N microseconds");
    Serial.println("  z       Reset step counter to 0");
    Serial.println("  ?       Show this help");
    Serial.println("------------------------------");
}

// ── Setup ───────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    stepper.init(PIN_STEP, PIN_DIR, PIN_LIMIT_BOTTOM, PIN_LIMIT_TOP);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  Stepper Motor Debug Console");
    Serial.println("========================================");
    Serial.printf("  STEP pin      : GPIO%d\n", PIN_STEP);
    Serial.printf("  DIR pin       : GPIO%d\n", PIN_DIR);
    Serial.printf("  Bottom limit  : GPIO%d\n", PIN_LIMIT_BOTTOM);
    Serial.printf("  Top limit     : GPIO%d\n", PIN_LIMIT_TOP);
    Serial.printf("  Default speed : %d us\n", MOTOR_SPEED_DEFAULT);
    Serial.println("========================================");
    Serial.println();
    Serial.println("NOTE (Wokwi): HOLD a limit-switch button");
    Serial.println("  to simulate 'not at limit', RELEASE to");
    Serial.println("  simulate hitting the limit.");

    printHelp();
    printStatus();
    Serial.print("> ");
}

// ── Main loop ───────────────────────────────────────────────

void loop() {
    // ── Process serial commands ──────────────────────────────
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() == 0) {
            Serial.print("> ");
            return;
        }

        char cmd   = input.charAt(0);
        int  value = (input.length() > 1) ? input.substring(1).toInt() : 0;

        switch (cmd) {
        case 'f': case 'F': {
            int steps = (value > 0) ? value : 100;
            Serial.printf("[STEP] %d pulses CW (descend)...\n", steps);
            unsigned long t0 = millis();
            stepper.stepBatch(steps, true);
            Serial.printf("[DONE] count=%d  elapsed=%lu ms\n",
                          stepper.getStepCount(), millis() - t0);
            break;
        }
        case 'b': case 'B': {
            int steps = (value > 0) ? value : 100;
            Serial.printf("[STEP] %d pulses CCW (ascend)...\n", steps);
            unsigned long t0 = millis();
            stepper.stepBatch(steps, false);
            Serial.printf("[DONE] count=%d  elapsed=%lu ms\n",
                          stepper.getStepCount(), millis() - t0);
            break;
        }
        case 'r': case 'R':
            if (stepper.isAtLimit(true)) {
                Serial.println("[WARN] Bottom limit already triggered!");
                Serial.println("       Hold the bottom-limit button, then retry.");
            } else {
                Serial.println("[RUN]  Running CW to bottom limit...");
                stepper.runToLimit(true);
                Serial.printf("[DONE] Bottom reached. count=%d\n",
                              stepper.getStepCount());
            }
            break;

        case 'h': case 'H':
            if (stepper.isAtLimit(false)) {
                Serial.println("[WARN] Top limit already triggered!");
                Serial.println("       Hold the top-limit button, then retry.");
            } else {
                Serial.println("[RUN]  Homing CCW to top limit...");
                stepper.runToLimit(false);
                Serial.printf("[DONE] Home. count=%d (reset)\n",
                              stepper.getStepCount());
            }
            break;

        case 's': case 'S':
            printStatus();
            break;

        case 'v': case 'V':
            if (value > 0) {
                stepper.setSpeed(value);
                Serial.printf("[SET]  Speed = %d us/pulse\n", value);
            } else {
                Serial.printf("[INFO] Current speed = %d us.  Usage: v <us>\n",
                              stepper.getSpeed());
            }
            break;

        case 'z': case 'Z':
            stepper.resetStepCount();
            Serial.println("[SET]  Step counter reset to 0");
            break;

        case '?':
            printHelp();
            break;

        default:
            Serial.printf("[ERR]  Unknown: '%c'.  Type '?' for help.\n", cmd);
            break;
        }
        Serial.print("> ");
    }

    // ── Report limit-switch state changes ────────────────────
    static bool prevBottom = false, prevTop = false;
    bool curBottom = stepper.isAtLimit(true);
    bool curTop    = stepper.isAtLimit(false);

    if (curBottom != prevBottom) {
        Serial.printf("\n[LIMIT] Bottom: %s\n> ",
                      curBottom ? "TRIGGERED" : "released");
        prevBottom = curBottom;
    }
    if (curTop != prevTop) {
        Serial.printf("\n[LIMIT] Top: %s\n> ",
                      curTop ? "TRIGGERED" : "released");
        prevTop = curTop;
    }

    delay(10);
}
