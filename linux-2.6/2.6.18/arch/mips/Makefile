#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994, 95, 96, 2003 by Ralf Baechle
# DECStation modifications by Paul M. Antoine, 1996
# Copyright (C) 2002, 2003, 2004  Maciej W. Rozycki
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" cleaning up for this architecture.
#

cflags-y :=

#
# Select the object file format to substitute into the linker script.
#
ifdef CONFIG_CPU_LITTLE_ENDIAN
32bit-tool-prefix	= mipsel-linux-
64bit-tool-prefix	= mips64el-linux-
32bit-bfd		= elf32-tradlittlemips
64bit-bfd		= elf64-tradlittlemips
32bit-emul		= elf32ltsmip
64bit-emul		= elf64ltsmip
else
32bit-tool-prefix	= $(TOOLROOT)/bin/mips-TiVo-linux-gnu-
64bit-tool-prefix	= $(TOOLROOT)/bin/mips-TiVo-linux-gnu-
32bit-bfd		= elf32-tradbigmips
64bit-bfd		= elf64-tradbigmips
32bit-emul		= elf32btsmip
64bit-emul		= elf64btsmip
endif

ifdef CONFIG_32BIT
tool-prefix		= $(32bit-tool-prefix)
UTS_MACHINE		:= mips
endif
ifdef CONFIG_64BIT
tool-prefix		= $(64bit-tool-prefix)
UTS_MACHINE		:= mips64
endif

ifdef CONFIG_CROSSCOMPILE
CROSS_COMPILE		:= $(tool-prefix)
endif

ifdef CONFIG_32BIT
ld-emul			= $(32bit-emul)
vmlinux-32		= vmlinux
vmlinux-64		= vmlinux.64

cflags-y		+= -mabi=32
endif

ifdef CONFIG_64BIT
ld-emul			= $(64bit-emul)
vmlinux-32		= vmlinux.32
vmlinux-64		= vmlinux

cflags-y		+= -mabi=64
ifdef CONFIG_BUILD_ELF64
cflags-y		+= $(call cc-option,-mno-explicit-relocs)
else
# -msym32 can not be used for modules since they are loaded into XKSEG
CFLAGS_MODULE		+= $(call cc-option,-mno-explicit-relocs)
CFLAGS_KERNEL		+= $(call cc-option,-msym32)
endif
endif


#
# GCC uses -G 0 -mabicalls -fpic as default.  We don't want PIC in the kernel
# code since it only slows down the whole thing.  At some point we might make
# use of global pointer optimizations but their use of $28 conflicts with
# the current pointer optimization.
#
# The DECStation requires an ECOFF kernel for remote booting, other MIPS
# machines may also.  Since BFD is incredibly buggy with respect to
# crossformat linking we rely on the elf2ecoff tool for format conversion.
#
cflags-y			+= -G 0 -mno-abicalls -fno-pic -pipe
cflags-y			+= -msoft-float
LDFLAGS_vmlinux			+= -G 0 -static -n -nostdlib
MODFLAGS			+= -mlong-calls

cflags-y += -ffreestanding

#
# We explicitly add the endianness specifier if needed, this allows
# to compile kernels with a toolchain for the other endianness. We
# carefully avoid to add it redundantly because gcc 3.3/3.4 complains
# when fed the toolchain default!
#
# Certain gcc versions upto gcc 4.1.1 (probably 4.2-subversion as of
# 2006-10-10 don't properly change the the predefined symbols if -EB / -EL
# are used, so we kludge that here.  A bug has been filed at
# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=29413.
#
undef-all += -UMIPSEB -U_MIPSEB -U__MIPSEB -U__MIPSEB__
undef-all += -UMIPSEL -U_MIPSEL -U__MIPSEL -U__MIPSEL__
predef-be += -DMIPSEB -D_MIPSEB -D__MIPSEB -D__MIPSEB__
predef-le += -DMIPSEL -D_MIPSEL -D__MIPSEL -D__MIPSEL__
cflags-$(CONFIG_CPU_BIG_ENDIAN)		+= $(shell $(CC) -dumpmachine |grep -q 'mips.*el-.*' && echo -EB $(undef-all) $(predef-be))
cflags-$(CONFIG_CPU_LITTLE_ENDIAN)	+= $(shell $(CC) -dumpmachine |grep -q 'mips.*el-.*' || echo -EL $(undef-all) $(predef-le))

cflags-$(CONFIG_SB1XXX_CORELIS)	+= $(call cc-option,-mno-sched-prolog) \
				   -fno-omit-frame-pointer

#
# CPU-dependent compiler/assembler options for optimization.
#
cflags-$(CONFIG_CPU_R3000)	+= -march=r3000
cflags-$(CONFIG_CPU_TX39XX)	+= -march=r3900
cflags-$(CONFIG_CPU_R6000)	+= -march=r6000 -Wa,--trap
cflags-$(CONFIG_CPU_R4300)	+= -march=r4300 -Wa,--trap
cflags-$(CONFIG_CPU_VR41XX)	+= -march=r4100 -Wa,--trap
cflags-$(CONFIG_CPU_R4X00)	+= -march=r4600 -Wa,--trap
cflags-$(CONFIG_CPU_TX49XX)	+= -march=r4600 -Wa,--trap
cflags-$(CONFIG_CPU_MIPS32_R1)	+= $(call cc-option,-march=mips32,-mips32 -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS32) \
			-Wa,-mips32 -Wa,--trap -mtivo-check-ra
cflags-$(CONFIG_CPU_MIPS32_R2)	+= $(call cc-option,-march=mips32r2,-mips32r2 -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS32) \
			-Wa,-mips32r2 -Wa,--trap
