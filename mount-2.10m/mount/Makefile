
ISM_DEPTH := ./../..
include $(ISM_DEPTH)/ismdefs

#include ../make_include
#include ../MCONFIG

LIB = ../lib

#CFLAGS = -I$(LIB) $(OPT)
#WARNFLAGS = -Wall -Wstrict-prototypes -Wmissing-prototypes
#DEFINES = -DHAVE_NFS
LCINCS = -I$(LIB)
LCWARN = \
	-Wno-cast-qual            \
        -Wno-conversion           \
        -Wno-implicit             \
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

LCDEFS = -DHAVE_NFS -DTIVO

RPCSVCDIR = rpcsvc
RPC_CFLAGS = -Wno-unused 
RPCGEN = rpcgen

COMPILE = $(CC) -c $(WARNFLAGS) $(CFLAGS) $(DEFINES)
#LINK = $(CC) $(LDFLAGS)
LINK = $(CC) $(CFLAGS)

SUID_PROGS = mount umount nfsinit
NOSUID_PROGS = swapon losetup
MAN5 = fstab.5 nfs.5
MAN8 = mount.8 swapoff.8 swapon.8 umount.8 losetup.8

#ifeq "$(HAVE_PIVOT_ROOT)" "yes"
#NOSUID_PROGS := $(NOSUID_PROGS) pivot_root
#MAN8 := $(MAN8) pivot_root.8
#endif

PROGS = $(SUID_PROGS) $(NOSUID_PROGS)

# comment these out if you are not compiling in NFS support
NFS_OBJS = nfsmount.o nfsmount_xdr.o nfsmount_clnt.o
# uncomment this if you don't have libc-4.2 but do have the rpclib
GEN_FILES = nfsmount.h nfsmount_xdr.c nfsmount_clnt.c

# comment these out if you are not compiling in loop support
LO_OBJS=lomount.o

default checkin: all

all: $(PROGS)

install: $(PROGS)
	$(INSTALLDIR) $(BINDIR) $(SBINDIR)
	$(INSTALLSUID) $(SUID_PROGS) $(BINDIR)
	$(INSTALLBIN) $(NOSUID_PROGS) $(SBINDIR)
	(cd $(SBINDIR); ln -sf swapon swapoff)
	$(INSTALLDIR) $(MAN5DIR) $(MAN8DIR)
	$(INSTALLMAN) $(MAN5) $(MAN5DIR)
	$(INSTALLMAN) $(MAN8) $(MAN8DIR)

%.o: %.c
	$(COMPILE) $<

mount: mount_main.o mount.o fstab.o sundries.o realpath.o mntent.o version.o \
       mount_guess_fstype.o mount_by_label.o getusername.o \
       $(LIB)/setproctitle.o $(NFS_OBJS) $(LO_OBJS)
	$(LINK) $^ -o $@ $(LDFLAGS)

nfsinit: nfsinit.o mount.o fstab.o sundries.o realpath.o mntent.o version.o \
       mount_guess_fstype.o mount_by_label.o getusername.o \
       $(LIB)/setproctitle.o $(NFS_OBJS) $(LO_OBJS)
	$(LINK) $^ -o $@ -static $(LDFLAGS)

umount: umount.o fstab.o sundries.o realpath.o mntent.o getusername.o \
	version.o $(LO_OBJS)
	$(LINK) $^ -o $@ $(LDFLAGS)

swapon:	swapon.o version.o
	$(LINK) $^ -o $@ $(LDFLAGS)

losetup.o: lomount.c
	$(COMPILE) -DMAIN lomount.c -o losetup.o

losetup: losetup.o
	$(LINK) $^ -o $@ $(LDFLAGS)

mount.o umount.o nfsmount.o losetup.o fstab.o realpath.o sundries.o: sundries.h

mount.o umount.o fstab.o sundries.o: fstab.h

mount.o fstab.o mntent.o: mntent.h

mount.o mount_guess_fstype.o: mount_guess_fstype.h

mount.o: $(LIB)/setproctitle.h

mount.o umount.o getusername.o: getusername.h

mount.o umount.o losetup.o lomount.o: lomount.h loop.h

swapon.o: swap_constants.h swapargs.h

sundries.o nfsmount.o nfsmount_xdr.o nfsmount_clnt.o: nfsmount.h

umount.o: mount_constants.h

mount.o mount_by_label.o mount_guess_fstype.o: linux_fs.h

sundries.o realpath.o: realpath.h

nfsmount_clnt.o: nfsmount_clnt.c
	$(COMPILE) $(RPC_CFLAGS) nfsmount_clnt.c

nfsmount_xdr.o: nfsmount_xdr.c
	$(COMPILE) $(RPC_CFLAGS) nfsmount_xdr.c

ifeq "$(HAVE_GOOD_RPC)" "yes"
nfsmount.h nfsmount_xdr.c nfsmount_clnt.c: nfsmount.x
	rm -f nfsmount.h nfsmount_xdr.c nfsmount_clnt.c
	$(RPCGEN) -h -o nfsmount.h nfsmount.x
	$(RPCGEN) -c -o nfsmount_xdr.c nfsmount.x
	$(RPCGEN) -l -o nfsmount_clnt.c nfsmount.x
else
nfsmount.h nfsmount_xdr.c nfsmount_clnt.c:
	cp $(RPCSVCDIR)/nfsmount.h .
	cp $(RPCSVCDIR)/nfsmount_xdr.c .
	cp $(RPCSVCDIR)/nfsmount_clnt.c .
endif

nfsmount.o: nfs_mount4.h

swapargs.h:
	sh swap.configure

loop.h:
	sh mk_loop_h

clean:
	rm -f a.out core *~ *.o swapargs.h $(PROGS) swapoff
	rm -f loop.h nfs_mountversion.h *.d

clobber distclean realclean: clean
	rm -f $(GEN_FILES)
