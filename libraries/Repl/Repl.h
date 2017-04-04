#ifndef REPL_H
#define REPL_H

#include "SimpleBuffer.h"

typedef enum ReplState {
    Working,
    Prompt,
    Command
} ReplState;

class Repl {
private:
    ReplState state = ReplState::Prompt;
    SimpleBuffer buffer;

public:
    Repl() {
    }

    virtual const char *getPromptPrefix() {
        return "";
    }

    void transition(ReplState newState) {
        state = newState;
    }

    virtual bool doWork() {
        return false;
    }

    void busy() {
        transition(ReplState::Working);
    }

    void promptIfNecessary() {
        if (state == ReplState::Working) {
            transition(ReplState::Prompt);
        }
    }

    bool tick() {
        switch (state) {
        case ReplState::Working: {
            if (!doWork()) {
                transition(ReplState::Prompt);
            }

            break;
        }
        case ReplState::Prompt: {
            showPrompt();
            transition(ReplState::Command);
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

                    transition(ReplState::Prompt);

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
        Serial.print(getPromptPrefix());
        Serial.print("> ");
    }

    virtual void handle(String command) {
    }
};

#endif