cflags-$(CONFIG_CPU_MIPS64_R1)	+= $(call cc-option,-march=mips64,-mips64 -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS64) \
			-Wa,-mips64 -Wa,--trap
cflags-$(CONFIG_CPU_MIPS64_R2)	+= $(call cc-option,-march=mips64r2,-mips64r2 -U_MIPS_ISA -D_MIPS_ISA=_MIPS_ISA_MIPS64) \
			-Wa,-mips64r2 -Wa,--trap
cflags-$(CONFIG_CPU_R5000)	+= -march=r5000 -Wa,--trap
cflags-$(CONFIG_CPU_R5432)	+= $(call cc-option,-march=r5400,-march=r5000) \
			-Wa,--trap
cflags-$(CONFIG_CPU_NEVADA)	+= $(call cc-option,-march=rm5200,-march=r5000) \
			-Wa,--trap
cflags-$(CONFIG_CPU_RM7000)	+= $(call cc-option,-march=rm7000,-march=r5000) \
			-Wa,--trap
cflags-$(CONFIG_CPU_RM9000)	+= $(call cc-option,-march=rm9000,-march=r5000) \
			-Wa,--trap
cflags-$(CONFIG_CPU_SB1)	+= $(call cc-option,-march=sb1,-march=r5000) \
			-Wa,--trap
cflags-$(CONFIG_CPU_R8000)	+= -march=r8000 -Wa,--trap
cflags-$(CONFIG_CPU_R10000)	+= $(call cc-option,-march=r10000,-march=r8000) \
			-Wa,--trap

ifndef CONFIG_WARN_UNINITIALIZED
cflags-y			+= -Wno-uninitialized
endif

ifdef CONFIG_CPU_SB1
ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
MODFLAGS	+= -msb1-pass1-workarounds
endif
endif

#
# Firmware support
#
libs-$(CONFIG_ARC)		+= arch/mips/arc/
libs-$(CONFIG_SIBYTE_CFE)	+= arch/mips/sibyte/cfe/

#
# Board-dependent options and extra files
#

#
# Acer PICA 61, Mips Magnum 4000 and Olivetti M700.
#
core-$(CONFIG_MACH_JAZZ)	+= arch/mips/jazz/
cflags-$(CONFIG_MACH_JAZZ)	+= -Iinclude/asm-mips/mach-jazz
load-$(CONFIG_MACH_JAZZ)	+= 0xffffffff80080000

#
# Common Alchemy Au1x00 stuff
#
core-$(CONFIG_SOC_AU1X00)	+= arch/mips/au1000/common/
cflags-$(CONFIG_SOC_AU1X00)	+= -Iinclude/asm-mips/mach-au1x00

#
# AMD Alchemy Pb1000 eval board
#
libs-$(CONFIG_MIPS_PB1000)	+= arch/mips/au1000/pb1000/
cflags-$(CONFIG_MIPS_PB1000)	+= -Iinclude/asm-mips/mach-pb1x00
load-$(CONFIG_MIPS_PB1000)	+= 0xffffffff80100000

#
# AMD Alchemy Pb1100 eval board
#
libs-$(CONFIG_MIPS_PB1100)	+= arch/mips/au1000/pb1100/
cflags-$(CONFIG_MIPS_PB1100)	+= -Iinclude/asm-mips/mach-pb1x00
load-$(CONFIG_MIPS_PB1100)	+= 0xffffffff80100000

#
# AMD Alchemy Pb1500 eval board
#
libs-$(CONFIG_MIPS_PB1500)	+= arch/mips/au1000/pb1500/
cflags-$(CONFIG_MIPS_PB1500)	+= -Iinclude/asm-mips/mach-pb1x00
load-$(CONFIG_MIPS_PB1500)	+= 0xffffffff80100000

#
# AMD Alchemy Pb1550 eval board
#
libs-$(CONFIG_MIPS_PB1550)	+= arch/mips/au1000/pb1550/
cflags-$(CONFIG_MIPS_PB1550)	+= -Iinclude/asm-mips/mach-pb1x00
load-$(CONFIG_MIPS_PB1550)	+= 0xffffffff80100000

#
# AMD Alchemy Pb1200 eval board
#
libs-$(CONFIG_MIPS_PB1200)	+= arch/mips/au1000/pb1200/
cflags-$(CONFIG_MIPS_PB1200)	+= -Iinclude/asm-mips/mach-pb1x00
load-$(CONFIG_MIPS_PB1200)	+= 0xffffffff80100000

#
# AMD Alchemy Db1000 eval board
#
libs-$(CONFIG_MIPS_DB1000)	+= arch/mips/au1000/db1x00/
cflags-$(CONFIG_MIPS_DB1000)	+= -Iinclude/asm-mips/mach-db1x00
load-$(CONFIG_MIPS_DB1000)	+= 0xffffffff80100000

#
# AMD Alchemy Db1100 eval board
#
libs-$(CONFIG_MIPS_DB1100)	+= arch/mips/au1000/db1x00/
cflags-$(CONFIG_MIPS_DB1100)	+= -Iinclude/asm-mips/mach-db1x00
load-$(CONFIG_MIPS_DB1100)	+= 0xffffffff80100000

#
# AMD Alchemy Db1500 eval board
#
libs-$(CONFIG_MIPS_DB1500)	+= arch/mips/au1000/db1x00/
cflags-$(CONFIG_MIPS_DB1500)	+= -Iinclude/asm-mips/mach-db1x00
load-$(CONFIG_MIPS_DB1500)	+= 0xffffffff80100000

