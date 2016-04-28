//////////////////////////////////////////////////////////////////////
//
// File: tivogfx_input_driver.c
//
// Copyright 2013 TiVo Inc.  All Rights Reserved.
//
//////////////////////////////////////////////////////////////////////

// always include first
#include "tivogfx_directfb.h"

#include <core/input.h>
#include <core/input_driver.h>
#include <core/system.h>

#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tivogfx_util.h"
#include "tivogfx_input.h"

/******************************************************************************/

DFB_INPUT_DRIVER( tivogfx )

D_DEBUG_DOMAIN( tivogfx_input_driver,
                "tivogfx/input/driver",
                "TiVo Gfx input driver" );

/******************************************************************************/

#define TIVOGFX_INPUT_MAJOR_VERSION   0
#define TIVOGFX_INPUT_MINOR_VERSION   2

/******************************************************************************/

/* exported symbols */

/*
 * Return the number of available devices.
 * Called once during initialization of DirectFB.
 */
static int
driver_get_available( void )
{
    D_DEBUG_AT( tivogfx_input_driver, "%s()\n", __FUNCTION__ );

    if (dfb_system_type() == CORE_TIVOGFX)
    {
        return 1;
    }

    return 0;
}

/*
 * Fill out general information about this driver.
 * Called once during initialization of DirectFB.
 */
static void
driver_get_info( InputDriverInfo * info )
{
    D_DEBUG_AT( tivogfx_input_driver, "%s()\n", __FUNCTION__ );

    /* fill driver info structure */
    snprintf( info->name, DFB_INPUT_DRIVER_INFO_NAME_LENGTH,
        "TiVo Gfx input" );

    snprintf( info->vendor, DFB_INPUT_DRIVER_INFO_VENDOR_LENGTH,
        "TiVo Inc." );

    info->version.major = TIVOGFX_INPUT_MAJOR_VERSION;
    info->version.minor = TIVOGFX_INPUT_MINOR_VERSION;
}

/*
 * Open the device, fill out information about it,
 * allocate and fill private data, start input thread.
 * Called during initialization, resuming or taking over mastership.
 */
static DFBResult
driver_open_device( CoreInputDevice  * device,
                    unsigned int       number,
                    InputDeviceInfo  * info,
                    void            ** driver_data )
{
    D_DEBUG_AT( tivogfx_input_driver, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( number );

    /* set device vendor and name */
    snprintf( info->desc.name,   DFB_INPUT_DEVICE_DESC_NAME_LENGTH,
        "TiVo Gfx input" );

    snprintf( info->desc.vendor, DFB_INPUT_DEVICE_DESC_VENDOR_LENGTH,
        "TiVo Inc." );

    /* set one of the primary input device IDs */
    info->prefered_id = DIDID_KEYBOARD;

    /* set type flags */
    info->desc.type   = DIDTF_KEYBOARD;

    /* set capabilities */
    info->desc.caps   = DICAPS_ALL;

    /* enable translation of fake raw hardware keycodes */
    info->desc.min_keycode = 8;
    info->desc.max_keycode = 255;

    /* start input thread and set private data pointer */
    *driver_data = TvGfx_InputOpen( device );

    return DFB_OK;
}

/*
 * Fetch one entry from the device's keymap if supported.
 * this does a fake mapping based on the orginal DFB code
 */
static DFBResult
driver_get_keymap_entry(    CoreInputDevice           * device,
                            void                      * driver_data,
                            DFBInputDeviceKeymapEntry * entry )
{
    D_DEBUG_AT( tivogfx_input_driver, "%s()\n", __FUNCTION__ );

    TIVOGFX_UNUSED_PARAMETER( device );
    TIVOGFX_UNUSED_PARAMETER( driver_data );
    TIVOGFX_UNUSED_PARAMETER( entry );

    return DFB_UNSUPPORTED;
}

/*
 * End thread, close device and free private data.
 */
static void
driver_close_device( void * driver_data )
{
    D_DEBUG_AT( tivogfx_input_driver, "%s()\n", __FUNCTION__ );

    /* stop input thread and free private data */
    TvGfx_InputClose( driver_data );
}

/******************************************************************************/

/*
 * Proxy function to be called from C++ module.
 */
void TvGfx_InputDispatch( CoreInputDevice * device, DFBInputEvent * evt )
{
    dfb_input_dispatch( device, evt );
}
