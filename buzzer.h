#pragma once
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pwm.h"

class Note
{
public:
    uint16_t frequency;
    int16_t duration;
};

struct Melody
{
    const Note *notes;
    uint tempo;
};

class Buzzer
{
private:
    uint pin;
    uint slice_num;
    const Note *current_note;
    alarm_id_t current_alarm;
    uint delay_off;
    uint whole_note_duration;
    uint tempo;
    volatile bool is_done;

    // Static callback function for the alarm
    // Having this is necessary since add_alarm_in_us() expects a specific function signature
    // and a non-static method does not fit it
    static int64_t timer_note_callback_static(alarm_id_t id, void *user_data);

    // Non-static member function to handle the callback logic
    int64_t timer_note_callback(alarm_id_t id);

    uint playTone();

    void pwm_calc_div_top(pwm_config &cfg, int frequency, int sysClock);

public:
    Buzzer(uint gpio);
    ~Buzzer();
    void playMelody(const Melody &melody);
    void playMelody(const Melody &melody, uint custom_tempo);
    void stopMelody();
    bool isDone() const;
};