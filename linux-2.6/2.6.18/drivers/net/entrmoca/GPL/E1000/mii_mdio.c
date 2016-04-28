/*******************************************************************************
*
* GPL/E1000/mii_mdio.c
*
* Description: MDIO and Sonics access functions for MII mode
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

#include "e1000_gpl_hdr.h"
#include "e1000.h"


static SYS_INT32 mii_read( struct e1000_hw *ehw, SYS_UINT32 reg);
static void mii_write( struct e1000_hw *ehw, int reg, SYS_UINT16 data);
static int clnk_busy_check( struct e1000_hw *ehw );



/*
*STATIC*******************************************************************************/
static SYS_INT32 mii_read( struct e1000_hw *ehw, SYS_UINT32 reg)
{
    SYS_INT32  x ;
    SYS_UINT16 s ;                                     

//HostOS_PrintLog(L_INFO, "%s: ehw=%08x\n", __FUNCTION__ , (unsigned int)ehw );
//HostOS_PrintLog(L_INFO, "%s: address to be read=%08x\n", __FUNCTION__ , (unsigned int)reg );

    if ((x = e1000_read_phy_reg( ehw, reg & 0x1f, &s )))
       HostOS_PrintLog(L_ERR, "e1000_read_phy_reg failure\n") ;

//HostOS_PrintLog(L_INFO, "%s: data=%08x\n", __FUNCTION__ , s );
    return( s ) ;
}

/*
*STATIC*******************************************************************************/
static void mii_write( struct e1000_hw *ehw, int reg, SYS_UINT16 data)
{

//HostOS_PrintLog(L_INFO, "%s: address=%08x\n", __FUNCTION__ , (unsigned int)reg );
//HostOS_PrintLog(L_INFO, "%s: data=%08x\n", __FUNCTION__ , (unsigned int)data );
    if (e1000_write_phy_reg( ehw, reg, data ))
       HostOS_PrintLog(L_ERR, "e1000_write_phy_reg failure\n");
}

/*
*STATIC*******************************************************************************/
static int clnk_busy_check( struct e1000_hw *ehw )
{
    SYS_INT32 addr_mode;
    int       stat ;

//HostOS_PrintLog(L_INFO, "%s: ehw=%08x\n", __FUNCTION__ , (unsigned int)ehw );
    stat = 0 ;
    do
    {
        addr_mode = mii_read( ehw, MMI_REG_ENH_ADDR_MODE );
        if(addr_mode < 0)
        {
            HostOS_PrintLog(L_ERR, "MMI_REG_ENH_ADDR_MODE lookup failed\n");
            stat = -1 ;
            break ;
        }
        if(addr_mode & GPHY_ENH_ADDR_MODE__ERROR)
        {
            HostOS_PrintLog(L_ERR, "MMI reported ADDR_MODE_ERROR\n");
            stat = -1 ;
            break ;
        }
    } while( addr_mode & GPHY_ENH_ADDR_MODE__BUSY );

//HostOS_PrintLog(L_INFO, "%s: stat=%08x\n", __FUNCTION__ , (unsigned int)stat );
    return( stat ) ;
}

