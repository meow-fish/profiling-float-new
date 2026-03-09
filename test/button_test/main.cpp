// Standalone test: button and LED test.
// Lights an LED when the corresponding button is pressed.

#include <Arduino.h>

#define BUTTON_PIN_1  35
#define BUTTON_PIN_2  34
#define LED_PIN_1     4
#define LED_PIN_2     13

void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN_1, INPUT_PULLUP);
    pinMode(BUTTON_PIN_2, INPUT_PULLUP);
    pinMode(LED_PIN_1, OUTPUT);
    pinMode(LED_PIN_2, OUTPUT);
}

void loop() {
    int state1 = digitalRead(BUTTON_PIN_1);
    int state2 = digitalRead(BUTTON_PIN_2);

    digitalWrite(LED_PIN_1, state1 == LOW ? HIGH : LOW);
    digitalWrite(LED_PIN_2, state2 == LOW ? HIGH : LOW);

    Serial.print("B1: "); Serial.print(state1);
    Serial.print(" | B2: "); Serial.println(state2);

    delay(500);
}
