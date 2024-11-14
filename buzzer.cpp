#include "buzzer.h"


int64_t timer_note_callback(alarm_id_t id, void *user_data);

static inline void pwm_calcDivTop(pwm_config *c, int frequency, int sysClock)
{
    uint count = sysClock * 16 / frequency;
    uint div = count / 60000; // to be lower than 65535*15/16 (rounding error)
    if (div < 16)
        div = 16;
    c->div = div;
    c->top = count / div;
}

uint playTone(note_timer_struct *ntTimer)
{
    int duration;
    pwm_config cfg = pwm_get_default_config();

    note_struct *note = ntTimer->pt;
    duration = note->duration;
    if (duration == 0)
        return 0;
    if (duration > 0)
        duration = ntTimer->wholenote / duration;
    else
        duration = (3 * ntTimer->wholenote / (-duration)) / 2;

    if (note->frequency == 0)
        pwm_set_chan_level(ntTimer->slice_num, PWM_CHAN_A, 0);
    else
    {
        pwm_calcDivTop(&cfg, note->frequency, 125000000);
        pwm_init(ntTimer->slice_num, &cfg, true);
        pwm_set_chan_level(ntTimer->slice_num, PWM_CHAN_A, cfg.top / 2);
    }
    ntTimer->delayOFF = duration;
    return duration;
}

int64_t timer_note_callback(alarm_id_t id, void *user_data)
{
    note_timer_struct *ntTimer = (note_timer_struct *)user_data;
    note_struct *note = ntTimer->pt;
    if (note->duration == 0)
    {
        ntTimer->Done = true;
        return 0; // done!
    }
    // are we in pause time between  note
    if (ntTimer->delayOFF == 0)
    {
        uint delayON = playTone(ntTimer);
        if (delayON == 0)
        {
            ntTimer->Done = true;
            return 0;
        }
        ntTimer->delayOFF = delayON;
        return 900 * delayON;
    }
    else
    {
        pwm_set_chan_level(ntTimer->slice_num, PWM_CHAN_A, 0);
        uint delayOFF = ntTimer->delayOFF;
        ntTimer->delayOFF = 0;
        //  next note
        (ntTimer->pt)++;
        return 100 * delayOFF;
    }
}

int play_melody(note_timer_struct *ntTimer, note_struct *melody, int tempo)
{
    ntTimer->Done = false;
    ntTimer->pt = melody;
    // set tempo
    ntTimer->tempo = tempo;
    ntTimer->wholenote = (60000 * 4) / tempo;
    ntTimer->delayOFF = 0;
    // start timer
    return add_alarm_in_us(1000, timer_note_callback, ntTimer, false);
}
