#ifndef SELF_RESTART_H_INCLUDED
#define SELF_RESTART_H_INCLUDED

#define MANDATORY_RESTART_INTERVAL   (1000 * 60 * 60 * 6)

#include <SD.h>

class SelfRestart {
public:
    static bool isRestartNecessary(uint32_t uptime) {
        if (uptime > MANDATORY_RESTART_INTERVAL) {
            DEBUG_PRINTLN("Mandatory restart triggered.");
            return true;
        }
        return false;
    }
};

#endif
