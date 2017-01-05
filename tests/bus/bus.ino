#include <Arduino.h>
#include <Wire.h>

#define LED_PIN                                               13

void blink(uint8_t pin, uint8_t times) {
    while (times--) {
        digitalWrite(pin, HIGH);
        delay(500);
        digitalWrite(pin, LOW);
        delay(500);
    }
}

typedef struct i2c_device_t {
    uint8_t address;
    i2c_device_t *next;
} i2c_device_t;

size_t i2c_devices_number(i2c_device_t *devices) {
    size_t number = 0;
    while (devices != nullptr) {
        number++;
        devices = devices->next;
    }
    return number;
}

bool i2c_devices_exists(i2c_device_t *head, uint8_t address) {
    size_t number = 0;
    while (head != nullptr) {
        if (head->address == address) {
            return true;
        }
        head = head->next;
    }
    return false;
}

void i2c_devices_free(i2c_device_t *head) {
    while (head != nullptr) {
        i2c_device_t *temp = head->next;
        free(head);
        head = temp;
    }
}

i2c_device_t *i2c_devices_scan() {
    i2c_device_t *head = nullptr;
    i2c_device_t *tail = nullptr;

    Serial.println("i2c: scanning...");

    Wire.begin();

    for (uint8_t i = 1; i < 128; ++i) {
        Wire.beginTransmission(i);
        Wire.write("HELLO");
        if (Wire.endTransmission() == 0) {
            i2c_device_t *n = (i2c_device_t *)malloc(sizeof(i2c_device_t));
            n->address = i;
            n->next = nullptr;
            if (head == nullptr) {
                head = n;
                tail = n;
            }
            else {
                tail->next = n;
            }
            Serial.print("i2c: slave ");
            Serial.println(i);
        }
    }

    return head;
}

void onRequest() {
    Serial.print("i2c: incoming: ");
    while (Wire.available()) {
        char c = Wire.read();
        Serial.print(c);
    }
    Serial.println();
}

void onReceive(int bytes) {
    Serial.print("i2c: ");
    while (Wire.available()) {
        char c = Wire.read();
        Serial.print(c);
    }
    Serial.println();
}

void setup() {
    pinMode(LED_PIN, OUTPUT);

    Serial.begin(115200);

    while (!Serial);

    i2c_device_t *devices = i2c_devices_scan();
    bool master = i2c_devices_exists(devices, 8);
    i2c_devices_free(devices);

    if (master) {
        Serial.println("i2c: master");

        blink(LED_PIN, 1);
    }
    else {
        Serial.println("i2c: slave");

        Wire.begin(8);
        Wire.onReceive(onReceive);
        Wire.onRequest(onRequest);
    }
}

void loop() {
    delay(10);
}