#
# AMD Alchemy Db1550 eval board
#
libs-$(CONFIG_MIPS_DB1550)	+= arch/mips/au1000/db1x00/
cflags-$(CONFIG_MIPS_DB1550)	+= -Iinclude/asm-mips/mach-db1x00
load-$(CONFIG_MIPS_DB1550)	+= 0xffffffff80100000

#
# AMD Alchemy Db1200 eval board
#
libs-$(CONFIG_MIPS_DB1200)	+= arch/mips/au1000/pb1200/
cflags-$(CONFIG_MIPS_DB1200)	+= -Iinclude/asm-mips/mach-db1x00
load-$(CONFIG_MIPS_DB1200)	+= 0xffffffff80100000

#
# AMD Alchemy Bosporus eval board
#
libs-$(CONFIG_MIPS_BOSPORUS)	+= arch/mips/au1000/db1x00/
cflags-$(CONFIG_MIPS_BOSPORUS)	+= -Iinclude/asm-mips/mach-db1x00
load-$(CONFIG_MIPS_BOSPORUS)	+= 0xffffffff80100000

#
# AMD Alchemy Mirage eval board
#
libs-$(CONFIG_MIPS_MIRAGE)	+= arch/mips/au1000/db1x00/
cflags-$(CONFIG_MIPS_MIRAGE)	+= -Iinclude/asm-mips/mach-db1x00
load-$(CONFIG_MIPS_MIRAGE)	+= 0xffffffff80100000

#
# 4G-Systems eval board
#
libs-$(CONFIG_MIPS_MTX1)	+= arch/mips/au1000/mtx-1/
load-$(CONFIG_MIPS_MTX1)	+= 0xffffffff80100000

#
# MyCable eval board
#
libs-$(CONFIG_MIPS_XXS1500)	+= arch/mips/au1000/xxs1500/
load-$(CONFIG_MIPS_XXS1500)	+= 0xffffffff80100000

#
# Cobalt Server
#
core-$(CONFIG_MIPS_COBALT)	+= arch/mips/cobalt/
cflags-$(CONFIG_MIPS_COBALT)	+= -Iinclude/asm-mips/mach-cobalt
load-$(CONFIG_MIPS_COBALT)	+= 0xffffffff80080000

#
# DECstation family
#
core-$(CONFIG_MACH_DECSTATION)	+= arch/mips/dec/
cflags-$(CONFIG_MACH_DECSTATION)+= -Iinclude/asm-mips/mach-dec
libs-$(CONFIG_MACH_DECSTATION)	+= arch/mips/dec/prom/
load-$(CONFIG_MACH_DECSTATION)	+= 0xffffffff80040000
CLEAN_FILES			+= drivers/tc/lk201-map.c

#
# Galileo EV64120 Board
#
core-$(CONFIG_MIPS_EV64120)	+= arch/mips/gt64120/ev64120/
core-$(CONFIG_MIPS_EV64120)	+= arch/mips/gt64120/common/
cflags-$(CONFIG_MIPS_EV64120)	+= -Iinclude/asm-mips/mach-ev64120
load-$(CONFIG_MIPS_EV64120)	+= 0xffffffff80100000

#
# Galileo EV96100 Board
#
core-$(CONFIG_MIPS_EV96100)	+= arch/mips/galileo-boards/ev96100/
cflags-$(CONFIG_MIPS_EV96100)	+= -Iinclude/asm-mips/mach-ev96100
load-$(CONFIG_MIPS_EV96100)	+= 0xffffffff80100000

#
# Wind River PPMC Board (4KC + GT64120)
#
core-$(CONFIG_WR_PPMC)		+= arch/mips/gt64120/wrppmc/
cflags-$(CONFIG_WR_PPMC)		+= -Iinclude/asm-mips/mach-wrppmc
load-$(CONFIG_WR_PPMC)		+= 0xffffffff80100000

#
# Globespan IVR eval board with QED 5231 CPU
#
core-$(CONFIG_ITE_BOARD_GEN)	+= arch/mips/ite-boards/generic/
core-$(CONFIG_MIPS_IVR)		+= arch/mips/ite-boards/ivr/
load-$(CONFIG_MIPS_IVR)		+= 0xffffffff80100000

#
# ITE 8172 eval board with QED 5231 CPU
#
core-$(CONFIG_MIPS_ITE8172)	+= arch/mips/ite-boards/qed-4n-s01b/
load-$(CONFIG_MIPS_ITE8172)	+= 0xffffffff80100000

#
# For all MIPS, Inc. eval boards
#
core-$(CONFIG_MIPS_BOARDS_GEN)	+= arch/mips/mips-boards/generic/

#
# MIPS Atlas board
#
core-$(CONFIG_MIPS_ATLAS)	+= arch/mips/mips-boards/atlas/
cflags-$(CONFIG_MIPS_ATLAS)	+= -Iinclude/asm-mips/mach-atlas
cflags-$(CONFIG_MIPS_ATLAS)	+= -Iinclude/asm-mips/mach-mips
load-$(CONFIG_MIPS_ATLAS)	+= 0xffffffff80100000

#
# MIPS Malta board
#
core-$(CONFIG_MIPS_MALTA)	+= arch/mips/mips-boards/malta/
cflags-$(CONFIG_MIPS_MALTA)	+= -Iinclude/asm-mips/mach-mips
load-$(CONFIG_MIPS_MALTA)	+= 0xffffffff80100000

#
# MIPS SEAD board
#
core-$(CONFIG_MIPS_SEAD)	+= arch/mips/mips-boards/sead/
cflags-$(CONFIG_MIPS_SEAD)	+= -Iinclude/asm-mips/mach-mips
load-$(CONFIG_MIPS_SEAD)	+= 0xffffffff80100000

