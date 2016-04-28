/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef _ASM_SHMPARAM_H
#define _ASM_SHMPARAM_H

#define __ARCH_FORCE_SHMLBA	1
#include <linux/autoconf.h>

#ifdef CONFIG_TIVO
#define	SHMLBA 0x4000
#else
#define	SHMLBA 0x40000			/* attach addr a multiple of this */
#endif

#endif /* _ASM_SHMPARAM_H */
