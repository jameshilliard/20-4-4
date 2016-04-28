// Utility functions for l2tp
//
// This code is based in large part on rp-l2tpd, and
// carries forth its copyrights and licensing.
//
// Copyright (C) 2002 by Roaring Penguin Software Inc.
//
// This software may be distributed under the terms of the GNU General
// Public License, Version 2, or (at your option) any later version.
//
// LIC: GPL

#include "includes.h"

#define MAX_ERRMSG_LEN 512

static int random_fd = -1;
static char errmsg[MAX_ERRMSG_LEN];

struct sd_handler
{
	l2tp_shutdown_func f;
	void *data;
};

static struct sd_handler shutdown_handlers[16];

static int n_shutdown_handlers = 0;

int l2tp_register_shutdown_handler(l2tp_shutdown_func f, void *data)
{
	if (n_shutdown_handlers == 16)
	{
		return -1;
	}
	shutdown_handlers[n_shutdown_handlers].f = f;
	shutdown_handlers[n_shutdown_handlers].data = data;
	n_shutdown_handlers++;
	return n_shutdown_handlers;
}

void l2tp_cleanup(void)
{
	int i;

	for (i=0; i<n_shutdown_handlers; i++)
	{
		shutdown_handlers[i].f(shutdown_handlers[i].data);
	}
}

char const * l2tp_get_errmsg(void)
{
	return errmsg;
}

/**********************************************************************
* %FUNCTION: set_errmsg
* %ARGUMENTS:
*  fmt -- printf format
*  ... -- format args
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Sets static errmsg string
***********************************************************************/
void l2tp_set_errmsg(char const *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(errmsg, MAX_ERRMSG_LEN, fmt, ap);
	va_end(ap);
	errmsg[MAX_ERRMSG_LEN-1] = 0;
	fprintf(stderr, "Error: %s\n", errmsg);
}

/**********************************************************************
* %FUNCTION: random_init
* %ARGUMENTS:
*  None
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Sets up random-number generator
***********************************************************************/
void l2tp_random_init(void)
{
	/* Prefer /dev/urandom; fall back on rand() */
	random_fd = open("/dev/urandom", O_RDONLY);
	if (random_fd < 0)
	{
		srand(time(NULL) + getpid()*getppid());
	}
}

/**********************************************************************
* %FUNCTION: bad random_fill
* %ARGUMENTS:
*  ptr -- pointer to a buffer
*  size -- size of buffer
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Fills buffer with "size" random bytes.  This function is not
*  cryptographically strong; it's used as a fallback for systems
*  without /dev/urandom.
***********************************************************************/
static void bad_random_fill(void *ptr, size_t size)
{
	unsigned char *buf = (unsigned char *)ptr;
	while(size--)
	{
		*buf++ = rand() & 0xFF;
	}
}

/**********************************************************************
* %FUNCTION: random_fill
* %ARGUMENTS:
*  ptr -- pointer to a buffer
*  size -- size of buffer
* %RETURNS:
*  Nothing
* %DESCRIPTION:
*  Fills buffer with "size" random bytes.
***********************************************************************/
void l2tp_random_fill(void *ptr, size_t size)
{
	int n;
	int ndone = 0;
	int nleft = size;
	unsigned char *buf = (unsigned char *)ptr;

	if (random_fd < 0)
	{
		bad_random_fill(ptr, size);
		return;
	}

	while(nleft)
	{
		n = read(random_fd, buf+ndone, nleft);
		if (n <= 0)
		{
			close(random_fd);
			random_fd = -1;
			bad_random_fill(buf+ndone, nleft);
			return;
		}
		nleft -= n;
		ndone += n;
	}
}