#
# MIPS SIM
#
core-$(CONFIG_MIPS_SIM)		+= arch/mips/mips-boards/sim/
cflags-$(CONFIG_MIPS_SIM)	+= -Iinclude/asm-mips/mach-sim
load-$(CONFIG_MIPS_SIM)		+= 0x80100000

#
# Momentum Ocelot board
#
# The Ocelot setup.o must be linked early - it does the ioremap() for the
# mips_io_port_base.
#
core-$(CONFIG_MOMENCO_OCELOT)	+= arch/mips/gt64120/common/ \
				   arch/mips/gt64120/momenco_ocelot/
cflags-$(CONFIG_MOMENCO_OCELOT)	+= -Iinclude/asm-mips/mach-ocelot
load-$(CONFIG_MOMENCO_OCELOT)	+= 0xffffffff80100000

#
# Momentum Ocelot-G board
#
# The Ocelot-G setup.o must be linked early - it does the ioremap() for the
# mips_io_port_base.
#
core-$(CONFIG_MOMENCO_OCELOT_G)	+= arch/mips/momentum/ocelot_g/
load-$(CONFIG_MOMENCO_OCELOT_G)	+= 0xffffffff80100000

#
# Momentum Ocelot-C and -CS boards
#
# The Ocelot-C[S] setup.o must be linked early - it does the ioremap() for the
# mips_io_port_base.
core-$(CONFIG_MOMENCO_OCELOT_C)	+= arch/mips/momentum/ocelot_c/
load-$(CONFIG_MOMENCO_OCELOT_C)	+= 0xffffffff80100000

#
# PMC-Sierra Yosemite
#
core-$(CONFIG_PMC_YOSEMITE)	+= arch/mips/pmc-sierra/yosemite/
cflags-$(CONFIG_PMC_YOSEMITE)	+= -Iinclude/asm-mips/mach-yosemite
load-$(CONFIG_PMC_YOSEMITE)	+= 0xffffffff80100000

#
# Qemu simulating MIPS32 4Kc
#
core-$(CONFIG_QEMU)		+= arch/mips/qemu/
cflags-$(CONFIG_QEMU)		+= -Iinclude/asm-mips/mach-qemu
load-$(CONFIG_QEMU)		+= 0xffffffff80010000

#
# Momentum Ocelot-3
#
core-$(CONFIG_MOMENCO_OCELOT_3) 	+= arch/mips/momentum/ocelot_3/
cflags-$(CONFIG_MOMENCO_OCELOT_3)	+= -Iinclude/asm-mips/mach-ocelot3
load-$(CONFIG_MOMENCO_OCELOT_3) 	+= 0xffffffff80100000

#
# Basler eXcite
#
core-$(CONFIG_BASLER_EXCITE)	+= arch/mips/basler/excite/
cflags-$(CONFIG_BASLER_EXCITE)	+= -Iinclude/asm-mips/mach-excite
load-$(CONFIG_BASLER_EXCITE)	+= 0x80100000

#
# Momentum Jaguar ATX
#
core-$(CONFIG_MOMENCO_JAGUAR_ATX)	+= arch/mips/momentum/jaguar_atx/
cflags-$(CONFIG_MOMENCO_JAGUAR_ATX)	+= -Iinclude/asm-mips/mach-ja
#ifdef CONFIG_JAGUAR_DMALOW
#load-$(CONFIG_MOMENCO_JAGUAR_ATX)	+= 0xffffffff88000000
#else
load-$(CONFIG_MOMENCO_JAGUAR_ATX)	+= 0xffffffff80100000
#endif

#
# NEC DDB
#
core-$(CONFIG_DDB5XXX_COMMON)	+= arch/mips/ddb5xxx/common/

#
# NEC DDB Vrc-5477
#
core-$(CONFIG_DDB5477)		+= arch/mips/ddb5xxx/ddb5477/
load-$(CONFIG_DDB5477)		+= 0xffffffff80100000

core-$(CONFIG_LASAT)		+= arch/mips/lasat/
cflags-$(CONFIG_LASAT)		+= -Iinclude/asm-mips/mach-lasat
load-$(CONFIG_LASAT)		+= 0xffffffff80000000

#
# Common VR41xx
#
core-$(CONFIG_MACH_VR41XX)	+= arch/mips/vr41xx/common/
cflags-$(CONFIG_MACH_VR41XX)	+= -Iinclude/asm-mips/mach-vr41xx

#
# NEC VR4133
#
core-$(CONFIG_NEC_CMBVR4133)	+= arch/mips/vr41xx/nec-cmbvr4133/
load-$(CONFIG_NEC_CMBVR4133)	+= 0xffffffff80100000

#
# ZAO Networks Capcella (VR4131)
#
load-$(CONFIG_ZAO_CAPCELLA)	+= 0xffffffff80000000

#
# Victor MP-C303/304 (VR4122)
#
load-$(CONFIG_VICTOR_MPC30X)	+= 0xffffffff80001000

#
# IBM WorkPad z50 (VR4121)
#
core-$(CONFIG_IBM_WORKPAD)	+= arch/mips/vr41xx/ibm-workpad/
load-$(CONFIG_IBM_WORKPAD)	+= 0xffffffff80004000

#
# CASIO CASSIPEIA E-55/65 (VR4111)
#
core-$(CONFIG_CASIO_E55)	+= arch/mips/vr41xx/casio-e55/
load-$(CONFIG_CASIO_E55)	+= 0xffffffff80004000

#
# TANBAC VR4131 multichip module(TB0225) and TANBAC VR4131DIMM(TB0229) (VR4131)
#
load-$(CONFIG_TANBAC_TB022X)	+= 0xffffffff80000000

