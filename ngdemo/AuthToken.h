#ifndef AUTH_TOKEN_H_INCLUDED
#define AUTH_TOKEN_H_INCLUDED

class AuthToken {
private:
    uint8_t buffer[64];
    size_t size;

public:
    bool read(const char *fn);
    size_t include(uint8_t *ptr, size_t available);

};

#endif
