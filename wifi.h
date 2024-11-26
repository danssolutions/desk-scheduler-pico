#include "pico/cyw43_arch.h"
#include <string>
#include <stdio.h>

class WiFiManager
{
private:
    const char *ssid;
    const char *password;
    bool connected;

public:
    WiFiManager(const char *ssid, const char *password);

    bool connect();

    bool isConnected() const;

    std::string listenForRequest();

    ~WiFiManager();
};