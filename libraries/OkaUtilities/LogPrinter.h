#ifndef LOG_PRINTER_H_INCLUDED
#define LOG_PRINTER_H_INCLUDED

class LogPrinter : public Stream {
private:
    bool serial1Relay = false;
    bool serial2Relay = false;

public:
    bool open(bool serial1Relay = false, bool serial2Relay = false);

public:
    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual void flush() override;
    virtual size_t write(uint8_t) override;
    virtual size_t write(const uint8_t *buffer, size_t size) override;
};

extern LogPrinter logPrinter;

#endif
