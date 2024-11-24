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

class Scheduler
{
private:
    WS2812 ledStrip;
    Buzzer buzzer;
    Button button;
    pico_ssd1306::SSD1306 display;

public:
    Scheduler() : ledStrip(RGBLED_PIN, RGBLED_LENGTH, pio0, 0, WS2812::FORMAT_GRB),
                  buzzer(BUZZER_PIN),
                  button(BUTTON_PIN),
                  display(i2c_default, 0x3C, pico_ssd1306::Size::W128xH64)
    {
        // Init LED on pin 7
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        gpio_pull_up(LED_PIN);

        // Set display orientation so it's not flipped
        display.setOrientation(0);

        cyw43_arch_enable_sta_mode();
        absolute_time_t scan_time = nil_time;
        bool scan_in_progress = false;

        printf("Scheduler constructed\n");
    }

    // Main loop
    void run()
    {
        printf("Scheduler running\n");

        int position = 9999;    // Example placeholder
        char alarmMelody = 'D'; // Example placeholder
        char username[] = "Ronaldinho"; // Example placeholder

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
            ActivateAlarm(position, alarmMelody);
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

    inline void ActivateConnectionError()
    {
        display.clear();
        drawText(&display, font_12x16, "Conn.Error", 0, 0);
        drawText(&display, font_5x8, "Cannot connect to app", 0, 18);
        drawText(&display, font_5x8, "Resolve error to proceed", 0, 38);
        drawText(&display, font_5x8, "Group 7", 46, 56);
        display.sendBuffer();

        int brightness = 0;
        bool fadeOut = false;
        while (true)
        {
            if (!brightness)
                fadeOut = false;
            else if (brightness > 128)
                fadeOut = true;
            if (fadeOut)
                brightness--;
            else
                brightness++;
            
            ledStrip.fill(WS2812::RGB(0, 0, brightness));
            ledStrip.show();
            sleep_ms(5);
            // TODO: this is where we should check for status
            // for now, just use button to close the warning
            if (buttonPressed()) break;
        }

        // Clear display
        display.clear();
        display.sendBuffer();
        ledStrip.fill(WS2812::RGB(0, 0, 0));
        ledStrip.show();
    }

    inline void ActivateDeskError()
    {
        ledStrip.fill(WS2812::RGB(255, 0, 0));
        ledStrip.show();

        display.clear();
        drawText(&display, font_12x16, "Desk Error", 0, 0);
        drawText(&display, font_5x8, "Desk returning error code", 0, 18);
        drawText(&display, font_5x8, "Resolve error to proceed", 0, 38);
        drawText(&display, font_5x8, "Group 7", 46, 56);
        display.sendBuffer();

        while (true)
        {
            sleep_ms(1);
            // TODO: this is where we should check for status
            // for now, just use button to close the warning
            if (buttonPressed()) break;
        }

        // Clear display
        display.clear();
        display.sendBuffer();
        ledStrip.fill(WS2812::RGB(0, 0, 0));
        ledStrip.show();
    }

    inline void ActivateLogin(char username[], int seconds = 10)
    {
        display.clear();
        drawText(&display, font_12x16, "Welcome", 16, 0);
        drawText(&display, font_5x8, "Logged in as", 0, 18);

        for (int i = 0; i < 10; i++)
            drawChar(&display, font_5x8, username[i], 65 + 5 * i, 18);

        drawText(&display, font_5x8, "Press button to dismiss", 0, 38);
        drawText(&display, font_5x8, "Group 7", 46, 56);
        display.sendBuffer();

        for (int i = 0; i < (1000 * seconds); i++)
        {
            sleep_ms(1);
            if (buttonPressed()) break;
        }

        // Clear display
        display.clear();
        display.sendBuffer();
    }

    inline void ActivateLogout(int seconds = 10)
    {
        display.clear();
        drawText(&display, font_12x16, "Logging out", 0, 0);
        drawText(&display, font_5x8, "Have a nice day!", 16, 18);
        drawText(&display, font_5x8, "Press button to dismiss", 0, 38);
        drawText(&display, font_5x8, "Group 7", 46, 56);
        display.sendBuffer();

        for (int i = 0; i < (1000 * seconds); i++)
        {
            sleep_ms(1);
            if (buttonPressed()) break;
        }

        // Clear display
        display.clear();
        display.sendBuffer();
    }

    inline void ActivatePreAlarm(int seconds = 10)
    {
        ledStrip.fill(WS2812::RGB(255, 255, 0));
        ledStrip.show();

        display.clear();
        drawText(&display, font_12x16, "Warning", 16, 0);
        drawText(&display, font_5x8, "Desk alarm will play soon", 0, 18);
        drawText(&display, font_5x8, "Press button to dismiss", 0, 38);
        drawText(&display, font_5x8, "Group 7", 46, 56);
        display.sendBuffer();

        for (int i = 0; i < (1000 * seconds); i++)
        {
            sleep_ms(1);
            if (buttonPressed()) break;
        }
        
        // Clear display and LED strip
        display.clear();
        display.sendBuffer();
        ledStrip.fill(WS2812::RGB(0, 0, 0));
        ledStrip.show();
    }

    // Handles logic for alarm sound and display
    inline void ActivateAlarm(int position, char alarmMelody)
    {
        ledStrip.fill(WS2812::RGB(0, 255, 0));
        ledStrip.show();

        display.clear();
        drawText(&display, font_12x16, "Desk Alarm", 3, 0);
        drawText(&display, font_5x8, "Changing position to", 0, 18);

        char charpos[5];
        snprintf(charpos, sizeof charpos, "%d", position);
        for (int i = 0; i < 4; i++)
            drawChar(&display, font_5x8, charpos[i], 105 + 5 * i, 18);

        drawText(&display, font_5x8, "Playing:", 0, 28);

        // TODO: add support for more alarms, or redo how they're passed
        switch (alarmMelody)
        {
        case 'D':
            drawText(&display, font_5x8, "DOOM", 45, 28);
            buzzer.playMelody(DoomMelody);
            break;
        case 'R':
            drawText(&display, font_5x8, "Rick Roll", 45, 28);
            buzzer.playMelody(RickRollMelody);
            break;
        default:
            drawText(&display, font_5x8, "None", 45, 28);
            break;
        }

        drawText(&display, font_5x8, "Press btn to stop sound", 0, 38);
        drawText(&display, font_5x8, "Group 7", 46, 56);
        display.sendBuffer();

        // Wait for melody to finish or button press
        while (!buzzer.isDone())
        {
            if (buttonPressed()) break;
        }

        buzzer.stopMelody();
        // Clear display and LED strip
        display.clear();
        display.sendBuffer();
        ledStrip.fill(WS2812::RGB(0, 0, 0));
        ledStrip.show();
    }

    ~Scheduler()
    {
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
    uint baudrate = 800000;

    // Uncomment to allow calibrating baudrate using potmeter
    // ADC potentiometer(0, true);
    // uint32_t result = potentiometer.Read();
    // if (result < deadzone)
    //     result = 0;
    // baudrate = result * 250;

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
