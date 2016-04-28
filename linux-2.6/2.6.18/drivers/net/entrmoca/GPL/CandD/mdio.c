/*******************************************************************************
*
* GPL/CandD/mdio.c
*
* Description: High level MDIO access
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
*******************************************************************************/

/*******************************************************************************
*                            # I n c l u d e s                                 *
********************************************************************************/

#include "candd_gpl_hdr.h"


unsigned long ClinkReadFrom(unsigned long addr);
void ClinkWriteTo(unsigned long addr, unsigned long data);
void Turbo_open(unsigned long addr);
void Turbo_close(void);
void Turbo_write(unsigned long data);
unsigned int Turbo_read(void);



/*
*PUBLIC*******************************************************************************/
int clnk_write( void *vctx, SYS_UINT32 addr, SYS_UINT32 data)
{
    ClinkWriteTo( addr, data );

    return(SYS_SUCCESS);
}


/*
*PUBLIC*******************************************************************************/
int clnk_read( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data)
{
    SYS_UINT32      ret;
    
    ret = ClinkReadFrom( addr );
    *data = ret;

    return(SYS_SUCCESS);
}

/*
*P UBLIC*******************************************************************************/
int clnk_write_burst( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data, unsigned int size, int inc )
{
    unsigned int    i;
   
    Turbo_open(addr);

    for(i = 0; i < size; i += sizeof(*data))
    {
        Turbo_write(*data);
        data++;
    }

    Turbo_close();

    return(SYS_SUCCESS);
}

/*
*P UBLIC*******************************************************************************/
int clnk_read_burst( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data, unsigned int size, int inc)
{
    unsigned int    i;

    Turbo_open(addr);

    for(i = 0; i < size; i += sizeof(*data))
    {
        *data = Turbo_read();
        data++;
    }

    Turbo_close();

    return(SYS_SUCCESS);
}

