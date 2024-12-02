#include <stdio.h>
#include <lwip/err.h>
#include <lwip/pbuf.h>
#include <lwip/tcp.h>
#include <lwip/netif.h>
#include <lwip/ip4.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "WS2812.hpp"
#include "pico-ssd1306/ssd1306.h"
#include "pico-ssd1306/textRenderer/TextRenderer.h"
#include "buzzer.h"
#include "buzzer_melodies.h"
#include "button.h"
#include "wifi.h"
#include "rtc.h"

#define RGBLED_PIN 6
#define RGBLED_LENGTH 6
#define LED_PIN 7
#define BUTTON_PIN 10
#define BUZZER_PIN 20

class Scheduler
{
private:
    WS2812 ledStrip;
    Buzzer buzzer;
    Button button;
    pico_ssd1306::SSD1306 display;
    WiFi wifi;
    RTC rtc;

    void ClearFlags()
    {
        displayDeskError = false;
        displayPreAlarm = false;
        displayAlarm = false;
        displayLogin = false;
        displayLogout = false;
    }

    void HandleError(const char *query)
    {
        displayDeskError = true;
    }

    void HandleErrorEnd(const char *query)
    {
        displayDeskError = false;
    }

    void HandlePreAlarm(const char *query)
    {
        displayPreAlarm = true;
    }

    void HandleAlarm(const char *query)
    {
        int position = -1;
        char melody = '\0';

        if (query && strlen(query) > 0)
        {
            char *positionStr = strstr(const_cast<char *>(query), "position=");
            char *melodyStr = strstr(const_cast<char *>(query), "melody=");

            if (positionStr)
            {
                positionStr += 9; // Skip "position="
                position = atoi(positionStr);
            }

            if (melodyStr)
            {
                melodyStr += 7; // Skip "melody="
                melody = *melodyStr;
            }
        }

        if (position > 0 && melody != '\0')
        {
            positionToDisplay = position;
            melodyToPlay = melody;
            displayAlarm = true;
        }
        else
        {
            printf("Invalid or missing parameters for alarm.\n");
        }
    }

    void HandleLogin(const char *query)
    {
        char *usernameStr = strstr(const_cast<char *>(query), "username=");

        if (usernameStr)
        {
            usernameStr += 9; // Skip "username="
            strncpy(usernameToDisplay, usernameStr, sizeof(usernameToDisplay) - 1);
            usernameToDisplay[sizeof(usernameToDisplay) - 1] = '\0';
            displayLogin = true;
        }
        else
        {
            printf("Username not provided in login request.\n");
        }
    }

    void HandleLogout(const char *query)
    {
        displayLogout = true;
    }

    // http code starts here
    void parse_http_request(const char *request, char *path, size_t pathSize, char *query, size_t querySize)
    {
        const char *pathStart = strchr(request, ' ') + 1;
        const char *queryStart = strchr(pathStart, '?');
        const char *pathEnd = strchr(pathStart, ' ');

        size_t pathLength = (queryStart ? queryStart : pathEnd) - pathStart;
        strncpy(path, pathStart, pathLength < pathSize ? pathLength : pathSize - 1);

        if (queryStart)
        {
            strncpy(query, queryStart + 1, pathEnd - queryStart - 1 < querySize ? pathEnd - queryStart - 1 : querySize - 1);
        }
        else
        {
            query[0] = '\0';
        }
    }

    const char *match_route_and_handle(const char *path, const char *query)
    {
        struct Route
        {
            const char *path;
            const char *response;
            void (Scheduler::*handler)(const char *query);
        };
        const char *response = "{\"result\":\"success\"}";
        static const Route routes[] = {
            {"/api/error", response, &Scheduler::HandleError},
            {"/api/errend", response, &Scheduler::HandleErrorEnd},
            {"/api/prealarm", response, &Scheduler::HandlePreAlarm},
            {"/api/alarm", response, &Scheduler::HandleAlarm},
            {"/api/login", response, &Scheduler::HandleLogin},
            {"/api/logout", response, &Scheduler::HandleLogout},
        };

        for (const auto &route : routes)
        {
            if (strncmp(path, route.path, strlen(route.path)) == 0)
            {
                if (route.handler)
                {
                    (this->*route.handler)(query);
                }
                return route.response;
            }
        }

        response = "{\"result\":\"error\",\"error\":\"404 Not Found: Path does not exist\"}";
        return response;
    }