#
# Common Philips PNX8550
#
core-$(CONFIG_SOC_PNX8550)	+= arch/mips/philips/pnx8550/common/
cflags-$(CONFIG_SOC_PNX8550)	+= -Iinclude/asm-mips/mach-pnx8550

#
# Philips PNX8550 JBS board
#
libs-$(CONFIG_PNX8550_JBS)	+= arch/mips/philips/pnx8550/jbs/
#cflags-$(CONFIG_PNX8550_JBS)	+= -Iinclude/asm-mips/mach-pnx8550
load-$(CONFIG_PNX8550_JBS)	+= 0xffffffff80060000

# NEC EMMA2RH boards
#
core-$(CONFIG_EMMA2RH)          += arch/mips/emma2rh/common/
cflags-$(CONFIG_EMMA2RH)        += -Iinclude/asm-mips/mach-emma2rh

# NEC EMMA2RH Mark-eins
core-$(CONFIG_MARKEINS)         += arch/mips/emma2rh/markeins/
load-$(CONFIG_MARKEINS)         += 0xffffffff88100000

#
# SGI IP22 (Indy/Indigo2)
#
# Set the load address to >= 0xffffffff88069000 if you want to leave space for
# symmon, 0xffffffff80002000 for production kernels.  Note that the value must
# be aligned to a multiple of the kernel stack size or the handling of the
# current variable will break so for 64-bit kernels we have to raise the start
# address by 8kb.
#
core-$(CONFIG_SGI_IP22)		+= arch/mips/sgi-ip22/
cflags-$(CONFIG_SGI_IP22)	+= -Iinclude/asm-mips/mach-ip22
ifdef CONFIG_32BIT
load-$(CONFIG_SGI_IP22)		+= 0xffffffff88002000
endif
ifdef CONFIG_64BIT
load-$(CONFIG_SGI_IP22)		+= 0xffffffff88004000
endif

#
# SGI-IP27 (Origin200/2000)
#
# Set the load address to >= 0xc000000000300000 if you want to leave space for
# symmon, 0xc00000000001c000 for production kernels.  Note that the value must
# be 16kb aligned or the handling of the current variable will break.
#
ifdef CONFIG_SGI_IP27
core-$(CONFIG_SGI_IP27)		+= arch/mips/sgi-ip27/
cflags-$(CONFIG_SGI_IP27)	+= -Iinclude/asm-mips/mach-ip27
ifdef CONFIG_MAPPED_KERNEL
load-$(CONFIG_SGI_IP27)		+= 0xc00000004001c000
OBJCOPYFLAGS			:= --change-addresses=0x3fffffff80000000
dataoffset-$(CONFIG_SGI_IP27)	+= 0x01000000
else
load-$(CONFIG_SGI_IP27)		+= 0xa80000000001c000
OBJCOPYFLAGS			:= --change-addresses=0x57ffffff80000000
endif
endif

#
# SGI-IP32 (O2)
#
# Set the load address to >= 80069000 if you want to leave space for symmon,
# 0xffffffff80004000 for production kernels.  Note that the value must be aligned to
# a multiple of the kernel stack size or the handling of the current variable
# will break.
#
core-$(CONFIG_SGI_IP32)		+= arch/mips/sgi-ip32/
cflags-$(CONFIG_SGI_IP32)	+= -Iinclude/asm-mips/mach-ip32
load-$(CONFIG_SGI_IP32)		+= 0xffffffff80004000

#
# Sibyte SB1250 SOC
#
# This is a LIB so that it links at the end, and initcalls are later
# the sequence; but it is built as an object so that modules don't get
# removed (as happens, even if they have __initcall/module_init)
#
core-$(CONFIG_SIBYTE_BCM112X)	+= arch/mips/sibyte/sb1250/
cflags-$(CONFIG_SIBYTE_BCM112X)	+= -Iinclude/asm-mips/mach-sibyte \
			-DSIBYTE_HDR_FEATURES=SIBYTE_HDR_FMASK_1250_112x_ALL

core-$(CONFIG_SIBYTE_SB1250)	+= arch/mips/sibyte/sb1250/
cflags-$(CONFIG_SIBYTE_SB1250)	+= -Iinclude/asm-mips/mach-sibyte \
			-DSIBYTE_HDR_FEATURES=SIBYTE_HDR_FMASK_1250_112x_ALL

core-$(CONFIG_SIBYTE_BCM1x55)	+= arch/mips/sibyte/bcm1480/
cflags-$(CONFIG_SIBYTE_BCM1x55)	+= -Iinclude/asm-mips/mach-sibyte \
			-DSIBYTE_HDR_FEATURES=SIBYTE_HDR_FMASK_1480_ALL

core-$(CONFIG_SIBYTE_BCM1x80)	+= arch/mips/sibyte/bcm1480/
cflags-$(CONFIG_SIBYTE_BCM1x80)	+= -Iinclude/asm-mips/mach-sibyte \
			-DSIBYTE_HDR_FEATURES=SIBYTE_HDR_FMASK_1480_ALL

