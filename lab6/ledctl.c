/**
 * \file ledctl.c
 * \author Eric Mueller -- emueller@hmc.edu
 * \brief Turn on or off the LEDs for lab6.
 *
 * \detail
 *    usage: ledctl n
 *        n must be either 0 or 1. 0 turns the led off, 1 turns it on.
 *
 *        This program must be run as root in order to maipulate the
 *        Pi's hardware peripherals.
 *
 *        An exit status of 1 denote invalid arguments, exit status of 2
 *        denots that memory setup failed (probably perms). An exit status
 *        of 0 denotes success.
 */

#include "../lib/libpi.h"

#include <errno.h>
#include <stdlib.h>

#define LED_PIN (12)

int main(int argc, char **argv)
{
        int ret;
        long val;
        char *end;

        if (argc != 2)
                exit(1);

        val = strtol(argv[1], &end, 10);
        if (end == argv[1] || errno)
                exit(1);

        if (val != 0 && val != 1)
                exit(1);
        
        ret = pi_mem_setup();
        if (ret)
                exit(2);

        pi_gpio_fsel(LED_PIN, GF_OUTPUT);
        pi_gpio_write(LED_PIN, val);

        return 0;
}