    void handle_request(struct tcp_pcb *tpcb, const char *request)
    {
        constexpr size_t PATH_BUFFER_SIZE = 64, QUERY_BUFFER_SIZE = 64;
        char path[PATH_BUFFER_SIZE] = {}, query[QUERY_BUFFER_SIZE] = {};
        parse_http_request(request, path, PATH_BUFFER_SIZE, query, QUERY_BUFFER_SIZE);

        const char *responseBody = match_route_and_handle(path, query);
        char response[256];
        int responseLength = snprintf(response, sizeof(response),
                                      "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %zu\r\n\r\n%s", strlen(responseBody), responseBody);

        tcp_write(tpcb, response, responseLength, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
    }

    static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
    {
        constexpr size_t REQUEST_BUFFER_SIZE = 128;
        if (!p)
            return tcp_close(tpcb), ERR_OK;

        char request[REQUEST_BUFFER_SIZE] = {};
        pbuf_copy_partial(p, request, sizeof(request) - 1, 0);
        pbuf_free(p);

        static_cast<Scheduler *>(arg)->handle_request(tpcb, request);
        return ERR_OK;
    }

    static err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
    {
        tcp_recv(newpcb, http_recv);
        tcp_arg(newpcb, arg);
        return ERR_OK;
    }
    // http code ends here

    void UpdateIdleDisplay(datetime_t t)
    {
        char clock_buf[50];
        snprintf(clock_buf, sizeof(clock_buf), "%02d:%02d", t.hour, t.min);
        display.clear();
        drawText(&display, font_16x32, clock_buf, 24, 16);
        drawText(&display, font_5x8, "Group 7", 46, 56);
        display.sendBuffer();
    }

    void UpdateDisplay(const char *title,
                       const char *line1 = "",
                       const char *line2 = "",
                       const char *line3 = "",
                       const char *line4 = "")
    {
        display.clear();
        drawText(&display, font_12x16, title, 0, 0);
        if (line1)
            drawText(&display, font_5x8, line1, 0, 16);
        if (line2)
            drawText(&display, font_5x8, line2, 0, 26);
        if (line3)
            drawText(&display, font_5x8, line3, 0, 36);
        if (line4)
            drawText(&display, font_5x8, line4, 0, 46);
        drawText(&display, font_5x8, "Group 7", 46, 56);
        display.sendBuffer();
    }

    void WaitForButtonPress(int timeout_ms = -1)
    {
        int elapsed = 0;
        while (timeout_ms == -1 || elapsed < timeout_ms)
        {
            sleep_ms(1);
            elapsed++;
            if (buttonPressed())
                break;
        }
    }

    void ActivateLED(uint32_t color)
    {
        ledStrip.fill(color);
        ledStrip.show();
    }

    void ClearLEDAndDisplay()
    {
        ledStrip.fill(WS2812::RGB(0, 0, 0));
        ledStrip.show();
        display.clear();
        display.sendBuffer();
    }

public:
    Scheduler() : ledStrip(RGBLED_PIN, RGBLED_LENGTH, pio0, 0, WS2812::FORMAT_GRB),
                  buzzer(BUZZER_PIN),
                  button(BUTTON_PIN),
                  display(i2c_default, 0x3C, pico_ssd1306::Size::W128xH64),
                  wifi(WIFI_SSID, WIFI_PASSWORD)
    {
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        gpio_pull_up(LED_PIN);
        display.setOrientation(0);
        ClearLEDAndDisplay();
        cyw43_arch_enable_sta_mode();
        printf("Scheduler initialized\n");
    }
    
    int positionToDisplay = 0;
    char melodyToPlay = '\0';
    char usernameToDisplay[11] = {};

    bool displayDeskError = false;
    bool displayPreAlarm = false;
    bool displayAlarm = false;
    bool displayLogin = false;
    bool displayLogout = false;

    void run()
    {
        while (!wifi.connect())
            ActivateConnectionError();

        printf("Scheduler running\n");

        const ip4_addr_t *localIP = netif_ip4_addr(netif_default);
        if (localIP)
            printf("assigned local ipv4 address: %s\n", ip4addr_ntoa(localIP));
        else
            printf("failed to obtain local ipv4 address\n");

        struct tcp_pcb *pcb = tcp_new();
        if (!pcb)
        {
            printf("tcp_pcb allocation failed\n");
            return;
        }

        tcp_bind(pcb, IP_ADDR_ANY, 80);
        pcb = tcp_listen(pcb);
        tcp_accept(pcb, http_accept);
        tcp_arg(pcb, this);

        while (true)
        {
            if (displayDeskError)
                ActivateDeskError();
            if (displayPreAlarm)
                ActivatePreAlarm();
            if (displayAlarm)
                ActivateAlarm();
            if (displayLogin)
                ActivateLogin();
            if (displayLogout)
                ActivateLogout();
            UpdateIdleDisplay(rtc.get_rtc_time());
            sleep_ms(100);
        }
    }

    inline bool buttonPressed()
    {
        if (button.is_pressed())
        {
            gpio_put(LED_PIN, true);
            sleep_ms(10);
            gpio_put(LED_PIN, false);
            return true;
        }
        return false;
    }

    void ActivateConnectionError()
    {
        UpdateDisplay("Conn.Error", "Cannot connect to Wi-Fi", "", "", "Press button to try again");
        printf("Unable to start Scheduler due to Wi-Fi connection failure.\n");

        int brightness = 0;
        int step = 1; // Controls whether we fade in or out

        while (true)
        {
            brightness += step;
            // Reverse direction at bounds
            if (brightness <= 0 || brightness >= 128)
                step = -step;

            ActivateLED(WS2812::RGB(0, 0, brightness)); // Blue pulse
            sleep_ms(5);

            if (buttonPressed())
            {
                cyw43_arch_deinit();
                if (cyw43_arch_init())
                {
                    printf("failed to initialise\n");
                    continue;
                }
                cyw43_arch_enable_sta_mode();
                break;
            }
        }
        ClearLEDAndDisplay();
    }

    void ActivateDeskError()
    {
        UpdateDisplay("Desk Error", "", "Desk returning error code", "Resolve error to proceed");
        ActivateLED(WS2812::RGB(128, 0, 0)); // Red

        while (displayDeskError) // flag needs to be cleared via separate API call
            sleep_ms(100);

        ClearLEDAndDisplay();
        ClearFlags();
    }

    void ActivateLogin(int seconds = 10)
    {
        char userMsg[50];
        snprintf(userMsg, sizeof(userMsg), "Logged in as %s", usernameToDisplay);
        UpdateDisplay("Welcome", "", userMsg, "Press button to dismiss");
        WaitForButtonPress(seconds * 1000);
        ClearLEDAndDisplay();
        ClearFlags();
    }

    void ActivateLogout(int seconds = 10)
    {
        UpdateDisplay("Logging out", "", "Have a nice day!", "Press button to dismiss");
        WaitForButtonPress(seconds * 1000);
        ClearLEDAndDisplay();
        ClearFlags();
    }

    void ActivatePreAlarm(int seconds = 10)
    {
        UpdateDisplay("Warning", "", "Desk alarm will play soon", "Press button to dismiss");
        ActivateLED(WS2812::RGB(128, 128, 0)); // Yellow
        WaitForButtonPress(seconds * 1000);
        ClearLEDAndDisplay();
        ClearFlags();
    }

    void ActivateAlarm()
    {
        char positionMsg[50];
        snprintf(positionMsg, sizeof(positionMsg), "Changing position to %d", positionToDisplay);

        const char *melodyName;
        switch (melodyToPlay)
        {
        case 'B':
            melodyName = "Beep";
            buzzer.playMelody(BeepMelody);
            break;
        case 'E':
            melodyName = "Breeze";
            buzzer.playMelody(BreezeMelody);
            break;
        case 'M':
            melodyName = "Brrr";
            buzzer.playMelody(RumbleMelody);
            break;
        case 'Z':
            melodyName = "Bzzz";
            buzzer.playMelody(BzzzMelody);
            break;
        case 'D':
            melodyName = "DOOM";
            buzzer.playMelody(DoomMelody);
            break;
        case 'R':
            melodyName = "Rick Roll";
            buzzer.playMelody(RickRollMelody);
            break;
        case 'N':
            melodyName = "Nokia ringtone";
            buzzer.playMelody(NokiaMelody);
            break;
        case 'K':
            melodyName = "Krusty Krab";
            buzzer.playMelody(KrabMelody);
            break;
        case 'P':
            melodyName = "Pink Panther";
            buzzer.playMelody(PinkPantherMelody);
            break;
        default:
            melodyName = "None";
            buzzer.playMelody(NoMelody);
            break;
        }

        char melodyMsg[50];
        snprintf(melodyMsg, sizeof(melodyMsg), "Playing: %s", melodyName);

        UpdateDisplay("Desk Alarm", positionMsg, melodyMsg, "Press button to dismiss");
        ActivateLED(WS2812::RGB(0, 128, 0)); // Green

        while (!buzzer.isDone())
            if (buttonPressed())
                break;
        buzzer.stopMelody();
        ClearLEDAndDisplay();
        ClearFlags();
    }

    ~Scheduler()
    {
        buzzer.stopMelody();
        ClearLEDAndDisplay();
    }
};

int main()
{
    stdio_init_all();
    printf("Starting Desk Alarm System\n");

    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        return 1;
    }

    // Set default baudrate for i2c display
    uint baudrate = 500000;

    // Init i2c0 controller for display
    i2c_init(i2c_default, baudrate);
    // Set up pins
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // If you don't do anything before initializing a display pi pico is too fast and starts sending
    // commands before the screen controller had time to set itself up, so we add an artificial delay for
    // ssd1306 to set itself up
    sleep_ms(250);

    Scheduler scheduler;
    scheduler.run();

    cyw43_arch_deinit();
    return 0;
}
