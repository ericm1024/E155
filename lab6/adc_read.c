/**
 * \file adc_read.c
 * \author Eric Mueller -- emueller@hmc.edu
 * \brief Use the Pi's SPI hardware to communicate with the MuddPi's adc
 * and read the 10 bit-value corresponding to the current light level.
 */

#include "../lib/libpi.h"

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static unsigned adc_read()
{
        unsigned dval;

        pi_spi0_init(SPI_976kHz);

        /*
	 * here we prep the adc for reading. It wants to see the following
	 * bits on MOSI.
	 *     1. a single high bit signifying the start of communication
	 *     2. a bit determining if the adc is in single ended or
	 *        differential input mode. 1 for us because we are in
	 *        single-ended mode
	 *     3. a bit selecting which channel to read from. 0 for us.
	 *     4. a bit determing if the output should be MSB first. 1 for
	 *        us.
	 *
	 * Data is transmitted MSB-first, so those bits are in reverse order
	 * starting at the highest bit.
	 */
        pi_spi_write(0xD << 3);
	dval = (unsigned)pi_spi_read() << 8;
        pi_spi_write(0);
        dval |= pi_spi_read();
        pi_spi_end();

        assert((dval & 1 << 10) == 0);
        dval &= (1 << 10) - 1;

        return dval;
}

#define BUFF_FNAME "/tmp/adc_read_buffer"

static void eat_lines(int fd, unsigned nr_lines)
{
        unsigned i;
        char c;

        for (i = 0; i < nr_lines; ++i) {
                while (read(fd, &c, 1) == 0)
                        if (c == '\n')
                                break;
                printf("ate a line, om nom nom\n");
        }
}

int main()
{
	int ret, fd;
        struct timespec ts = {.tv_sec = 1, .tv_nsec = 1000*1000};

	ret = pi_mem_setup();
	if (ret)
		exit(1);
        
        ret = mknod(BUFF_FNAME, S_IFIFO|00644, 0);
        if (ret && errno != EEXIST) {
                printf("%s\n", strerror(errno));
                exit(2);
        }
                
        fd = open(BUFF_FNAME, O_RDWR);
        if (fd < 0)
                exit(3);
        
        for (;;) {
                ret = dprintf(fd, "%d\n", adc_read());

                if (ret < 0) {
                        assert(errno == EAGAIN);
                        eat_lines(fd, 20);
                }

                nanosleep(&ts, NULL);
        }
        
	return 0;
}
