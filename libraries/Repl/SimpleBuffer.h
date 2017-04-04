#ifndef SIMPLE_BUFFER_H
#define SIMPLE_BUFFER_H

class SimpleBuffer {
private:
    char buffer[256];
    size_t length;

public:		
    SimpleBuffer() : length(0) {
        memset(buffer, 0, sizeof(buffer));
    }

    void append(char c) {
        if (length < sizeof(buffer) - 1) {
            buffer[length++] = c;
            buffer[length] = 0;
        }
    }

    const char *c_str() {
        return buffer;
    }

    void clear() {
        length = 0;
        buffer[0] = 0;
    }
};

#endif
