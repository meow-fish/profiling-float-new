// ============================================================
// Wokwi Stepper LED Debug
// ============================================================
// Drives GPIO4 (STEP) and GPIO5 (DIR) slowly so you can
// visually confirm output with LEDs or a stepper driver.
// ============================================================

#include <Arduino.h>
#include "config.h"

// Slow speed so LEDs visibly blink (200ms per pulse)
#define DEBUG_STEP_DELAY_MS 200

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_STEP, OUTPUT);
    pinMode(PIN_DIR, OUTPUT);
    pinMode(PIN_LIMIT_BOTTOM, INPUT_PULLUP);
    pinMode(PIN_LIMIT_TOP, INPUT_PULLUP);

    Serial.println("=== Stepper LED Debug ===");
    Serial.printf("STEP pin: GPIO%d\n", PIN_STEP);
    Serial.printf("DIR pin:  GPIO%d\n", PIN_DIR);
    Serial.println("Watch the LEDs blink!");
}

void loop() {
    // --- CW: DIR LOW, pulse STEP 10 times ---
    Serial.println("[CW] DIR=LOW, pulsing STEP 10x...");
    digitalWrite(PIN_DIR, LOW);
    for (int i = 0; i < 10; i++) {
        digitalWrite(PIN_STEP, HIGH);
        delay(DEBUG_STEP_DELAY_MS);
        digitalWrite(PIN_STEP, LOW);
        delay(DEBUG_STEP_DELAY_MS);
        Serial.printf("  pulse %d\n", i + 1);
    }

    delay(1000);

    // --- CCW: DIR HIGH, pulse STEP 10 times ---
    Serial.println("[CCW] DIR=HIGH, pulsing STEP 10x...");
    digitalWrite(PIN_DIR, HIGH);
    for (int i = 0; i < 10; i++) {
        digitalWrite(PIN_STEP, HIGH);
        delay(DEBUG_STEP_DELAY_MS);
        digitalWrite(PIN_STEP, LOW);
        delay(DEBUG_STEP_DELAY_MS);
        Serial.printf("  pulse %d\n", i + 1);
    }

    delay(1000);

    // --- Check limit switches ---
    Serial.printf("[LIMIT] Bottom(GPIO%d)=%s  Top(GPIO%d)=%s\n",
                  PIN_LIMIT_BOTTOM,
                  digitalRead(PIN_LIMIT_BOTTOM) == LOW ? "LOW" : "HIGH",
                  PIN_LIMIT_TOP,
                  digitalRead(PIN_LIMIT_TOP) == LOW ? "LOW" : "HIGH");

    delay(2000);
    Serial.println("--- Repeating ---\n");
}