#
# Sibyte BCM91120x (Carmel) board
# Sibyte BCM91120C (CRhine) board
# Sibyte BCM91125C (CRhone) board
# Sibyte BCM91125E (Rhone) board
# Sibyte SWARM board
# Sibyte BCM91x80 (BigSur) board
#
libs-$(CONFIG_SIBYTE_CARMEL)	+= arch/mips/sibyte/swarm/
load-$(CONFIG_SIBYTE_CARMEL)	:= 0xffffffff80100000
libs-$(CONFIG_SIBYTE_CRHINE)	+= arch/mips/sibyte/swarm/
load-$(CONFIG_SIBYTE_CRHINE)	:= 0xffffffff80100000
libs-$(CONFIG_SIBYTE_CRHONE)	+= arch/mips/sibyte/swarm/
load-$(CONFIG_SIBYTE_CRHONE)	:= 0xffffffff80100000
libs-$(CONFIG_SIBYTE_RHONE)	+= arch/mips/sibyte/swarm/
load-$(CONFIG_SIBYTE_RHONE)	:= 0xffffffff80100000
libs-$(CONFIG_SIBYTE_SENTOSA)	+= arch/mips/sibyte/swarm/
load-$(CONFIG_SIBYTE_SENTOSA)	:= 0xffffffff80100000
libs-$(CONFIG_SIBYTE_SWARM)	+= arch/mips/sibyte/swarm/
load-$(CONFIG_SIBYTE_SWARM)	:= 0xffffffff80100000
libs-$(CONFIG_SIBYTE_BIGSUR)	+= arch/mips/sibyte/swarm/
load-$(CONFIG_SIBYTE_BIGSUR)	:= 0xffffffff80100000

#
# SNI RM200 PCI
#
core-$(CONFIG_SNI_RM200_PCI)	+= arch/mips/sni/
cflags-$(CONFIG_SNI_RM200_PCI)	+= -Iinclude/asm-mips/mach-rm200
load-$(CONFIG_SNI_RM200_PCI)	+= 0xffffffff80600000

#
# Toshiba JMR-TX3927 board
#
core-$(CONFIG_TOSHIBA_JMR3927)	+= arch/mips/jmr3927/rbhma3100/ \
				   arch/mips/jmr3927/common/
cflags-$(CONFIG_TOSHIBA_JMR3927) += -Iinclude/asm-mips/mach-jmr3927
load-$(CONFIG_TOSHIBA_JMR3927)	+= 0xffffffff80050000

#
# Toshiba RBTX4927 board or
# Toshiba RBTX4937 board
#
core-$(CONFIG_TOSHIBA_RBTX4927)	+= arch/mips/tx4927/toshiba_rbtx4927/
core-$(CONFIG_TOSHIBA_RBTX4927)	+= arch/mips/tx4927/common/
load-$(CONFIG_TOSHIBA_RBTX4927)	+= 0xffffffff80020000

#
# Toshiba RBTX4938 board
#
core-$(CONFIG_TOSHIBA_RBTX4938) += arch/mips/tx4938/toshiba_rbtx4938/
core-$(CONFIG_TOSHIBA_RBTX4938) += arch/mips/tx4938/common/
load-$(CONFIG_TOSHIBA_RBTX4938) += 0xffffffff80100000

#
# BCM93548A0
#
ifeq ($(CONFIG_MIPS_BCM3548),y)
core-$(CONFIG_MIPS_BCM3548) += arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM3548) += arch/mips/brcmstb/brcm93548a0/
cflags-$(CONFIG_MIPS_BCM3548) += -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM3548) := 0x80001000
endif
ifdef CONFIG_MIPS_BCM3548A0
        UTS_MACHINE:=3548a0
endif

#
#
# BCM93563C0
#
core-$(CONFIG_MIPS_BCM3563C0) += arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM3563C0) += arch/mips/brcmstb/brcm93563c0/
cflags-$(CONFIG_MIPS_BCM3563C0)   += -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM3563C0) := 0x80001000
ifdef CONFIG_MIPS_BCM3563C0
UTS_MACHINE:= 3563c0
endif

#
# Broadcom BCM93730
#
core-$(CONFIG_BCM93730) += arch/mips/brcm-boards/common arch/mips/brcm-boards/bcm93730
libs-$(CONFIG_BCM93730) += arch/mips/brcm-boards/common/brcmstb.a arch/mips/brcm-boards/bcm93730/bcm93730.a
cflags-$(CONFIG_MIPS_BCM93730)	+= -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_BCM93730) := 0x80001000
ifdef CONFIG_MIPS_BCM93730
UTS_MACHINE:= 93730
endif

#
#
# BCM97038C0
#
core-$(CONFIG_MIPS_BCM7038C0) += arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM7038C0) += arch/mips/brcmstb/brcm97038c0/
cflags-$(CONFIG_MIPS_BCM7038C0)	+= -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7038C0) := 0x80001000
ifdef CONFIG_MIPS_BCM7038C0
	ifdef CONFIG_MIPS_BCM97398
	UTS_MACHINE:= 97398
	else
	UTS_MACHINE:= 7038c0
	endif
endif

#
#
# BCM97118A0
#
core-$(CONFIG_MIPS_BCM7118A0) += arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM7118A0) += arch/mips/brcmstb/brcm97118a0/
cflags-$(CONFIG_MIPS_BCM7118A0)	+= -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7118A0) := 0x80001000
ifdef CONFIG_MIPS_BCM7118A0
UTS_MACHINE:=7118a0
endif

#
#
# BCM97118C0
#
core-$(CONFIG_MIPS_BCM7118C0) += arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM7118C0) += arch/mips/brcmstb/brcm97118c0/
cflags-$(CONFIG_MIPS_BCM7118C0)	+= -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7118C0) := 0x80001000
ifdef CONFIG_MIPS_BCM7118C0
UTS_MACHINE:=7118c0
endif

#
# BCM97400D0 & BCM97456D0
#
ifeq ($(CONFIG_MIPS_BCM97456D0),y)
# BCM97456D0
core-$(CONFIG_MIPS_BCM97456D0) += arch/mips/brcmstb/brcm97400d0/
UTS_MACHINE:=97456d0
else
core-$(CONFIG_MIPS_BCM7400D0) += arch/mips/brcmstb/brcm97400d0/
ifdef CONFIG_MIPS_BCM7400D0
	UTS_MACHINE:=7400d0
