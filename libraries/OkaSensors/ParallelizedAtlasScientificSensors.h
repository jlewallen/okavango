#ifndef PARALLIZED_ATLAS_SCIENTIFICS_SENSORS_H
#define PARALLIZED_ATLAS_SCIENTIFICS_SENSORS_H

#include <Arduino.h>
#include "SensorBoard.h"
#include "SerialPortExpander.h"
#include "NonBlockingSerial.h"

enum class ParallelizedAtlasScientificSensorsState {
    Start = 0,
    Factory,
    DisableContinuousReading,
    ConfigureResponse,
    Status0,
    Status1,
    LedsOn,
    Configure,
    Waiting,
    Read0,
    LedsOnBeforeRead,
    Read1,
    LedsOff,
    Sleeping,
    Done
};

class ParallelizedAtlasScientificSensors : public NonBlockingSerialProtocol, public SensorBoard {
private:
    Stream *debug;
    ParallelizedAtlasScientificSensorsState state = ParallelizedAtlasScientificSensorsState::Start;
    SerialPortExpander *serialPortExpander = nullptr;
    float *values = nullptr;
    uint32_t lastTransisitonAt = 0;
    uint8_t numberOfRead0s = 0;
    uint8_t maximumNumberOfRead0s = 0;
    uint8_t maximumNumberOfValues = 0;
    uint8_t numberOfValues = 0;
    uint8_t portNumber = 0;
    uint8_t hasPortFailed[8] = { false };
    uint32_t runs = 0;
    bool disableSleep = false;

public:
    ParallelizedAtlasScientificSensors(Stream *debug, SerialPortExpander *serialPortExpander, bool disableSleep, uint8_t maximumNumberOfValues, uint8_t maximumNumberOfRead0s = 20);

    virtual bool tick() override;

    virtual const float *getValues() override {
        return values;
    }

    virtual uint8_t getNumberOfValues() override {
        return numberOfValues;
    }

    virtual bool isDone() override {
        return state == ParallelizedAtlasScientificSensorsState::Done;
    }

    virtual bool isStartingFakeReads() override {
        return portNumber == 0 && state == ParallelizedAtlasScientificSensorsState::Read0;
    }

    virtual void takeReading() override;
    virtual void start() override;

protected:
    void transition(ParallelizedAtlasScientificSensorsState newState);
    NonBlockingHandleStatus handle(String reply, bool forceTransition = false);
    virtual NonBlockingHandleStatus handle(String reply) override {
        return handle(reply, false);
    }
};

#endif
