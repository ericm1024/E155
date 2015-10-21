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

/* frequency constants for spi0_init */
#define SPI_122kHz 2048
#define SPI_244kHz 1024
#define SPI_488kHz 512
#define SPI_976kHz 256
#define SPI_2MHz 128
#define SPI_4MHz 64
#define SPI_8MHz 32
#define SPI_16MHz 16
#define SPI_31MHz 8
#define SPI_63MHz 4
#define SPI_125MHz 2
#define SPI_250MHz 1

/* XXX: rename to pi_pin_* */
void pi_gpio_fsel(unsigned pin, int fn);
void pi_gpio_write(unsigned pin, bool val);
bool pi_gpio_read(unsigned pin);
int pi_mem_setup();
void pi_sleep_us(unsigned us);
void error(const char *fmt, ...);

void pi_spi0_init(unsigned freq);
void pi_spi_write(unsigned char val);
unsigned char pi_spi_read();
void pi_spi_wait_tx();
void pi_spi_toggle_ce();
void pi_spi_end();
