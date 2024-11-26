#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

#define deadzone 0x015                               // Deadzone constant potentiometer on ADC
#define conversion_factor_V 3.3f / (1 << 12)         // conversion factor for voltage
#define conversion_factor_deg 270.0f / (1 << 12)     // conversion factor for degrees
#define conversion_factor_percent 100.0f / (1 << 12) // conversion factor for percent

// ADC class
class ADC
{
protected:
    uint channel_m; // channel 0-3 for ADC
public:
    // initializing ADC
    ADC(uint channel, bool first) : channel_m(channel)
    {
        // checking if user input incorrect channel
        if (channel_m < 4)
        {
            // initialize ADC first time
            if (first)
            {
                adc_init();
            }
            // switch on channel number to set correct GPIO for corresponding channel
            uint gpio_nr;
            switch (channel_m)
            {
            case 0:
                gpio_nr = 26;
                break;
            case 1:
                gpio_nr = 27;
                break;
            case 2:
                gpio_nr = 28;
                break;
            case 3:
                gpio_nr = 0;
                adc_set_temp_sensor_enabled(true);
                break;
            default:
                break;
            }
            if (gpio_nr)
            {
                gpio_set_dir(gpio_nr, false);
                gpio_disable_pulls(gpio_nr);
                gpio_set_input_enabled(gpio_nr, false);
            }
        }
        else
        {
            printf("Not a valid channel");
        }
    }
    ~ADC()
    {
        // disable temp sensor if it was enabled
        if (channel_m == 3)
        {
            adc_set_temp_sensor_enabled(false);
        }
    }
    // read data from ADC
    uint16_t Read()
    {
        adc_select_input(channel_m);
        return adc_read();
    }
};