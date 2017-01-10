#ifndef WATCHDOG_CALLBACKS_H_INCLUDED
#define WATCHDOG_CALLBACKS_H_INCLUDED

class WatchdogCallbacks : public IridiumCallbacks  {
public:
    virtual void tick() override {
        Watchdog.reset();
    }
};

#endif
