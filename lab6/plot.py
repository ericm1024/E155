#!/usr/bin/env python

# Eric Mueller
# June 9 2014
#
# This is a program called plot. It accepts a stream of points (x, y) 
# from stdin and generates a plot using matplotlib
#
# This script was originally written to plot the frequency of seek
# distances from a filesystem trace.
#

# Torn appart and addapted in a sleep deprived rage for E155 lab6 on
# October 21 2015

import sys, os, time, matplotlib
# Tell matplotlib to use a backend that doesn't requre an X-server
matplotlib.use('Agg')
import pylab as plt

# plot the buffer
fig = plt.subplot(111)
plt.title("Digital voltage readings for some time period")
plt.xlabel("time")
plt.ylabel("digital V");

Y=[]
with open("/tmp/adc_read_buffer", 'r') as f:
    while os.path.getsize("/tmp/adc_read_buffer") != 0:
        Y.append(int(line))

X=range(len(Y))
fig.scatter(X,Y)

# http://stackoverflow.com/a/3198124/3775803
sys.stdout.write("Content-type: image/png\r\n\r\n")
plt.savefig(sys.stdout)
#plt.savefig("plot.png")
