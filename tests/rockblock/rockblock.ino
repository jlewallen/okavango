#include <IridiumSBD.h>

static const int ledPin = 13;

void setup()
{
    int signalQuality = -1;

    Serial.begin(115200);

    while (!Serial);

    pinMode(ledPin, OUTPUT);

    IridiumSBD isbd(Serial1, 9);
    Serial1.begin(19200);

    isbd.attachConsole(Serial);
    isbd.setPowerProfile(1);
    isbd.begin();

    int err = isbd.getSignalQuality(signalQuality);
    if (err != 0)
    {
        Serial.print("SignalQuality failed: error ");
        Serial.println(err);
        return;
    }

    Serial.print("Signal quality is ");
    Serial.println(signalQuality);

    err = isbd.sendSBDText("1234567890123456789012345678901234567890123456789012345678901234567890");
    if (err != 0)
    {
        Serial.print("sendSBDText failed: error ");
        Serial.println(err);
        return;
    }

    Serial.println("Hey, it worked!");
    Serial.print("Messages left: ");
    Serial.println(isbd.getWaitingMessageCount());
}

void loop()
{
    digitalWrite(ledPin, HIGH);
}

bool ISBDCallback()
{
    digitalWrite(ledPin, (millis() / 1000) % 2 == 1 ? HIGH : LOW);
    return true;
}

// vim: set ft=cpp:
