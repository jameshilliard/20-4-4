/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader 2.13.  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have the getmntent function.  */
//#define HAVE_GETMNTENT 1

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* #undef WORDS_BIGENDIAN */

/* This symbol will be declared if we're compiling for an Irix system,
   and we have access to the Irix cdaudio library to do the dirty
   work.  */
/* #undef IRIX_CDAUDIO */

/* This symbol will be defined if we are compiling from a BeOS system.
   In that case, we will use closesocket() instead of close(), and we
   won't use shutdown() at all.  */
/* #undef BEOS_CDAUDIO */

/* These are quick fixes for Solaris.  */
/* #undef SOLARIS_GETMNTENT */
/* #undef BROKEN_SOLARIS_LEADOUT */

/* Define if your system provides POSIX.4 threads.  */
/* #undef HAVE_PTHREAD */

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* Define if you have the gethostbyname function.  */
#define HAVE_GETHOSTBYNAME 1

/* Define if you have the gethostbyname_r function.  */
#define HAVE_GETHOSTBYNAME_R 1

/* Define if you have the getmntinfo function.  */
/* #undef HAVE_GETMNTINFO */

/* Define if you have the mkdir function.  */
#define HAVE_MKDIR 1

/* Define if you have the snprintf function.  */
#define HAVE_SNPRINTF 1

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strstr function.  */
#define HAVE_STRSTR 1

/* Define if you have the strtol function.  */
#define HAVE_STRTOL 1

/* Define if you have the strtoul function.  */
#define HAVE_STRTOUL 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <dlfcn.h> header file.  */
#define HAVE_DLFCN_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <io/cam/cdrom.h> header file.  */
/* #undef HAVE_IO_CAM_CDROM_H */

/* Define if you have the <linux/cdrom.h> header file.  */
#define HAVE_LINUX_CDROM_H 1

/* Define if you have the <linux/ucdrom.h> header file.  */
/* #undef HAVE_LINUX_UCDROM_H */

/* Define if you have the <mntent.h> header file.  */
#define HAVE_MNTENT_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <string.h> header file.  */
/* #undef HAVE_STRING_H */

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/cdio.h> header file.  */
/* #undef HAVE_SYS_CDIO_H */

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/mntent.h> header file.  */
/* #undef HAVE_SYS_MNTENT_H */

/* Define if you have the <sys/mount.h> header file.  */
#define HAVE_SYS_MOUNT_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/ucred.h> header file.  */
/* #undef HAVE_SYS_UCRED_H */

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Name of package */
#define PACKAGE "libcdaudio"

/* Version number of package */
#define VERSION "0.99.6"

/* Define if compiler has function prototypes */
#define PROTOTYPES 1

