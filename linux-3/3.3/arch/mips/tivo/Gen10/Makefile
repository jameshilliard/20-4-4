#
# Makefile for TiVo platform specific code 
# Copyright (C) 2009-2012 TiVo Inc
# 
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

obj-y	 += setup.o

ifdef CONFIG_TIVO_WATCHDOG
obj-$(CONFIG_TIVO_WATCHDOG)    += watchdog.o
endif

