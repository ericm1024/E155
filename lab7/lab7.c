/**
 * \filelab7.c
 * \author Eric Mueller
 *
 * Based heavily off of starter code from bchasnov@hmc.edu, david_harris@hmc.edu
 * 15 October 2015
 *
 * Send data to encryption accelerator over SPI
 */

#include "../lib/libpi.h"

#include <stdio.h>
#include <string.h>

#define LOAD_PIN 20
#define DONE_PIN 21

/* Test Case from Appendix A.1, B */
static const char test_key_a[16] =
{0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};

static const char test_pt_a[16] =
{0x32, 0x43, 0xF6, 0xA8, 0x88, 0x5A, 0x30, 0x8D,
 0x31, 0x31, 0x98, 0xA2, 0xE0, 0x37, 0x07, 0x34};

static const char test_ct_a[16] =
{0x39, 0x25, 0x84, 0x1D, 0x02, 0xDC, 0x09, 0xFB,
 0xDC, 0x11, 0x85, 0x97, 0x19, 0x6A, 0x0B, 0x32};

/* Another test case from Appendix C.1 */
static const char test_key_b[16] =
{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

static const char test_pt_b[16] =
{0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

static const char test_ct_b[16] =
{0x69, 0xC4, 0xE0, 0xD8, 0x6A, 0x7B, 0x04, 0x30,
 0xD8, 0xCD, 0xB7, 0x80, 0x70, 0xB4, 0xC5, 0x5A};


static void print16(const char *text)
{
        int i;

        for(i = 0; i < 16; i++)
                printf("%02x ", text[i]);
        printf("\n");
}

static void printall(const char *key, const char *plaintext,
                     const char *cyphertext, const char *expected)
{
        printf("Key:        ");
        print16(key);
        printf("Plaintext:  ");
        print16(plaintext);
        printf("\n");
        printf("Ciphertext: ");
        print16(cyphertext);
        printf("Expected:   ");
        print16(expected);
}

static void encrypt(const char *key, const char *plaintext, char *cyphertext)
{
        int i;

        pi_gpio_write(LOAD_PIN, 1);

        for(i = 0; i < 16; i++)
                pi_spi_write(plaintext[i]);

        for(i = 0; i < 16; i++)
                pi_spi_write(key[i]);

        pi_gpio_write(LOAD_PIN, 0);

        while (!pi_gpio_read(DONE_PIN))
                ;

        for(i = 0; i < 16; i++) {
                pi_spi_write(0);
                cyphertext[i] = pi_spi_read();
        }
}

static void run_test(const char *key, const char *plaintext,
                     const char *expected)
{
        char cyphertext[16];

        encrypt(key, plaintext, cyphertext);
        printall(key, plaintext, cyphertext, expected);

        if (strncmp(cyphertext, expected, 16) == 0)
                printf("Success!\n");
        else {
                printf("Test faied.\n");
                printall(key, plaintext, cyphertext, expected);
        }
}

int main(void)
{
        int ret;
  
        ret = pi_mem_setup();
        if (ret)
                return 1;

        pi_spi0_init(SPI_244kHz);
        pi_gpio_fsel(LOAD_PIN, GF_OUTPUT);
        pi_gpio_fsel(DONE_PIN, GF_INPUT);

        run_test(test_key_a, test_pt_a, test_ct_a);
        run_test(test_key_b, test_pt_b, test_ct_b);

        return 0;
}

