#include "buzzer.h"

Buzzer::Buzzer(uint gpio) : pin(gpio)
{
    current_note = nullptr;
    delay_off = 0;
    whole_note_duration = 0;
    tempo = 0;
    is_done = false;
    gpio_set_function(pin, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(pin);
}

Buzzer::~Buzzer()
{
    gpio_set_function(pin, GPIO_FUNC_NULL);
}

void Buzzer::playMelody(const Melody &melody)
{
    playMelody(melody, 0);
}
void Buzzer::playMelody(const Melody &melody, uint custom_tempo = 0)
{
    current_note = melody.notes;
    tempo = (custom_tempo == 0) ? melody.tempo : custom_tempo;
    whole_note_duration = (60000 * 4) / tempo;
    delay_off = 0;
    is_done = false;
    current_alarm = add_alarm_in_us(1000, timer_note_callback_static, this, false);
    return;
}
void Buzzer::stopMelody()
{
    cancel_alarm(current_alarm);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
    current_note = nullptr;
    delay_off = 0;
    is_done = true;
}
bool Buzzer::isDone() const
{
    return is_done;
}

void Buzzer::pwm_calc_div_top(pwm_config &cfg, int frequency, int sysClock)
{
    uint count = sysClock * 16 / frequency;
    uint div = count / 60000;
    if (div < 16)
        div = 16;
    cfg.div = div;
    cfg.top = count / div;
}

uint Buzzer::playTone()
{
    pwm_config cfg = pwm_get_default_config();

    if (current_note == nullptr)
        return 0;

    int duration = current_note->duration;
    if (duration == 0)
        return 0;

    if (duration > 0)
        duration = whole_note_duration / duration;
    else
        duration = (3 * whole_note_duration / (-duration)) / 2;

    if (current_note->frequency == 0)
    {
        pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
    }
    else
    {
        pwm_calc_div_top(cfg, current_note->frequency, 125000000);
        pwm_init(slice_num, &cfg, true);
        pwm_set_chan_level(slice_num, PWM_CHAN_A, cfg.top / 2);
    }

    delay_off = duration;
    return duration;
}

int64_t Buzzer::timer_note_callback_static(alarm_id_t id, void *user_data)
{
    // Recover the Buzzer instance from the user_data pointer
    Buzzer *buzzer = static_cast<Buzzer *>(user_data);
    return buzzer->timer_note_callback(id);
}

int64_t Buzzer::timer_note_callback(alarm_id_t id)
{
    if (current_note == nullptr || current_note->duration == 0)
    {
        is_done = true;
        return 0; // Done!
    }

    if (delay_off == 0)
    {
        uint delay_on = playTone();
        if (delay_on == 0)
        {
            is_done = true;
            return 0;
        }
        delay_off = delay_on;
        return 900 * delay_on;
    }
    else
    {
        pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
        uint delay_off_temp = delay_off;
        delay_off = 0;
        current_note++;
        return 100 * delay_off_temp;
    }
}