#include "Platforms.h"
#include "core.h"
#include "SerialPortExpander.h"
#include "NonBlockingSerial.h"
#include "Repl.h"

String phScript[] = {
    "c,0",
    "l,1",
    "cal,clear",
    "cal,mid,7.00",
    "cal,low,4.00",
    "cal,high,10.00",
    "l,0",
    "STATUS"
};

String doScript[] = {
    "c,0",
    "l,1",
    "cal,clear",
    "cal",
    "cal,0",
    "L,0",
    "STATUS"
};

String orpScript[] = {
    "c,0",
    "l,1",
    "cal,clear",
    "cal,225",
    "L,0",
    "STATUS"
};

String ecScript[] = {
    "c,0",
    "l,1",
    "cal,clear",
    "cal,dry",
    "cal,low,12880",
    "cal,high,80000",
    "L,0",
    "STATUS"
};

ConductivityConfig conductivityConfig = ConductivityConfig::OnSerial2;

SerialType *getSerialForPort(uint8_t port) {
    if (port < 3 || conductivityConfig != ConductivityConfig::OnSerial2) {
        return &Serial1;
    }
    else if (port == 3) {
        return &Serial2;
    }
    return &Serial1;
}

typedef enum ScriptRunnerState {
    WaitingOnReply,
    DeviceIdle
} ScriptRunnerState;

class ScriptRunner : public NonBlockingSerialProtocol {
private:
    SerialPortExpander *portExpander;
    ScriptRunnerState state;
    int8_t position;
    char activeCommand[64];
    char expectedResponse[64];
    String *commands;
    int8_t length;
    uint8_t nextPort;

public:
    ScriptRunner(SerialPortExpander *portExpander) :
        NonBlockingSerialProtocol(10000, true), portExpander(portExpander),
        commands(NULL), position(0), length(0), nextPort(0) {
        activeCommand[0] = 0;
        expectedResponse[0] = 0;
    }

    void select(uint8_t port) {
        portExpander->select(port);
        SerialType *serial = getSerialForPort(port);
        setSerial(serial);
        drain();
    }

    void noScript() {
        commands = nullptr;
        position = 0;
        length = 0;
    }

    template<int8_t N>
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

    void changePosition(int8_t by) {
        int8_t newPosition = (int8_t)position + by;

        if (newPosition >= length) {
            position = length - 1;
        }
        else if (newPosition < 0) {
            position = 0;
        }
        else {
            position = newPosition;
        }

    }

    void startOver() {
        position = 0;
    }

    void send() {
        send(currentCommand(), "*OK");
        if (position < length) {
            position++;
        }
    }

    void sendEverywhere(const char *command, const char *expected) {
        nextPort = 1;
        send(command, expected);
    }

    void send(const char *command, const char *expected) {
        strncpy(activeCommand, command, sizeof(activeCommand));
        strncpy(expectedResponse, expected, sizeof(expectedResponse));
        state = ScriptRunnerState::WaitingOnReply;
        sendCommand(command);
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

    virtual NonBlockingHandleStatus handle(String reply) override {
        if (reply.length() > 0) {
            Serial.print("# ");
            Serial.print(reply);
        }
        if (expectedResponse != NULL && reply.indexOf(expectedResponse) >= 0) {
            if (nextPort >= 4) {
                state = ScriptRunnerState::DeviceIdle;
                return NonBlockingHandleStatus::Handled;
            }
            else if (nextPort > 0) {
                Serial.print("PORT ");
                Serial.println(nextPort);
                select(nextPort);
                send(activeCommand, expectedResponse);
                nextPort++;
                return NonBlockingHandleStatus::Ignored;
            }
            state = ScriptRunnerState::DeviceIdle;
            return NonBlockingHandleStatus::Handled;
        }
        else if (reply.indexOf("*W") == 0) {
            Serial.println("Woke up, resend...");
            send(activeCommand, expectedResponse);
            return NonBlockingHandleStatus::Handled;
        }
        else if (reply.indexOf("*ER") == 0) {
            state = ScriptRunnerState::DeviceIdle;
            return NonBlockingHandleStatus::Handled;
        }
        return NonBlockingHandleStatus::Unknown;
    }
};

class CalibratorRepl : public Repl {
private:
    ScriptRunner *scriptRunner;

public:
    CalibratorRepl(ScriptRunner *scriptRunner) : scriptRunner(scriptRunner) {
    }

