
ISM_DEPTH := ./../..
include $(ISM_DEPTH)/ismdefs

#include ../make_include
#include ../MCONFIG

LIB = ../lib

#CFLAGS=-I$(LIB) $(OPT)
LCINCS = -I$(LIB)
LCWARN =                        \
        -Wno-all                  \
        -Wno-cast-qual            \
        -Wno-conversion           \
        -Wno-implicit             \
        -Wno-missing-prototypes   \
        -Wno-strict-prototypes    \
        -Wno-undef                \
        -Wno-missing-declarations \
        -Wno-sign-compare 	  \
        -Wno-unused		  \
        -Wno-error		  \
	$(NULL)
#
# Removed because -Wno-tivo is not supported in gcc 4.2.0
#        -Wno-tivo                 \
#

default checkin: all

all: err.o my_reboot.o setproctitle.o env.o

err.o: err.c

my_reboot.o: my_reboot.c linux_reboot.h

env.o: env.h

setproctitle.o: setproctitle.h

clobber: clean

.PHONY: clean
clean:
	-rm -f *.o *~ core *.d

install:
