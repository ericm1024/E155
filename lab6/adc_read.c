/**
 * \file adc_read.c
 * \author Eric Mueller -- emueller@hmc.edu
 * 
 * \brief Use the Pi's SPI hardware to communicate with the MuddPi's adc
 * and read the 10 bit-value corresponding to the current light level.
 *
 * \detail This program is meant to be run as a daemon and NOT as a CGI script.
 * TODO: doccument design/change child process bs if possible.
 */

#include "../lib/libpi.h"

/* we want sighandler_t from signal.h */
#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* for hilarious infinite loops */
#define ever ;;

/*
 * Read and return a single 10-bit value from the adc via spi.
 * This assumes the spi peripheral address range has been mapped
 */
static unsigned adc_read()
{
        unsigned dval;

        pi_spi0_init(SPI_976kHz);

        /*
	 * The ADC wants to see the following bits on MOSI:
	 *     1. a single high bit signifying the start of communication
	 *     2. a bit determining if the adc is in single ended or
	 *        differential input mode. 1 for us because we are in
	 *        single-ended mode, i.e. referencing ground.
	 *     3. a bit selecting which channel to read from. 0 for us.
	 *     4. a bit determing if the output should be MSB first. 1 for
	 *        us because it's simple.
	 *
	 * Data is transmitted MSB-first, so those bits are in reverse order
	 * starting at the highest bit, i.e. '0xD'
	 *
	 * Data is also transmitted/recieved in 1 byte chunks, (and there is
	 * no clock if we're not transmitting data) so we need to send
	 * an extra byte of all 0's in order to produce enough clocks to get
	 * all 10 bits back from the ADC.
	 */
        pi_spi_write(0xD << 3);
	dval = (unsigned)pi_spi_read() << 8;
        pi_spi_write(0);
        dval |= pi_spi_read();
        pi_spi_end();

	/* ADC always sends a null bit before it sends the 10 data bits */
        assert((dval & 1 << 10) == 0);
        dval &= (1 << 10) - 1;

        return dval;
}

#define BUFF_FNAME "/tmp/adc_read_buffer"
#define WWW_DATA_UID 33
#define BUFFSIZE 1000
#define SAMPLE_INTERVAL_MS 10

/* buffer used by child process */
static unsigned buffer[BUFFSIZE] = {0};
static unsigned buff_idx = 0;

/*
 * Dump the contents of the buffer to the output file.
 * This is called in signal context.
 */
static void dump_buffer(int unused)
{
	unsigned count;
	unsigned idx = buff_idx + 1;
	int fd;

        (void)unused;

        printf("dumping buffer...\n");
        
	fd = open(BUFF_FNAME, O_CREAT|O_TRUNC|O_RDWR, 00644);
	if (fd < 0) {
		printf("%s: %s\n", __func__, strerror(errno));
		exit(1);
	}

	for (count = 1; count < BUFFSIZE; ++count)
		dprintf(fd, "%d\n", buffer[idx++ % BUFFSIZE]);

	close(fd);
}

/* Create a pipe then fork off a child with one end of the pipe. */
int main()
{
        struct timespec ts = {.tv_sec = 0, 
			      .tv_nsec = 1000*1000*SAMPLE_INTERVAL_MS};
	sighandler_t sh;
	int ret;

	ret = pi_mem_setup();
	if (ret)
		exit(1);

	ret = setuid(WWW_DATA_UID);
	if (ret)
		exit(1);

	sh = signal(SIGUSR1, dump_buffer);
	if (sh == SIG_ERR)
		exit(1);

        for (ever) {
		buffer[buff_idx++ % BUFFSIZE] = adc_read();
                nanosleep(&ts, NULL);
        }

	return 0;
}
