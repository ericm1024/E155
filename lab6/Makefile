CC=gcc
CFLAGS=-g -std=c11 -Wall -Wextra -pedantic
LIBPI=../lib/libpi.o

HTML_DIR=/var/www/html
CGI_DIR=/usr/lib/cgi-bin

LEDCTL_TARGET=$(CGI_DIR)/ledctl
LEDCONTROL_TARGET=$(HTML_DIR)/ledcontrol.html
PLOT_TARGET=$(CGI_DIR)/plot.py

all: adc_read ledctl

ledctl: ledctl.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBPI)

adc_read: adc_read.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBPI)

clean:
	rm -f adc_read ledctl

install:
# 	instal the ledctl executable in
	cp ledctl $(LEDCTL_TARGET)
	chown "root:www-data" $(LEDCTL_TARGET)
	chmod 010 $(LEDCTL_TARGET)
	chmod u+s $(LEDCTL_TARGET)
#	install plot.py executable
	cp plot.py $(PLOT_TARGET)
	chown "root:www-data" $(PLOT_TARGET)
	chmod 555 $(PLOT_TARGET)
#	install the ledcontrol.html file
	cp ledcontrol.html $(LEDCONTROL_TARGET)
	chown "root:root" $(LEDCONTROL_TARGET)
	chmod 444 $(LEDCONTROL_TARGET)
