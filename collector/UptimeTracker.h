#ifndef UPTIME_TRACKER_H
#define UPTIME_TRACKER_H

class UptimeTracker {
public:
    static bool shouldWeRelax();
    static void started();
    static void remember();
};

#endif