    const char *getPromptPrefix() {
        return scriptRunner->currentCommand();
    }

    virtual bool doWork() override {
        return scriptRunner->tick();
    }

    void handle(String command) {
        if (command.startsWith("p ")) {
            uint8_t number = command.substring(2).toInt();
            scriptRunner->select(number);
        }
        else if (command.startsWith("ph")) {
            Serial.println("Ph Mode");
            scriptRunner->setScript(phScript);
            scriptRunner->select(2);
        }
        else if (command.startsWith("do")) {
            Serial.println("Do Mode");
            scriptRunner->setScript(doScript);
            scriptRunner->select(1);
        }
        else if (command.startsWith("orp")) {
            Serial.println("Orp Mode");
            scriptRunner->setScript(orpScript);
            scriptRunner->select(0);
        }
        else if (command.startsWith("ec")) {
            Serial.println("Ec Mode");
            scriptRunner->setScript(ecScript);
            scriptRunner->select(3);
        }
        else if (command.startsWith("none")) {
            Serial.println("No script");
            scriptRunner->noScript();
        }
        else if (command.startsWith("-")) {
            scriptRunner->changePosition(-1);
        }
        else if (command.startsWith("+")) {
            scriptRunner->changePosition(1);
        }
        else if (command.startsWith("wake")) {
            scriptRunner->select(0);
            scriptRunner->sendEverywhere("STATUS", "*OK");
            transition(ReplState::Working);
        }
        else if (command.startsWith("factory")) {
            Serial.println("Factory Reset All The Things!");
            scriptRunner->select(0);
            scriptRunner->sendEverywhere("factory", "*RE");
            transition(ReplState::Working);
        }
        else if (command.startsWith("!")) {
            scriptRunner->select(0);
            scriptRunner->sendEverywhere(command.substring(1).c_str(), "*OK");
            transition(ReplState::Working);
        }
        else if (command.startsWith("$")) {
            scriptRunner->send(command.substring(1).c_str(), "*OK");
            transition(ReplState::Working);
        }
        else if (command.startsWith("?")) {
            Serial.println("Help");
            Serial.println();
            Serial.println("  p N     change to port N where N is 0..3");
            Serial.println("  !CMD    send CMD to every module");
            Serial.println("  $CMD    send CMD to current module");
            Serial.println("  factory send FACTORY to every module");
            Serial.println("  wake    wake all modules");
            Serial.println();
            Serial.println("  orp     set orp script and change port to 0");
            Serial.println("  do      set do script and change port to 1");
            Serial.println("  ph      set ph script and change port to 2");
            Serial.println("  ec      set ec script and change port to 3");
            Serial.println("  none    clear script");
            Serial.println("  -       backup one command in the script");
            Serial.println("  +       forward one command in the script");

        }
        else if (command == "") {
            scriptRunner->send();
            transition(ReplState::Working);
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

    corePlatform.setup(PIN_SD_CS, PIN_RFM95_CS, PIN_RFM95_RST, false);

    Serial1.begin(9600);
    platformSerial2Begin(9600);

    Serial.println("Loop");
}

void loop() {
    SerialPortExpander portExpander(PORT_EXPANDER_SELECT_PIN_0, PORT_EXPANDER_SELECT_PIN_1, conductivityConfig);
    ScriptRunner scriptRunner(&portExpander);
    CalibratorRepl repl(&scriptRunner);

    scriptRunner.select(0);

    while (1) {
        repl.tick();

        delay(50);
    }
}

// vim: set ft=cpp:
