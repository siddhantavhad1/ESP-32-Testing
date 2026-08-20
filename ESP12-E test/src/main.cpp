#include <Arduino.h>
#include <Servo.h>

Servo myServo;

#define SERVO_PIN 2

void setup() {
    Serial.begin(115200);

    myServo.attach(SERVO_PIN);

    Serial.println("Servo test started");
}

void loop() {
    myServo.write(0);
    delay(1000);

    myServo.write(90);
    delay(1000);

    myServo.write(180);
    delay(1000);
}