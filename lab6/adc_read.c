/**
 * \file adc_read.c
 * \author Eric Mueller -- emueller@hmc.edu
 * \brief Use the Pi's SPI hardware to communicate with the MuddPi's adc
 * and read the 10 bit-value corresponding to the current light level.
 */

#include "../lib/libpi.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	int ret;
	unsigned word;

	ret = pi_mem_setup();
	if (ret)
		exit(1);

	pi_spi0_init(SPI_488kHz);

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
	pi_spi_write(0xD0000000);
	pi_spi_wait_tx();
	word = pi_spi_read();

	/*
	 * we wrote 32 bits, so we're going to read 32 bits from the same
	 * time frame. The first 4 bits are setup, the next bit is a null
	 * bit, then we have our 10 bits of data, and the rest should
	 * be zeros.
	 */
	printf("word before cleanup is 0x%x\n", word);
	assert((word & (1 << 27)) == 0);
	word = (word >> 17) & ((1 << 10) - 1);
	printf("word after cleanup is 0x%x\n", word);
	return 0;
}
