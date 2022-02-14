# Name: Makefile
# Project: mpd_ext_vol
# Author: Vladimir Shulev
# Creation Date: 2016-12-28
# Tabsize: 4
# This Revision: $Id$


CC		= gcc
CFLAGS		= -Wall -D NDEBUG -O2 -march=native
LIBS		= -lmpdclient -lpthread #-static

PROGRAM = mpd_ext_vol


all: $(PROGRAM)

.c.o:
	$(CC) $(CFLAGS) -c $<

$(PROGRAM): $(PROGRAM).o
	$(CC) -o $(PROGRAM) $(PROGRAM).o $(LIBS)

strip: $(PROGRAM)
	strip $(PROGRAM)

clean:
	rm -f *.o $(PROGRAM)

#PREFIX=/usr/local
#BIN=$(PREFIX)/bin
#MAN=$(PREFIX)/man

#OPTS = -O -pedantic -Wall -DDEBUG -g
#OPTS = -D NDEBUG -O2 -march=native

#all: mpd_ext_vol

#mpd_ext_vol.o: mpd_ext_vol.c
#	gcc $(OPTS) -c mpd_ext_vol.c

#mpd_ext_vol: mpd_ext_vol.o
#	gcc $(OPTS) -o mpd_ext_vol mpd_ext_vol.o -lmpdclient -lpthread

#clean:
#	rm -f *.o
