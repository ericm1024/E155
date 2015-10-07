/**
 * \file pi.c
 * \author Eric Mueller -- enueller@hmc.edu
 * \note based off of the E155 lab 5 starter code
 * \brief A library of functions to make writing to the PI's registers
 * easier. Written for the Raspberry Pi 2 Model B.
 */

#include "libpi.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define FSEL_OFF 0
#define SET_OFF 7
#define CLR_OFF 10
#define LEV_OFF 13
#define INPUT  0
#define OUTPUT 1

#define BCM2836_PERI_BASE        0x3F000000
#define GPIO_BASE               (BCM2836_PERI_BASE + 0x200000)
#define PAGE_SIZE               (1 << 12)

/* base of GPIO registers */
static volatile unsigned *gpio_base;

/**
 * \brief print an error line to stderr and exit.
 */
static void error(const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        exit(1);
}

static void map_gpio_mem()
{
	int fd;
	void *reg_map;

        fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd < 0)
                error("open failed: %s", strerror(errno));

	reg_map = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
                       fd, GPIO_BASE);
	if (reg_map == MAP_FAILED) {
                close(fd);
                error("mmap failed: %s", strerror(errno));
        }

	gpio_base = (volatile unsigned *)reg_map;
}

/**
 * \brief Select the function of one of the Pi's GPIO pins.
 * \param pin   The pin to select the function of. Must be in the range
 *              [0, GPIO_NR_PINS)
 * \param fn    The function to set the pin to. One of the GF_* macros.
 */
void pi_gpio_fsel(unsigned pin, int fn)
{
        unsigned offset, shift;
        unsigned fn_mask = (1 << __GF_BITS) - 1;
        
        if (pin >= GPIO_NR_PINS)
                error("%s: bad pin %d", __func__, pin);

        if (fn & ~fn_mask)
                error("%s: bad function %d", __func__, fn);

        if (!gpio_base)
                error("%s: gpio_base uninitialized", __func__);

        /* each GPFSEL register controls 10 pins */
        offset = pin/10 + FSEL_OFF;
        shift = __GF_BITS*(pin%10);
        /* first clear the bits not in fn, then set the bits in fn */
        gpio_base[offset] &= ~((~fn & fn_mask) << shift);
        gpio_base[offset] |= fn << shift;
}

/**
 * \brief Write a 0 or 1 to one of the GPIO pins.
 * \param pin   The pin to write to. Must be in the range [0, GPIO_NR_PINS)
 * \param val   The value to write.
 */
void pi_gpio_write(unsigned pin, bool val)
{
        unsigned offset;

        if (pin >= GPIO_NR_PINS)
                error("%s: bad pin %d", __func__, pin);

        if (!gpio_base)
                error("%s: gpio_base uninitialized", __func__);

        offset = pin/32 + (val ? SET_OFF : CLR_OFF);
        gpio_base[offset] = 1 << pin%32;
}

/**
 * \brief Read the value currently on a GPIO pin.
 * \param pin   The pin to read from. Must be in the range [0, GPIO_NR_PINS)
 * \return True if the pin was set, false otherwise
 */
bool pi_gpio_read(unsigned pin)
{
        unsigned offset, shift;

        if (pin >= GPIO_NR_PINS)
                error("%s: bad pin %d", __func__, pin);

        if (!gpio_base)
                error("%s: gpio_base uninitialized", __func__);

        offset = pin/32 + LEV_OFF;
        shift = pin%32;

        return !!(gpio_base[offset] & (1 << shift));
}

/**
 * \brief Map the necessary memory needed for interacting with peripherals.
 * Right now this just sets up the memory needed for GPIO, but in the
 * future it could set up more things.
 */
void pi_mem_setup()
{
        map_gpio_mem();
}
