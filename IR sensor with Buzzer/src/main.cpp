#include <Arduino.h>

#define IR_PIN 4       // D2 = GPIO4
#define BUZZER_PIN 14  // D5 = GPIO14

void setup() {
    pinMode(IR_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    digitalWrite(BUZZER_PIN, LOW);

    Serial.begin(115200);
}

void loop() {

    int irState = digitalRead(IR_PIN);

    if (irState == LOW) {
        // Object detected
        digitalWrite(BUZZER_PIN, HIGH);
        Serial.println("Object Detected - Buzzer ON");
    }
    else {
        // No object
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println("No Object - Buzzer OFF");
    }

    delay(100);
}