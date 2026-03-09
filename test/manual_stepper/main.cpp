// Standalone test: manual stepper motor control via two buttons.
// Button 1 (GPIO13): run motor while held.
// Button 2 (GPIO14): toggle direction.

#include <Arduino.h>
#include "config.h"

bool isClockwise = true;

void setup() {
    Serial.begin(115200);

    pinMode(PIN_DIR, OUTPUT);
    pinMode(PIN_STEP, OUTPUT);
    pinMode(PIN_LIMIT_BOTTOM, INPUT_PULLUP);
    pinMode(PIN_LIMIT_TOP, INPUT_PULLUP);

    Serial.println("Manual stepper test ready");
    Serial.println("Button 1 (GPIO13): Run motor");
    Serial.println("Button 2 (GPIO14): Toggle direction");
}

void loop() {
    // Toggle direction on button 2 press
    if (digitalRead(PIN_LIMIT_TOP) == LOW) {
        isClockwise = !isClockwise;
        digitalWrite(PIN_DIR, isClockwise ? LOW : HIGH);
        delay(300); // debounce
    }

    // Step while button 1 is held
    if (digitalRead(PIN_LIMIT_BOTTOM) == LOW) {
        digitalWrite(PIN_STEP, HIGH);
        delayMicroseconds(MOTOR_SPEED_DEFAULT);
        digitalWrite(PIN_STEP, LOW);
        delayMicroseconds(MOTOR_SPEED_DEFAULT);
    }

    delay(1);
}
