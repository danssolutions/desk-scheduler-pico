#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "pico-ssd1306/ssd1306.h"
#include "pico-ssd1306/textRenderer/TextRenderer.h"
#include "WS2812.hpp"
#include "buzzer.h"
#include "buzzer_melodies.h"
#include "button.h"
#include "adc.h"

#define RGBLED_PIN 6
#define RGBLED_LENGTH 6
#define LED_PIN 7
#define BUZZER_PIN 20

static int scan_result(void *env, const cyw43_ev_scan_result_t *result)
{
    if (result)
    {
        printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n",
               result->ssid, result->rssi, result->channel,
               result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5],
               result->auth_mode);
    }
    return 0;
}

int main()
{
    stdio_init_all();

    // initialize wifi scan
    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    absolute_time_t scan_time = nil_time;
    bool scan_in_progress = false;

    // Initialize LED strip
    WS2812 ledStrip(
        RGBLED_PIN,        // Data line is connected to pin 0. (GP0)
        RGBLED_LENGTH,     // Strip is 6 LEDs long.
        pio0,              // Use PIO 0 for creating the state machine.
        0,                 // Index of the state machine that will be created for controlling the LED strip
                           // You can have 4 state machines per PIO-Block up to 8 overall.
                           // See Chapter 3 in: https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf
        WS2812::FORMAT_GRB // Pixel format used by the LED strip
    );

    Buzzer buzzer(BUZZER_PIN);
    
    // this turns the oldschool LED on
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_pull_up(LED_PIN);

    // initialize Button
    Button input_a(10);

    // ADC stuff
    ADC potentiometer(0, true); // initialize adc
    ADC lightsensor(1, false);
    ADC humidsensor(2, false);

    // Init i2c0 controller for display
    i2c_init(i2c_default, 500000);
    // Set up pins
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // If you don't do anything before initializing a display pi pico is too fast and starts sending
    // commands before the screen controller had time to set itself up, so we add an artificial delay for
    // ssd1306 to set itself up
    sleep_ms(250);
    // Create a new display object at address 0x3C and size of 128x64
    pico_ssd1306::SSD1306 display = pico_ssd1306::SSD1306(i2c_default, 0x3C, pico_ssd1306::Size::W128xH64);
    // Here we rotate the display by 180 degrees, so that it's not upside down from my perspective
    // If your screen is upside down try setting it to 1 or 0
    display.setOrientation(0);

    while (true)
    {
        // scan wifi
        if (absolute_time_diff_us(get_absolute_time(), scan_time) < 0)
        {
            if (!scan_in_progress)
            {
                cyw43_wifi_scan_options_t scan_options = {0};
                int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result);
                if (err == 0)
                {
                    printf("\nPerforming wifi scan\n");
                    scan_in_progress = true;
                }
                else
                {
                    printf("Failed to start scan: %d\n", err);
                    scan_time = make_timeout_time_ms(10000); // wait 10s and scan again
                }
            }
            else if (!cyw43_wifi_scan_active(&cyw43_state))
            {
                scan_time = make_timeout_time_ms(10000); // wait 10s and scan again
                scan_in_progress = false;
            }
        }

        // // ADC stuff
        // uint32_t result = potentiometer.Read(); // get adc value
        // if (result < deadzone)                  // check if in deadzone
        // {
        //     result = 0;
        // }
        // float V = result * conversion_factor_V; // convert to V

        // // converting result to displayable format
        // char charV[10];
        // snprintf(charV, sizeof charV, "%f", V);
        // display.clear();
        // for (int i = 0; i < 6; i++)
        //     drawChar(&display, font_12x16, charV[i], 0 + 12 * i, 0);
        // drawText(&display, font_12x16, "Group 7", 16, 44);
        // display.sendBuffer();
        // sleep_ms(1);

        buzzer.playMelody(DoomMelody);

        display.clear();
        drawText(&display, font_5x8, "now playing...", 0, 0);
        drawText(&display, font_16x32, "DOOM", 32, 10);
        drawText(&display, font_12x16, "Group 7", 16, 44);
        display.sendBuffer();

        ledStrip.fill(WS2812::RGB(255, 0, 0));
        ledStrip.show();

        while (!buzzer.isDone()) {
            if (input_a.is_pressed())
            {
                gpio_put(LED_PIN, true);
                buzzer.stopMelody();
                sleep_ms(10);
                gpio_put(LED_PIN, false);
                break;
            }
        }
        
        buzzer.playMelody(BreezeMelody);

        display.clear();
        drawText(&display, font_5x8, "now playing...", 0, 0);
        drawText(&display, font_5x8, "chirp up", 0, 10);
        drawText(&display, font_12x16, "Group 7", 16, 44);
        display.sendBuffer();

        ledStrip.fill(WS2812::RGB(255, 0, 255));
        ledStrip.show();

        while (!buzzer.isDone());
        
        // pause
        display.clear();
        drawText(&display, font_5x8, "now playing...", 0, 0);
        drawText(&display, font_5x8, "nothing, go to bed", 0, 10);
        drawText(&display, font_12x16, "Group 7", 16, 44);
        display.sendBuffer();

        gpio_put(LED_PIN, false);
        ledStrip.fill(WS2812::RGB(255, 0, 255));
        ledStrip.show();

        sleep_ms(1000 * 10);
    }

    cyw43_arch_deinit();
    return 0;
}