endif
endif

core-$(CONFIG_MIPS_BCM7400D0) += arch/mips/brcmstb/common/
cflags-$(CONFIG_MIPS_BCM7400D0)	+= -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7400D0) := 0x80001000

#
# BCM97405
#
ifeq ($(CONFIG_MIPS_BCM7405),y)
core-$(CONFIG_MIPS_BCM7405) += arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM7405A0) += arch/mips/brcmstb/brcm97405a0/
core-$(CONFIG_MIPS_BCM7405B0) += arch/mips/brcmstb/brcm97405b0/
cflags-$(CONFIG_MIPS_BCM7405) += -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7405) := 0x80001000
endif

ifdef CONFIG_TIVO_NEUTRON
core-$(CONFIG_MIPS_BCM7405B0) += arch/mips/tivo/Gen07/
core-$(CONFIG_MIPS_BCM7405B0) += arch/mips/tivo/common/
endif

ifdef CONFIG_MIPS_BCM7405A0
        UTS_MACHINE:=mips
endif
ifdef CONFIG_MIPS_BCM7405B0
ifdef CONFIG_MIPS_BCM97459B0
	UTS_MACHINE:=97459b0
else
	UTS_MACHINE:=mips
endif
endif

#
# BCM97335
#
ifeq ($(CONFIG_MIPS_BCM7335),y)
core-$(CONFIG_MIPS_BCM7335) += arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM7335) += arch/mips/brcmstb/brcm97335a0/
cflags-$(CONFIG_MIPS_BCM7335) += -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7335) := 0x80001000
endif
ifdef CONFIG_MIPS_BCM7335A0
        UTS_MACHINE:=7335a0
endif

ifdef CONFIG_TIVO_KONTIKI
core-$(CONFIG_MIPS_BCM7335) += arch/mips/tivo/Gen07T/
core-$(CONFIG_MIPS_BCM7335) += arch/mips/tivo/common/
endif


#
# BCM97401C0 & 7402C0
#
ifdef CONFIG_MIPS_BCM7402C0
core-$(CONFIG_MIPS_BCM7402C0) 		+= arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM7402C0) 		+= arch/mips/brcmstb/brcm97401c0/
cflags-$(CONFIG_MIPS_BCM7402C0)		+= -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7402C0) 		:= 0x80001000
else
   	ifdef CONFIG_MIPS_BCM97455C0
   	core-$(CONFIG_MIPS_BCM97455C0) 		+= arch/mips/brcmstb/common/
   	core-$(CONFIG_MIPS_BCM97455C0) 		+= arch/mips/brcmstb/brcm97401c0/
	cflags-$(CONFIG_MIPS_BCM97455C0)	+= -Iinclude/asm-mips/mach-brcmstb
   	load-$(CONFIG_MIPS_BCM97455C0) 		:= 0x80001000
   	else
   	# THT: Changed to 7401C0 from 7401CX to also pick up the NAND version
   	core-$(CONFIG_MIPS_BCM7401C0) 		+= arch/mips/brcmstb/common/
	core-$(CONFIG_MIPS_BCM7401C0) 		+= arch/mips/brcmstb/brcm97401c0/
	cflags-$(CONFIG_MIPS_BCM7401C0)		+= -Iinclude/asm-mips/mach-brcmstb
   	load-$(CONFIG_MIPS_BCM7401C0) 		:= 0x80001000
   	endif
endif

ifdef CONFIG_TIVO_MOJAVE
core-$(CONFIG_MIPS_BCM7401C0) 		+= arch/mips/tivo/Gen06D/
core-$(CONFIG_MIPS_BCM7401C0) 		+= arch/mips/tivo/common/
endif

ifdef CONFIG_MIPS_BCM7402C0
    UTS_MACHINE:=7402c0
endif
ifdef CONFIG_MIPS_BCM7401C0
    UTS_MACHINE:=mips
endif
ifdef CONFIG_MIPS_BCM97455C0
    UTS_MACHINE:=97455c0
endif
ifdef CONFIG_MIPS_BCM7454
    UTS_MACHINE:=7454
endif

#
#
# BCM97325A0
#
core-$(CONFIG_MIPS_BCM7325A0) += arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM7325A0) += arch/mips/brcmstb/brcm97325a0/
cflags-$(CONFIG_MIPS_BCM7325A0) += -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7325A0) := 0x80001000
ifdef CONFIG_MIPS_BCM7325A0
UTS_MACHINE:=7325a0
endif

#
#
# BCM97440A0
#
core-$(CONFIG_MIPS_BCM7440A0) += arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM7440A0) += arch/mips/brcmstb/brcm97440a0/
cflags-$(CONFIG_MIPS_BCM7440A0)	+= -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7440A0) := 0x80001000
ifdef CONFIG_MIPS_BCM7440A0
UTS_MACHINE:=7440a0
endif

#
#
# BCM97440B0
#
core-$(CONFIG_MIPS_BCM7440B0) += arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM7440B0) += arch/mips/brcmstb/brcm97440b0/
cflags-$(CONFIG_MIPS_BCM7440B0)	+= -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7440B0) := 0x80001000
ifdef CONFIG_MIPS_BCM7440B0
UTS_MACHINE:=7440b0
endif


#
#
# BCM97443A0
#
core-$(CONFIG_MIPS_BCM7443A0) += arch/mips/brcmstb/common/
core-$(CONFIG_MIPS_BCM7443A0) += arch/mips/brcmstb/brcm97443a0/
cflags-$(CONFIG_MIPS_BCM7443A0)	+= -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7443A0) := 0x80001000
ifdef CONFIG_MIPS_BCM7443A0
UTS_MACHINE:=7443a0
endif

