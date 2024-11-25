#include <stdio.h>
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

#define RGBLED_PIN 6
#define RGBLED_LENGTH 6
#define LED_PIN 7
#define BUTTON_PIN 10
#define BUZZER_PIN 20

class Scheduler {
private:
    WS2812 ledStrip;
    Buzzer buzzer;
    Button button;
    pico_ssd1306::SSD1306 display;

    void UpdateDisplay(const char* title,
        const char* line1 = "",
        const char* line2 = "",
        const char* line3 = "",
        const char* line4 = "")
    {
        display.clear();
        display.setOrientation(0);
        drawText(&display, font_12x16, title, 0, 0);
        if (line1) drawText(&display, font_5x8, line1, 0, 16);
        if (line2) drawText(&display, font_5x8, line2, 0, 26);
        if (line3) drawText(&display, font_5x8, line3, 0, 36);
        if (line4) drawText(&display, font_5x8, line4, 0, 46);
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
            if (buttonPressed()) break;
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
                  display(i2c_default, 0x3C, pico_ssd1306::Size::W128xH64) {
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        gpio_pull_up(LED_PIN);
        cyw43_arch_enable_sta_mode();
        printf("Scheduler initialized\n");
    }

    void run()
    {
        printf("Scheduler running\n");

        char username[] = "Ronaldinho";
        while (true)
        {
            ActivateConnectionError();
            sleep_ms(10000);
            ActivateDeskError();
            sleep_ms(10000);
            ActivateLogin(username);
            sleep_ms(10000);
            ActivatePreAlarm();
            sleep_ms(10000);
            ActivateAlarm(1234, 'K');
            sleep_ms(10000);
            ActivateLogout();
            sleep_ms(10000);
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
        UpdateDisplay("Conn.Error", "", "Cannot connect to app", "Resolve error to proceed");
        
        int brightness = 0;
        int step = 1; // Controls whether we fade in or out

        while (true)
        {
            brightness += step;
            // Reverse direction at bounds
            if (brightness <= 0 || brightness >= 128) step = -step; 

            ActivateLED(WS2812::RGB(0, 0, brightness)); // Blue pulse
            sleep_ms(5);

            // TODO: wait until error is resolved before proceeding.
            // For now just cancel the warning by button press
            if (buttonPressed()) break;
        }
        ClearLEDAndDisplay();
    }

    void ActivateDeskError()
    {
        UpdateDisplay("Desk Error", "", "Desk returning error code", "Resolve error to proceed");
        ActivateLED(WS2812::RGB(128, 0, 0)); // Red
        WaitForButtonPress(); // TODO: change with error checking
        ClearLEDAndDisplay();
    }

    void ActivateLogin(const char* username, int seconds = 10)
    {
        char userMsg[50];
        snprintf(userMsg, sizeof(userMsg), "Logged in as %s", username);
        UpdateDisplay("Welcome", "", userMsg, "Press button to dismiss");
        WaitForButtonPress(seconds * 1000);
        ClearLEDAndDisplay();
    }

    void ActivateLogout(int seconds = 10)
    {
        UpdateDisplay("Logging out", "", "Have a nice day!", "Press button to dismiss");
        WaitForButtonPress(seconds * 1000);
        ClearLEDAndDisplay();
    }

    void ActivatePreAlarm(int seconds = 10)
    {
        UpdateDisplay("Warning", "", "Desk alarm will play soon", "Press button to dismiss");
        ActivateLED(WS2812::RGB(128, 128, 0)); // Yellow
        WaitForButtonPress(seconds * 1000);
        ClearLEDAndDisplay();
    }

    void ActivateAlarm(int position, char alarmMelody)
    {
        char positionMsg[50];
        snprintf(positionMsg, sizeof(positionMsg), "Changing position to %d", position);

        const char* melodyName;
        switch (alarmMelody)
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
            break;
        }

        char melodyMsg[50];
        snprintf(melodyMsg, sizeof(melodyMsg), "Playing: %s", melodyName);

        UpdateDisplay("Desk Alarm", positionMsg, melodyMsg, "Press button to dismiss");
        ActivateLED(WS2812::RGB(0, 128, 0)); // Green

        
        while (!buzzer.isDone())
            if (buttonPressed()) break;
        buzzer.stopMelody();
        ClearLEDAndDisplay();
    }

    ~Scheduler() {}
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
    uint baudrate = 800000;

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
