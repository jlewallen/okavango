#include <SPI.h>
#include <SD.h>

#include "Platforms.h"
#include "core.h"
#include "SerialPortExpander.h"
#include "AtlasSensorBoard.h"
#include "SimpleBuffer.h"

String phScript[] = {
    "c,0",
    "c,0",
    "L,1",
    "R",
    "STATUS"
};

String doScript[] = {
    "c,0",
    "c,0",
    "L,1",
    "R",
    "STATUS"
};

String orpScript[] = {
    "c,0",
    "c,0",
    "L,1",
    "R",
    "STATUS"
};

String ecScript[] = {
    "c,0",
    "c,0",
    "L,1",
    "R",
    "STATUS"
};

ConductivityConfig conductivityConfig = ConductivityConfig::OnExpanderPort4;


typedef enum ScriptRunnerState {
    WaitingOnReply,
    DeviceIdle
} ScriptRunnerState;

class ScriptRunner : public NonBlockingSerialProtocol {
private:
    ScriptRunnerState state;
    uint8_t position;
    String *commands;
    size_t length;

public:
    ScriptRunner() : commands(NULL), position(0), length(0) {
    }

    template<size_t N>
    void setScript(String (&newCommands)[N]) {
        commands = newCommands;
        position = 0;
        length = N;
    }

    const char *currentCommand() {
        if (length == 0 || position >= length) {
            return "";
        }
        return commands[position].c_str();
    }

    void startOver() {
        position = 0;
    }

    void send() {
        send(currentCommand());

        if (position < length) {
            position++;
        }
    }

    void send(const char *command) {
        sendCommand(command);
        state = ScriptRunnerState::WaitingOnReply;
    }

    bool isIdle() {
        return state == ScriptRunnerState::DeviceIdle;
    }

    bool tick() {
        if (state == ScriptRunnerState::WaitingOnReply) {
            if (NonBlockingSerialProtocol::tick()) {
                return true;
            }
        }

        return false;
    }

    bool isAtEnd() {
        return sizeof(commands) / sizeof(String);
    }

    bool handle(String reply) {
        Serial.println(reply);
        if (reply.indexOf("*") == 0) {
            state = ScriptRunnerState::DeviceIdle;
            return true;
        }
        return true;
    }
};

typedef enum ReplState {
    WaitingOnDevice,
    ShowPrompt,
    Command
} ReplState;

class Repl {
private:
    ReplState state = ReplState::Command;
    SimpleBuffer buffer;
    SerialPortExpander *portExpander;
    ScriptRunner *scriptRunner;

public:
    Repl(SerialPortExpander *portExpander, ScriptRunner *scriptRunner) :
        portExpander(portExpander), scriptRunner(scriptRunner) {
    }

    bool tick() {
        switch (state) {
        case ReplState::WaitingOnDevice: {
            if (!scriptRunner->tick()) {
                state = ReplState::ShowPrompt;
            }

            break;
        }
        case ReplState::ShowPrompt: {
            showPrompt();
            state = ReplState::Command;
            break;
        }
        case ReplState::Command: {
            if (Serial.available()) {
                int16_t c = (int16_t)Serial.read();
                if (c < 0) {
                    return false;
                }

                char newChar = (char)c;

                Serial.print(newChar);

                if (newChar == '\r') {
                    Serial.println();

                    state = ReplState::ShowPrompt;

                    handle(buffer.c_str());
                    buffer.clear();
                }
                else {
                    buffer.append(newChar);
                }
            }
            break;
        }
        }
        return false;
    }

    void showPrompt() {
        Serial.print(scriptRunner->currentCommand());
        Serial.print("> ");
    }
    
    void handle(String command) {
        if (command.startsWith("p ")) {
            uint8_t number = command.substring(2).toInt();
            portExpander->select(number);
            if (number < 3 || conductivityConfig != ConductivityConfig::OnSerial2) {
                scriptRunner->setSerial(&Serial1);
            }
            else if (number == 4) {
                scriptRunner->setSerial(&Serial2);
            }
            return;
        }
        else if (command.startsWith("ph")) {
            Serial.println("Ph Mode");
            scriptRunner->setScript(phScript);
            return;
        }
        else if (command.startsWith("do")) {
            Serial.println("Do Mode");
            scriptRunner->setScript(doScript);
            return;
        }
        else if (command.startsWith("orp")) {
            Serial.println("Orp Mode");
            scriptRunner->setScript(orpScript);
            return;
        }
        else if (command.startsWith("ec")) {
            Serial.println("Ec Mode");
            scriptRunner->setScript(ecScript);
            return;
        }
        else if (command == "") {
            scriptRunner->send();
            state = ReplState::WaitingOnDevice;
        }
        else {
            scriptRunner->send(command.c_str());
            state = ReplState::WaitingOnDevice;
        }
    }
};

CorePlatform corePlatform;

void setup() {
    Serial.begin(115200);

    #ifdef WAIT_FOR_SERIAL
    while (!Serial) {
        delay(100);
        if (millis() > WAIT_FOR_SERIAL) {
            break;
        }
    }
    #endif

    Serial.println("Begin");

    corePlatform.setup();

    Serial1.begin(9600);
    platformSerial2Begin(9600);

    Serial.println("Loop");
}

void loop() {
    SerialPortExpander portExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1);
    ScriptRunner scriptRunner;
    Repl repl(&portExpander, &scriptRunner);

    repl.showPrompt();

    while (1) {
        repl.tick();

        delay(50);
    }
}

// vim: set ft=cpp:
