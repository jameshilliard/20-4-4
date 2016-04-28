#ifndef _LINUX_TIVO_TAGIO_H
#define _LINUX_TIVO_TAGIO_H

/*
 * Structure and enums for TiVo tag-oriented I/O ioctl.
 */

enum tivotagio_id {
   TIVOTAGIO_END = 0,
   TIVOTAGIO_CMD = 1,
   TIVOTAGIO_IOVEC = 2,
   TIVOTAGIO_SECTOR = 3,
   TIVOTAGIO_NRSECTORS = 4,
   TIVOTAGIO_ERRBUF = 5,
   TIVOTAGIO_SCRAMBLER = 6,
   TIVOTAGIO_DEADLINE = 7,
   TIVOTAGIO_DEADLINE_MONOTONIC = 8,
   TIVOTAGIO_STREAMID = 9,
   TIVOTAGIO_NUMTAGS = 10
};

typedef struct tivotag {
   enum tivotagio_id tag;
   long long int     val;
   long int          size;
} tivotag;
   
#endif
