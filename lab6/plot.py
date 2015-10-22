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

SAMPLE_PERIOD_MS=10

import sys, os, time, matplotlib, signal
from subprocess import check_output
# Tell matplotlib to use a backend that doesn't requre an X-server
matplotlib.use('Agg')
import pylab as plt

pid=int(check_output(["pgrep", "adc-buffer"]))
os.kill(pid, signal.SIGUSR1)

Y=[]
with open("/tmp/adc_read_buffer", 'r') as f:
    for line in f:
        Y.append(int(line))

# do digital --> real voltage conversion. conversion from MCP3002 data sheet
map(lambda x: x*5/1024.0, Y)

# plot the buffer
fig = plt.subplot(111)
plt.title("Digital voltage readings for the last %d seconds"
          % len(Y)*SAMPLE_PERIOD_MS/1000.0)
plt.xlabel("time (seconds)")
plt.ylabel("Voltage (V)");

# make range of X values
X=range(len(Y))
map(lambda x: x*SAMPLE_PERIOD_MS/1000.0), X)

fig.scatter(X,Y)

# http://stackoverflow.com/a/3198124/3775803
sys.stdout.write("Content-type: image/png\r\n\r\n")
plt.savefig(sys.stdout)
