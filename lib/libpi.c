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
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* offsets for various sets of registers */

/* GPIO: TODO RENAME ME */
#define FSEL_OFF 0
#define SET_OFF 7
#define CLR_OFF 10
#define LEV_OFF 13
#define INPUT  0
#define OUTPUT 1

/* timers */
#define TIMER_CS_OFF 0
#define TIMER_CLO_OFF 1
#define TIMER_CHI_OFF 2
#define TIMER_C1_OFF 4
#define TIMER_C2_OFF 5

/* spi0 */
#define SPI0_CS_OFF 0
#define SPI0_FIFO_OFF 1
#define SPI0_CLK_OFF 2
#define SPI0_DLEN_OFF 3
#define SPI0_LTOH_OFF 4
#define SPI0_DC_OFF 5

/* peripheral addresses */
#define BCM2836_PERI_BASE       0x3F000000
#define GPIO_BASE               (BCM2836_PERI_BASE + 0x200000)
#define TIMER_BASE              (BCM2836_PERI_BASE + 0x3000)
#define SPI0_BASE               (BCM2836_PERI_BASE + 0x204000)
#define PAGE_SIZE               (1 << 12)

/* base addresses of various register sets */
static volatile unsigned *gpio_base;
static volatile unsigned *timer_base;
static volatile unsigned *spi0_base;

/**
 * \brief print an error line to stderr and exit.
 */
void error(const char *fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        exit(1);
}

static int map_phys_mem(unsigned offset, unsigned len,
                        volatile unsigned **output)
{
	int fd;
	void *reg_map;

        fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd < 0) 
                return errno;

	reg_map = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset);
        close(fd);
	if (reg_map == MAP_FAILED)
                return errno;

	*output = reg_map;
        return 0;
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
int pi_mem_setup()
{
        int ret;

        ret = map_phys_mem(GPIO_BASE, PAGE_SIZE, &gpio_base);
        if (ret)
                goto out;

        ret = map_phys_mem(TIMER_BASE, PAGE_SIZE, &timer_base);
        if (ret)
                goto out_timer;

        ret = map_phys_mem(SPI0_BASE, PAGE_SIZE, &spi0_base);
        if (ret)
                goto out_spi;

        return 0;

out_spi:
        munmap((void*)timer_base, PAGE_SIZE);
out_timer:
        munmap((void*)gpio_base, PAGE_SIZE);
out:
        error("%s: %s", __func__, strerror(ret));
        return ret;
}

void pi_sleep_us(unsigned us)
{
        timer_base[TIMER_C1_OFF] = timer_base[TIMER_CLO_OFF] + us;
        /* we use timer 1 */
        timer_base[TIMER_CS_OFF] |= 1 << 1;
        for (;;)
                if (timer_base[TIMER_CS_OFF] & (1 << 1))
                        break;
}

/**
 * \brief set up SPI with default settings.
 * \param freq   Must be one of the SPI_<freq> macros as defined in
 *               libpi.h
 */
void pi_spi0_init(unsigned freq)
{
        /* spi0 uses pins 8 to 11 */
        pi_gpio_fsel(8, GF_ALT0);
        pi_gpio_fsel(9, GF_ALT0);
        pi_gpio_fsel(10, GF_ALT0);
        pi_gpio_fsel(11, GF_ALT0);

        spi0_base[SPI0_CS_OFF] = 0;
        spi0_base[SPI0_CLK_OFF] = freq;
}

/* write a byte out to SPI */
void pi_spi_write(unsigned char val)
{
	spi0_base[SPI0_FIFO_OFF] = val;

        /* spin on DONE bit in CS register */
	while (!(spi0_base[SPI0_CS_OFF] & (1 << 16)))
                ;
}

/* read a byte out of the FIFO read queue */
unsigned char pi_spi_read()
{
	return spi0_base[SPI0_FIFO_OFF];
}

unsigned char pi_spi_rw(unsigned char wval)
{
        pi_spi_write(wval);
        return pi_spi_read();
}

/* 
 * set the TA bit and block all signals. BEWARE this MUST be paird with
 * a pi_spi_end() if you are doing any kind of signal handling
 */
void pi_spi_begin()
{
        sigset_t ss;
        sigfillset(&ss);
        sigprocmask(SIG_BLOCK, &ss, NULL);
        spi0_base[SPI0_CS_OFF] |= 1 << 7;
}

/* 
 * clear the transfer active bit and unblock all signals. This should be paired
 * with a pi_spi_begin()
 */
void pi_spi_end()
{
        sigset_t ss;
        sigfillset(&ss);
        sigprocmask(SIG_UNBLOCK, &ss, NULL);
        spi0_base[SPI0_CS_OFF] &= ~(1 << 7);
}
