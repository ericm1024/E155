/**
 * \file lab5_em.c
 * \author Eric Mueller -- emueller@hmc.edu
 * \brief Code for lab 5. Plays a song using PWM. Some code taken
 * from the lab5 starter code.
 */

#include "../lib/libpi.h"

// Pitch in Hz, duration in ms
const int notes[][2] = {
{659,	125},
{623,	125},
{659,	125},
{623,	125},
{659,	125},
{494,	125},
{587,	125},
{523,	125},
{440,	250},
{0,	125},
{262,	125},
{330,	125},
{440,	125},
{494,	250},
{0,	125},
{330,	125},
{416,	125},
{494,	125},
{523,	250},
{0,	125},
{330,	125},
{659,	125},
{623,	125},
{659,	125},
{623,	125},
{659,	125},
{494,	125},
{587,	125},
{523,	125},
{440,	250},
{0,	125},
{262,	125},
{330,	125},
{440,	125},
{494,	250},
{0,	125},
{330,	125},
{523,	125},
{494,	125},
{440,	250},
{0,	125},
{494,	125},
{523,	125},
{587,	125},
{659,	375},
{392,	125},
{699,	125},
{659,	125},
{587,	375},
{349,	125},
{659,	125},
{587,	125},
{523,	375},
{330,	125},
{587,	125},
{523,	125},
{494,	250},
{0,	125},
{330,	125},
{659,	125},
{0,	250},
{659,	125},
{1319,	125},
{0,	250},
{623,	125},
{659,	125},
{0,	250},
{623,	125},
{659,	125},
{623,	125},
{659,	125},
{623,	125},
{659,	125},
{494,	125},
{587,	125},
{523,	125},
{440,	250},
{0,	125},
{262,	125},
{330,	125},
{440,	125},
{494,	250},
{0,	125},
{330,	125},
{416,	125},
{494,	125},
{523,	250},
{0,	125},
{330,	125},
{659,	125},
{623,	125},
{659,	125},
{623,	125},
{659,	125},
{494,	125},
{587,	125},
{523,	125},
{440,	250},
{0,	125},
{262,	125},
{330,	125},
{440,	125},
{494,	250},
{0,	125},
{330,	125},
{523,	125},
{494,	125},
{440,	500},
{0,	0}};

/* our circuit has GPIO pin 12 hooked up to in+ of the LM386 */
#define LM386_GPIO_PIN 12

void play_note(unsigned freq, unsigned dur, float tempo_mult)
{
        /* we work in units of microseconds in this function */
        unsigned period = freq ? (1000*1000)/freq : 0;
        unsigned elapsed;

        dur *= 1000;
        dur *= 1.0/tempo_mult;

        /* rest */
        if (!period) {
                if (dur)
                        pi_sleep_us(dur);
                return;
        }

        /* make a square wave at freq */
        for (elapsed = 0; elapsed < dur; elapsed += period) {
                pi_gpio_write(LM386_GPIO_PIN, 1);
                pi_sleep_us(period/2);
                pi_gpio_write(LM386_GPIO_PIN, 0);
                pi_sleep_us(period/2);
        }
}

int main()
{
        unsigned i;
        int ret = pi_mem_setup();
        if (ret)
                error("mem setup failed");

        pi_gpio_fsel(LM386_GPIO_PIN, GF_OUTPUT);

        for (i = 0; i < sizeof notes/sizeof notes[0]; i++)
                play_note(notes[i][0], notes[i][1], 0.2);
}
