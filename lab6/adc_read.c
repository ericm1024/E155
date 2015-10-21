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
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/**
 * \brief read a single 10-bit value from the adc via serial.
 * \return The value read
 * \pre Assumes the spi peripheral address range has been mapped
 */
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
#define WWW_DATA_UID 33
#define BUFFSIZE 1000

static unsigned buffer[BUFFSIZE] = {0};
static unsigned buff_idx = 0;

/**
 * \brief Dump the contents of the buffer to the output file.
 */
static void dump_buffer(int unused)
{
	unsigned count;
	unsigned idx = buff_idx + 1;
	int fd;

        (void)unused;
        
	fd = open(BUFF_FNAME, O_CREAT|O_TRUNC|O_RDWR, 00644);
	if (fd < 0) {
		printf("%s: %s\n", __func__, strerror(errno));
		exit(1);
	}

	for (count = 1; count < BUFFSIZE; ++count)
		dprintf(fd, "%d\n", buffer[idx++ % BUFFSIZE]);

	close(fd);
}

/**
 * \brief Read from the file descriptor and write out to a file in
 * tmpfs when signaled
 * \param read_fd   The read end of a pipe
 */
static void reader_loop(int read_fd)
{
	int ret;
        unsigned val;
	sighandler_t sh;

	/*
	 * drop root privs -- we need to be the www-data user so that we can
	 * get signaled
	 */
	ret = setuid(WWW_DATA_UID);
	if (ret) {
		printf("%s: %s\n", __func__, strerror(errno));
		exit(2);
	}

	/* rename ourselves so the cgi-script can find our pid by name */
	ret = prctl(PR_SET_NAME, "adc-buffer", 0, 0, 0);
	if (ret) {
		printf("%s: %s\n", __func__, strerror(errno));
		exit(3);
	}

	/* set up signal handler */
	sh = signal(SIGUSR1, dump_buffer);
	if (sh == SIG_ERR) {
		printf("%s: %s\n", __func__, strerror(errno));
		exit(3);
	}

	for (;;) {
		ret = read(read_fd, &val, sizeof val);
		assert(ret == sizeof val);
		buffer[buff_idx++ % BUFFSIZE] = val;
	}
}

/**
 * \brief Poll the adc and write the results to the pipe.
 */
static void writer_loop(int write_fd)
{
        struct timespec ts = {.tv_sec = 0, .tv_nsec = 1000*1000*10};
        unsigned val;
        int ret;

        for (;;) {
                val = adc_read();
                ret = write(write_fd, &val, sizeof val);
                assert(ret == sizeof val);
                nanosleep(&ts, NULL);
        }
}

int main()
{
	int ret, read_fd, write_fd, fds[2];
	pid_t pid;

	ret = pi_mem_setup();
	if (ret)
		exit(1);
	
	ret = pipe(fds);
        if (ret)
		exit(1);

	read_fd = fds[0];
	write_fd = fds[1];

	pid = fork();
	if (pid < 0)
		exit(1);
	else if (!pid)
		reader_loop(read_fd);
	else
		writer_loop(write_fd);

	return 0;
}
