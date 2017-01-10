#ifndef SELF_RESTART_H_INCLUDED
#define SELF_RESTART_H_INCLUDED

#define MANDATORY_RESTART_INTERVAL   (1000 * 60 * 60 * 3)
#define MANDATORY_RESTART_FILE       "RESUME.INF"

#include <SD.h>

class SelfRestart {
public:
    static void restartIfNecessary() {
        if (millis() > MANDATORY_RESTART_INTERVAL) {
            File file = SD.open(MANDATORY_RESTART_FILE, FILE_WRITE);
            if (!file) {
                // Consider not doing the restart now? Maybe waiting another interval?
                DEBUG_PRINTLN("Error creating restart indicator file.");
            }
            else {
                file.close();
            }
            DEBUG_PRINTLN("Mandatory restart triggered.");
            logPrinter.flush();
            platformRestart();
        }
    }

    static bool didWeJustRestart() {
        if (SD.exists(MANDATORY_RESTART_FILE)) {
            SD.remove(MANDATORY_RESTART_FILE);
            return true;
        }
        return false;
    }
};

#endif
