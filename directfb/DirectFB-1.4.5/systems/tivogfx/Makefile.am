## Makefile.am for DirectFB/systems/tivogfx

INCLUDES = \
	-I$(top_srcdir)/include		\
	-I$(top_builddir)/include	\
	-I$(top_builddir)/lib		\
	-I$(top_srcdir)/lib		\
	-I$(top_srcdir)/src

AM_CFLAGS = $(TIVO_CFLAGS)
AM_CXXFLAGS = $(TIVO_CXXFLAGS)
AM_LDFLAGS = $(TIVO_LDFLAGS)

internalincludedir = $(INTERNALINCLUDEDIR)/tivogfx

internalinclude_HEADERS = \
	tivo_directfb_ipc_defs.h	\
	tivo_directfb_keymap.h		\
	tivogfx_init.h				\
	tivogfx_input.h				\
	tivogfx_input_hpk.h			\
	tivogfx_input_ipc.h			\
	tivogfx_input_ipc2.h		\
	tivogfx_layer.h				\
	tivogfx_proxy.h				\
	tivogfx_screen.h			\
	tivogfx_surface.h			\
	tivogfx_system.h			\
	tivogfx_user_events.h		\
	tivogfx_util.h

systemsdir = $(MODULEDIR)/systems

if BUILD_STATIC
systems_DATA = libdirectfb_tivogfx.o
endif
systems_LTLIBRARIES = libdirectfb_tivogfx.la

libdirectfb_tivogfx_la_LDFLAGS = \
	-avoid-version	\
	-module

libdirectfb_tivogfx_la_SOURCES = \
	tivo_directfb_keymap.cpp	\
	tivogfx_gfx.c			\
	tivogfx_init.cpp		\
	tivogfx_input.cpp		\
	tivogfx_input_driver.c	\
	tivogfx_input_hpk.cpp	\
	tivogfx_input_ipc.cpp	\
	tivogfx_input_ipc2.cpp	\
	tivogfx_layer.c			\
	tivogfx_proxy.cpp		\
	tivogfx_screen.c		\
	tivogfx_surface.c		\
	tivogfx_system.c		\
	tivogfx_util.c

libdirectfb_tivogfx_la_LIBADD = \
	$(top_builddir)/lib/direct/libdirect.la \
	$(top_builddir)/lib/fusion/libfusion.la \
	$(top_builddir)/src/libdirectfb.la		\
	$(TIVO_LIBS)


include $(top_srcdir)/rules/libobject.make