/*
*   Purpose:    Writes a clink bus register/location
*               The register/location is accessed via MDIO
*
*   Imports:    vctx - pointer to the driver control context
*               addr - address of the register/location
*                      this will be translated
*               data - the data to write
*
*   Exports:    -1 on busy, 0 otherwise
*
*PUBLIC*******************************************************************************/
int clnk_write( void *vctx, SYS_UINT32 addr, SYS_UINT32 data)
{
    struct e1000_hw *ehw = &((dd_context_t *)dc_to_dd( vctx ))->hw ;
//HostOS_PrintLog(L_INFO, "In clnk_write function, addr = %x\n",addr);
    if( clnk_busy_check( ehw ) < 0 ) {
        return(-1);
    }

    mii_write( ehw, MMI_REG_ENH_ADDR_PORT_HI, addr >> 16) ;
    mii_write( ehw, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;

    mii_write( ehw, MMI_REG_ENH_DATA_PORT_HI, data >> 16) ;
    mii_write( ehw, MMI_REG_ENH_DATA_PORT_LO, data & 0xffff) ;

    mii_write( ehw, MMI_REG_ENH_ADDR_MODE, GPHY_ENH_ADDR_MODE__START_WRITE) ;
//HostOS_PrintLog(L_INFO, "OUT clnk_write function\n");
    return(0);
}

/*
*P UBLIC*******************************************************************************/
int clnk_write_burst( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data, unsigned int size, int inc )
{
    struct e1000_hw *ehw = &((dd_context_t *)dc_to_dd( vctx ))->hw ;
    unsigned int    i;

    if( clnk_busy_check( ehw ) < 0 ) {
        return(-1);
    }

    mii_write( ehw, MMI_REG_ENH_ADDR_PORT_HI, addr >> 16) ;
    mii_write( ehw, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;
    mii_write( ehw, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;

    for(i = 0; i < size; i += sizeof(*data))
    {
        int auto_inc;

        /*
         * FPGA AUTO_INCR is pre-increment
         * SoC AUTO_INCR is post-increment
         */
        if((i == 0) && (!inc))
            auto_inc = 0;
        else
            auto_inc = GPHY_ENH_ADDR_MODE__AUTO_INCR;

        mii_write( ehw, MMI_REG_ENH_DATA_PORT_HI, (*data) >> 16) ;
        mii_write( ehw, MMI_REG_ENH_DATA_PORT_LO, (*data) & 0xffff) ;

        mii_write( ehw, MMI_REG_ENH_ADDR_MODE,
                   GPHY_ENH_ADDR_MODE__START_WRITE |
                   auto_inc) ;

        data++;
    }
    mii_write( ehw, MMI_REG_ENH_ADDR_MODE, 0) ;

    return(0);
}

/*
*   Purpose:    Reads a clink bus register/location
*               The register/location is accessed via MDIO
*
*   Imports:    vctx - pointer to the driver control context
*               addr - address of the register/location
*                      this will be translated
*               val  - pointer to place to put the data
*
*   Exports:    -1 on busy, 0 otherwise
*
*PUBLIC*******************************************************************************/
int clnk_read( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data)
{
    struct e1000_hw *ehw = &((dd_context_t *)dc_to_dd( vctx ))->hw ;
    SYS_INT32       out;
    SYS_UINT32      ret;
    
//HostOS_PrintLog(L_INFO, "In clnk_read function\n");
//HostOS_PrintLog(L_INFO, "%s: vctx=%08x\n", __FUNCTION__ , vctx );
//HostOS_PrintLog(L_INFO, "%s: data=%08x\n", __FUNCTION__ , data );
//HostOS_PrintLog(L_INFO, "%s: ehw=%08x\n", __FUNCTION__ , ehw );
    if( clnk_busy_check( ehw ) < 0 ) {
//HostOS_PrintLog(L_ERR, "%s: return -1\n", __FUNCTION__ );
        return(-1);
    }

    mii_write( ehw, MMI_REG_ENH_ADDR_PORT_HI, addr >> 16) ;
    mii_write( ehw, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;

    mii_write( ehw, MMI_REG_ENH_ADDR_MODE, GPHY_ENH_ADDR_MODE__START_READ) ;

    if( clnk_busy_check( ehw ) < 0 ) {
        return(-1);
    }

    out = mii_read( ehw, MMI_REG_ENH_DATA_PORT_HI);
    ret = out << 16;

    out = mii_read( ehw, MMI_REG_ENH_DATA_PORT_LO);
    *data = ret | (out & 0xffff);

//HostOS_PrintLog(L_INFO, "%s: return 0\n", __FUNCTION__ );
//HostOS_PrintLog(L_INFO, "OUT clnk_read function\n");
    return(0);
}

/*
*P UBLIC*******************************************************************************/
int clnk_read_burst( void *vctx, SYS_UINT32 addr, SYS_UINT32 *data, unsigned int size, int inc)
{
    struct e1000_hw *ehw = &((dd_context_t *)dc_to_dd( vctx ))->hw ;
    SYS_INT32       out;
    SYS_UINT32      ret;
    unsigned int    i;

    if( clnk_busy_check( ehw ) < 0 ) {
        return(-1);
    }

    mii_write( ehw, MMI_REG_ENH_ADDR_PORT_HI, addr >> 16)    ;
    mii_write( ehw, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;
    mii_write( ehw, MMI_REG_ENH_ADDR_PORT_LO, addr & 0xffff) ;

    for(i = 0; i < size; i += sizeof(*data))
    {
        int auto_inc;

        if((i == 0) && (!inc))
            auto_inc = 0;
        else
            auto_inc = GPHY_ENH_ADDR_MODE__AUTO_INCR;

        mii_write( ehw, MMI_REG_ENH_ADDR_MODE,
                   GPHY_ENH_ADDR_MODE__START_READ | auto_inc) ;

        if( clnk_busy_check( ehw ) < 0 ) {
            return(-1);
        }

        out = mii_read( ehw, MMI_REG_ENH_DATA_PORT_HI);
        ret = out << 16;

        out = mii_read( ehw, MMI_REG_ENH_DATA_PORT_LO);
        *data = ret | (out & 0xffff);
        data++;
    }
    mii_write( ehw, MMI_REG_ENH_ADDR_MODE, 0) ;

    return(0);
}
