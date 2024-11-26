#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include "ntp.h"

class RTC
{
private:
    NTPClient ntpClient;

public:
    RTC()
    {
        // Start on Friday 5th of June 2020 15:45:00
        datetime_t t = {
            .year = 2020,
            .month = 06,
            .day = 05,
            .dotw = 5, // 0 is Sunday, so 5 is Friday
            .hour = 00,
            .min = 00,
            .sec = 00};
        rtc_init();
        rtc_set_datetime(&t);
        // clk_sys is >2000x faster than clk_rtc, so datetime is not updated immediately when rtc_get_datetime() is called.
        // The delay is up to 3 RTC clock cycles (which is 64us with the default clock settings)
        sleep_us(64);
    }

    void update_rtc_time(datetime_t nt)
    {
        rtc_set_datetime(&nt);
        sleep_us(64);
    }

    datetime_t get_rtc_time()
    {
        datetime_t t;
        ntpClient.run_ntp();
        if (ntpClient.isUpdated())
            update_rtc_time(ntpClient.getUpdatedTime());
        rtc_get_datetime(&t);
        // char datetime_buf[256];
        // char *datetime_str = &datetime_buf[0];
        // datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
        // printf("\r%s\n", datetime_str);
        return t;
    }
};
