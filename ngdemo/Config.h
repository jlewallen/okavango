#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

class Config {
private:
    String ssid;
    String password;
    String urlServer;
    String urlPath;

public:
    const char *getSsid() { return ssid.c_str(); }
    const char *getPassword() { return password.c_str(); }
    const char *getUrlPath() {
        return urlPath.c_str();
    }
    const char *getUrlServer() {
        return urlServer.c_str();
    }

public:
    bool read();

};

#endif
