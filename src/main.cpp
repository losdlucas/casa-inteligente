#include <Arduino.h>

#define BUZZER 21

void setup() {
    Serial.begin(115200);
    pinMode(BUZZER, OUTPUT);

    Serial.println("TESTE BUZZER GPIO 21");

    tone(BUZZER, 1000);
}

void loop() {
}