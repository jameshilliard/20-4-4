#ifndef __PANICLOG_H__
#define __PANICLOG_H__

#define PL_MAX_BANNER_SIZE (256)

#define PL_SIGNATURE "PaniKyKern"

struct panic_log_header {
	char signature[12];
	unsigned int syslogsize;
	unsigned int plogsize;
	unsigned int symtabsize;
	unsigned int checksum;
	unsigned char banner[PL_MAX_BANNER_SIZE];
};

#ifdef __KERNEL__

int do_panic_log_nocli(void);

#endif

#endif
