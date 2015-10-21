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
        unsigned dval;

	ret = pi_mem_setup();
	if (ret)
		exit(1);

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

        printf("Content-Type:text/plain\r\n\r\n");
        printf("digital voltage: %d\r\n", dval);
        
	return 0;
}
