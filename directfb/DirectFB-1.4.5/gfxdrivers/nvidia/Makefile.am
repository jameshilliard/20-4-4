## Makefile.am for DirectFB/src/core/gfxcards/nvidia

INCLUDES = \
	-I$(top_builddir)/include	\
	-I$(top_srcdir)/include	\
	-I$(top_builddir)/lib	\
	-I$(top_srcdir)/lib	\
	-I$(top_srcdir)/src	\
	-I$(top_srcdir)/systems

nvidia_LTLIBRARIES = libdirectfb_nvidia.la

if BUILD_STATIC
nvidia_DATA = $(nvidia_LTLIBRARIES:.la=.o)
endif

nvidiadir = $(MODULEDIR)/gfxdrivers

libdirectfb_nvidia_la_SOURCES = \
	nvidia.c			\
	nvidia_state.c		\
	nvidia_2d.c		\
	nvidia_3d.c		\
	nvidia_primary.c	\
	nvidia_overlay.c	\
	nvidia.h			\
	nvidia_regs.h		\
	nvidia_accel.h		\
	nvidia_objects.h	\
	nvidia_state.h		\
	nvidia_2d.h		\
	nvidia_3d.h

libdirectfb_nvidia_la_LDFLAGS = \
	-module			\
	-avoid-version		\
	$(DFB_LDFLAGS)

libdirectfb_nvidia_la_LIBADD = -lm \
	$(top_builddir)/lib/direct/libdirect.la \
	$(top_builddir)/lib/fusion/libfusion.la \
	$(top_builddir)/src/libdirectfb.la


include $(top_srcdir)/rules/libobject.make
