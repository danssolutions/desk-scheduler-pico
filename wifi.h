#include "pico/cyw43_arch.h"
#include <string>
#include <stdio.h>

class WiFi
{
private:
    const char *ssid;
    const char *password;
    bool connected;

public:
    WiFi(const char *ssid, const char *password)
        : ssid(ssid), password(password), connected(false) {}

    bool connect()
    {
        printf("Connecting to Wi-Fi...\n");
        if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 10000))
        {
            printf("Failed to connect to Wi-Fi\n");
            connected = false;
        }
        else
        {
            printf("Wi-Fi connected\n");
            connected = true;
        }
        return connected;
    }

    bool isConnected() const
    {
        return connected;
    }

    ~WiFi()
    {
        if (connected)
        {
            cyw43_arch_deinit();
            printf("Wi-Fi disconnected\n");
        }
    }
};
