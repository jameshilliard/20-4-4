#ifndef _LINUX_IDE_DEADSYSTEM_H
#define _LINUX_IDE_DEADSYSTEM_H

#include <linux/types.h>

extern int pio_write_buffer_to_swap(const u8 *buf, unsigned int size,
				    int secoffset, u32 *csum);

extern int pio_write_corefile(int offset);

extern void get_syslog_data(char *syslog_data[4]);

#endif
