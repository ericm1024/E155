/**
 * \file libpi.h
 * \brief Header for a set of library functions to  manipulate the Pi's
 * memory-mapped hardware.
 * \author Eric Mueller -- emueller@hmc.edu
 * \note Based partially off of e155 lab5 starter code.
 */

#include <stdbool.h>

/* GPIO FSEL Types */
#define GF_INPUT  0
#define GF_OUTPUT 1
#define GF_ALT0   4
#define GF_ALT1   5
#define GF_ALT2   6
#define GF_ALT3   7
#define GF_ALT4   3
#define GF_ALT5   2
#define __GF_BITS 3

#define GPIO_NR_PINS 54

/* XXX: rename to pi_pin_* */
void pi_gpio_fsel(unsigned pin, int fn);
void pi_gpio_write(unsigned pin, bool val);
bool pi_gpio_read(unsigned pin);
int pi_mem_setup();
void pi_sleep_us(unsigned us);
void error(const char *fmt, ...);
void pi_spi0_init(unsigned freq);
