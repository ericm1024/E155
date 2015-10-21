HTML_DIR=/var/www/html
CGI_DIR=/usr/lib/cgi-bin

LEDCTL_TARGET=$(CGI_DIR)/ledctl
LEDCONTROL_TARGET=$(HTML_DIR)/ledcontrol.html

ledctl: ledctl.c
	gcc -g -o ledctl ledctl.c ../lib/libpi.o -std=c11 -Wall -Wextra -pedantic

install: ledctl
# 	instal the ledctl executable in
	cp ledctl $(LEDCTL_TARGET)
	chown "root:www-data" $(LEDCTL_TARGET)
	chmod 010 $(LEDCTL_TARGET)
	chmod u+s $(LEDCTL_TARGET)
#	install the ledcontrol.html file
	cp ledcontrol.html $(LEDCONTROL_TARGET)
	chown "root:root" $(LEDCONTROL_TARGET)
	chmod 444 $(LEDCONTROL_TARGET)
