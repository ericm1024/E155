/**
 * \file ledctl.c
 * \author Eric Mueller -- emueller@hmc.edu
 * \brief Turn on or off the LEDs for lab6. Code based off of starter code
 * from Prof. Spencer.
 *
 * \detail
 *    usage: ledctl
 *        Looks for the QUERY_STRING environment variable, and within
 *        that looks for the string out=n where n is 0 or 1.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LED_PIN (12)

int main(void)
{
        int ret;
        char *q, *end;
        long val;

        q = getenv("QUERY_STRING");
        if (!q)
                exit(1);

        if (strncmp(q, "out=", 4))
                exit(1);

        q+=4;
        val = strtol(q, &end, 10);
        if (end == q || errno)
                exit(1);

        if (val != 0 && val != 1)
                exit(1);
        
        ret = pi_mem_setup();
        if (ret)
                exit(2);

        pi_gpio_fsel(LED_PIN, GF_OUTPUT);
        pi_gpio_write(LED_PIN, val);

        /* write out html header and redirect */
	printf("%s%c%c\n", "Content-Type:text/html;charset=iso-8859-1",13,10);
	printf("<META HTTP-EQUIV=\"Refresh\" CONTENT=\"0;url=/ledcontrol.html\">");

        return 0;
}
