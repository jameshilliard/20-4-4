#ifndef _LINUX_IDE_TIVO_H
#define _LINUX_IDE_TIVO_H

typedef struct FsIovec {
    void *pb;
    unsigned int cb;
} FsIovec;

#define INFINITE_DEADLINE 0x7fffffffffffffffLL

typedef struct FsIoRequest {
    unsigned long sector;
    unsigned int num_sectors;
    long long deadline;  /* in usec since Jan 1, 1970 (TmkTime / 1000) */
} FsIoRequest;

#endif
