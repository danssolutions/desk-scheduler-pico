#include "wifi.h"

WiFiManager::WiFiManager(const char *ssid, const char *password)
    : ssid(ssid), password(password), connected(false) {}

bool WiFiManager::connect()
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

bool WiFiManager::isConnected() const
{
    return connected;
}

std::string WiFiManager::listenForRequest()
{
    // Simulated incoming request. Replace with real implementation, such as a TCP/UDP server.
    printf("Simulated incoming request received.\n");

    // Example: Returning a mock request
    return "ALARM,1234,N"; // "TYPE,POSITION,MELODY"
}

WiFiManager::~WiFiManager()
{
    if (connected)
    {
        cyw43_arch_deinit();
        printf("Wi-Fi disconnected\n");
    }
}
