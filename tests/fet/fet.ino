void setup() {
    Serial.begin(115200);

    pinMode(6, OUTPUT);
    digitalWrite(6, LOW);

    while (!Serial) {
    }

    uint8_t state = LOW;

    while (true) {
        digitalWrite(6, state);

        delay(10);

        if (Serial.available() > 0) {
            while (Serial.available()) {
                Serial.read();
            }
            state = state == LOW ? HIGH : LOW;
            Serial.print("New: ");
            Serial.println(state);
        }
    }
}

void loop() {
}