#
#
# BCM97403A0
#

core-$(CONFIG_MIPS_BCM7403A0) += arch/mips/brcmstb/brcm97403a0/
core-$(CONFIG_MIPS_BCM7403A0) += arch/mips/brcmstb/common/
cflags-$(CONFIG_MIPS_BCM7403A0)	+= -Iinclude/asm-mips/mach-brcmstb
load-$(CONFIG_MIPS_BCM7403A0) := 0x80001000
ifdef CONFIG_MIPS_BCM7403A0
UTS_MACHINE:=7403a0
endif

#Append -n (and) to machine name, must be done prior to -smp
# We now key on what flash_info returns, not the machine name, because
# there may be a system with both NAND and NOR on it.
#ifdef CONFIG_MTD_BRCMNAND
#UTS_MACHINE:=$(UTS_MACHINE)-n
#endif

ifndef CONFIG_TIVO
# PR32215: append -smp to machine name
ifdef CONFIG_SMP
ifdef CONFIG_MIPS_MT_SMTC
UTS_MACHINE:=$(UTS_MACHINE)-smtc
else
UTS_MACHINE:=$(UTS_MACHINE)-smp
endif
endif
endif

# PR10860 - Allow long long divide support for modules
core-$(CONFIG_LONG_LONG_SUPPORT)       += arch/mips/brcmstb/lib/
# LIBS          += arch/mips/brcmstb/lib/lib_brcm_bsp.a

cflags-y			+= -Iinclude/asm-mips/mach-generic
drivers-$(CONFIG_PCI)		+= arch/mips/pci/

ifdef CONFIG_32BIT
ifdef CONFIG_CPU_LITTLE_ENDIAN
JIFFIES			= jiffies_64
else
JIFFIES			= jiffies_64 + 4
endif
else
JIFFIES			= jiffies_64
endif

AFLAGS		+= $(cflags-y)
CFLAGS		+= $(cflags-y)

# THT LDFLAGS			+= -m $(ld-emul)

ifdef CONFIG_MIPS
CHECKFLAGS += $(shell $(CC) $(CFLAGS) -dM -E -xc /dev/null | \
	egrep -vw '__GNUC_(|MINOR_|PATCHLEVEL_)_' | \
	sed -e 's/^\#define /-D/' -e "s/ /='/" -e "s/$$/'/")
ifdef CONFIG_64BIT
CHECKFLAGS		+= -m64
endif
endif

OBJCOPYFLAGS		+= --remove-section=.reginfo

#
# Choosing incompatible machines durings configuration will result in
# error messages during linking.  Select a default linkscript if
# none has been choosen above.
#

CPPFLAGS_vmlinux.lds := \
	$(CFLAGS) \
	-D"LOADADDR=$(load-y)" \
	-D"JIFFIES=$(JIFFIES)" \
	-D"DATAOFFSET=$(if $(dataoffset-y),$(dataoffset-y),0)"

head-y := arch/mips/kernel/head.o arch/mips/kernel/init_task.o

libs-y			+= arch/mips/lib/
libs-$(CONFIG_32BIT)	+= arch/mips/lib-32/
libs-$(CONFIG_64BIT)	+= arch/mips/lib-64/

core-y			+= arch/mips/kernel/ arch/mips/mm/ arch/mips/math-emu/

drivers-$(CONFIG_OPROFILE)	+= arch/mips/oprofile/

ifdef CONFIG_LASAT
rom.bin rom.sw: vmlinux
	$(Q)$(MAKE) $(build)=arch/mips/lasat/image $@
endif

#
# Some machines like the Indy need 32-bit ELF binaries for booting purposes.
# Other need ECOFF, so we build a 32-bit ELF binary for them which we then
# convert to ECOFF using elf2ecoff.
#
vmlinux.32: vmlinux
	$(OBJCOPY) -O $(32bit-bfd) $(OBJCOPYFLAGS) $< $@

#
# The 64-bit ELF tools are pretty broken so at this time we generate 64-bit
# ELF files from 32-bit files by conversion.
#
vmlinux.64: vmlinux
	$(OBJCOPY) -O $(64bit-bfd) $(OBJCOPYFLAGS) $< $@

makeboot =$(Q)$(MAKE) $(build)=arch/mips/boot VMLINUX=$(vmlinux-32) $(1)

ifdef CONFIG_BOOT_ELF32
all:	$(vmlinux-32)
endif

ifdef CONFIG_BOOT_ELF64
all:	$(vmlinux-64)
endif

ifdef CONFIG_MIPS_ATLAS
all:	vmlinux.srec
endif

ifdef CONFIG_MIPS_MALTA
all:	vmlinux.srec
endif

ifdef CONFIG_MIPS_SEAD
all:	vmlinux.srec
endif

ifdef CONFIG_QEMU
all:	vmlinux.bin
endif

ifdef CONFIG_SNI_RM200_PCI
all:	vmlinux.ecoff
endif

vmlinux.bin: $(vmlinux-32)
	+@$(call makeboot,$@)

vmlinux.ecoff: $(vmlinux-32)
	+@$(call makeboot,$@)

vmlinux.srec: $(vmlinux-32)
	+@$(call makeboot,$@)

CLEAN_FILES += vmlinux.ecoff \
	       vmlinux.srec

archclean:
	@$(MAKE) $(clean)=arch/mips/boot
	@$(MAKE) $(clean)=arch/mips/lasat

CLEAN_FILES += vmlinux.32 \
	       vmlinux.64 \
	       vmlinux.ecoff
