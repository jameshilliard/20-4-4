/*******************************************************************************
*
* PCI/Inc/ClnkCore_dvr.h
*
* Description: Describes the new clnkcore file format
*
*******************************************************************************/

/*******************************************************************************
*                        Entropic Communications, Inc.
*                         Copyright (c) 2001-2008
*                          All rights reserved.
*******************************************************************************/

/*******************************************************************************
* This file is licensed under GNU General Public license.                      *
*                                                                              *
* This file is free software: you can redistribute and/or modify it under the  *
* terms of the GNU General Public License, Version 2, as published by the Free *
* Software Foundation.                                                         *
*                                                                              *
* This program is distributed in the hope that it will be useful, but AS-IS and*
* WITHOUT ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,*
* FITNESS FOR A PARTICULAR PURPOSE, TITLE, or NONINFRINGEMENT. Redistribution, *
* except as permitted by the GNU General Public License is prohibited.         *
*                                                                              * 
* You should have received a copy of the GNU General Public License, Version 2 *
* along with this file; if not, see <http://www.gnu.org/licenses/>.            *
********************************************************************************/

#ifndef __CLNKCORE_H__
#define __CLNKCORE_H__

#include "inctypes_dvr.h"

enum
{
    CLNKCORE_REG    = 1,
    CLNKCORE_DATA   = 2,
    CLNKCORE_CSR    = 3,
    CLNKCORE_INST   = 4,
    CLNKCORE_EMPTY  = 5,
};

#define CLNKCORE_MAGIC          0x63307265
#define CLNKCORE_VERSION        2
#define CLNKCORE_MAX_SEC        0x20000
#define CLNKCORE_MAX_N_SEC      32

#define CLNKCORE_ARCH_ZIP1      1
#define CLNKCORE_ARCH_ZIP2      2
#define CLNKCORE_ARCH_MAVERICKS 3

struct clnkcore_hdr
{
        SYS_UINT32      magic;
        SYS_UINT32      version;
        SYS_UINT32      n_sections;
        SYS_UINT32      total_len;
        SYS_UINT32      arch;
        SYS_UINT32      reserved[3];
};

struct clnkcore_sec_hdr
{
        SYS_UINT32      type;
        SYS_UINT32      addr;
        SYS_UINT32      len;
        SYS_UINT32      file_offset;
        SYS_UINT8       name[16];
};

struct clnkcore_reg_sec
{
        SYS_UINT32      gpr[32];
        SYS_UINT32      pc;
        SYS_UINT32      hi;
        SYS_UINT32      lo;
        SYS_UINT32      reserved[5];
};

#endif /* ! __CLNKCORE_H__ */
