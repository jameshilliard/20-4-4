/*******************************************************************************
*
* Common/Src/hostos_stubs.c
*
* Description: Host OS abstraction support 
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

/*******************************************************************************
*                            # I n c l u d e s                                 *
********************************************************************************/

#include "drv_hdr.h"


/**
*   Purpose:    Sets size bytes of memory to a given byte value.
*
*   Imports:    pMem - pointer to block of memory to set
*               val  - value to set each byte to
*               size - number of bytes to set
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_Memset(void *pMem, int val, int size)
{
}

/**
*   Purpose:    Copies size bytes of memory from pFrom to pTo.
*               The memory areas may not overlap.
*
*   Imports:    pTo   - pointer to destination memory area
*               pFrom - pointer to source memory area
*               size  - number of bytes to copy
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_Memcpy(void *pTo, void *pFrom, int size)
{
}

/**
*   Purpose:    Scans input buffer and according to format specified.
*
*   Imports:    buf   - input buffer
*               fmt   - input format
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_Sscanf(const char *buf, const char *fmt, ...)
{
}

/**
*   Purpose:    Allocates DMA-addressable memory.
*
*   Imports:    size - size to allocate
*
*   Exports:    pointer to allocated block
*
*PUBLIC**************************/
void* HostOS_Alloc(int size)
{
    return (0);
}

/**
*   Purpose:    Free DMA-addressable memory.
*
*   Imports:    pMem - allocation pointer from get free pages
*               size - Requested block size
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_Free(void* pMem, int size)
{
}

/**
*   Purpose:    Delays for about timeInUsec microseconds.
*               The operation of this function is architecture dependent.
*               It is probably a tight loop.
*
*   Imports:    timeInUsec - delay in microseconds
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_Sleep(int timeInUsec)
{
}

/**
*   Purpose:    Inits a spinlock.
*               Sets the magic code for later testing.
*
*   Imports:    vlk - pointer to an Entropic lock structure
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_lock_init( void *vlk)
{
}

/**
*   Purpose:    Waits for the lock.
*               The lock must be initialized.
*
*               See the macro:  HostOS_Lock
*
*   Imports:    vlk - pointer to an initialized Entropic lock structure
*
*   Exports:    none - when this returns you have the lock.
*                      Please do an unlock later.
*
*PUBLIC**************************/
void HostOS_Lock(void *vlk)
{
}

/**
*   Purpose:    Saves current interrupt context and waits for the lock.
*               The lock must be initialized.
*
*   Imports:    vlk - pointer to an initialized Entropic lock structure
*
*   Exports:    none - when this returns you have the lock.
*                      Please do an unlock later.
*
*PUBLIC**************************/
void HostOS_Lock_Irqsave(void *vlk)
{
}

/**
*   Purpose:    Trys to get the lock.
*               The lock must be initialized.
*
*   Imports:    vlk - pointer to an initialized Entropic lock structure
*
*   Exports:    0 - the lock was NOT acquired. Please try again.
*               1 - the lock is yours. Please ulock it later.
*
*PUBLIC**************************/
int HostOS_Lock_Try(void *vlk)
{

    return( 0 );
}

/**
*   Purpose:    Unlocks a lock.
*               The lock must be initialized.
*
*               See the macro:  HostOS_Unlock
*
*   Imports:    vlk - pointer to an initialized Entropic lock structure
*
*   Exports:    none - when this returns you have the lock.
*
*PUBLIC**************************/
void HostOS_Unlock(void *vlk)
{
}

/**
*   Purpose:    Unlocks a lock and restores interrupt context.
*               The lock must be initialized.
*
*
*   Imports:    vlk - pointer to an initialized Entropic lock structure
*
*   Exports:    none - when this returns you have released the lock.
*
*PUBLIC**************************/
void HostOS_Unlock_Irqrestore(void *vlk)
{
}

/**
*   Purpose:    Terminates a lock.
*
*   Imports:    vlk - pointer to an initialized Entropic lock structure
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_TermLock(void *vlk)
{
}

/**
*   Purpose:    Variable argument OS print.
*
*   Imports:    lev - severity level. See the L_* enum.
*                     See HOST_OS_PRINTLOG_THRESHOLD
*               fmt - printf format string
*               ... - printf format arguments
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_PrintLog(SYS_INT32 lev, const char *fmt, ...)
{
}

/**
*   Purpose:    register/memory read access from kernel space.
*
*   Imports:    addr - address to read from
*
*   Exports:    32 bit value from given address
*
*PUBLIC**************************/
SYS_UINT32 HostOS_Read_Word( SYS_UINT32 *addr )
{

    return( 0 ) ;
}

/**
*   Purpose:    register/memory write access from kernel space.
*
*   Imports:    val  - 32 bit value to write
*               addr - address to write to
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_Write_Word( SYS_UINT32 val, SYS_UINT32 *addr )
{
}

/**
*   Purpose:    Initialize a timer and register it.
*
*   Imports:    vtmr - pointer to the timer to initialize
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_timer_init( void *vtmr )
{
}

/**
*   Purpose:    Deactivate a registered timer.
*
*   Imports:    vtmr - pointer to the timer to deactivate
*
*   Exports:    1 = the timer was active (not expired)
*               0 = the timer was not active
*
*PUBLIC**************************/
int HostOS_timer_del( void *vtmr )
{

    return( 0 ) ;
}

/**
*   Purpose:    Deactivate a registered timer.
*               And ensure the timer is not running on any CPU
*               Can sleep
*
*   Imports:    vtmr - pointer to the timer to deactivate
*
*   Exports:    1 = the timer was active (not expired)
*               0 = the timer was not active
*
*PUBLIC**************************/
int HostOS_timer_del_sync( void *vtmr )
{

    return( 0 ) ;
}

/**
*   Purpose:    Modifies a timer.
*
*   Imports:    vtmr    - pointer to the timer to deactivate
*               timeout - timeout value from HostOS_timer_expire_seconds
*                         or HostOS_timer_expire_ticks
*
*   Exports:    1 = the timer was active (not expired)
*               0 = the timer was not active
*
*PUBLIC**************************/
int HostOS_timer_mod( void *vtmr, SYS_ULONG timeout )
{

    return( 0 ) ;
}

/**
*   Purpose:    Add a timer.
*               Call this after HostOS_timer_setup
*
*   Imports:    vtmr - pointer to the timer to deactivate
*
*   Exports:    1 = the timer was active (not expired)
*               0 = the timer was not active
*
*PUBLIC**************************/
void HostOS_timer_add( void *vtmr )
{
}

/**
*   Purpose:    Sets up a timer with function and user data.
*
*               See HostOS_timer_set_timeout
*
*   Imports:    vtmr    - pointer to the timer to deactivate
*               func    - callback function to call at timer expiration
*               data    - data to pass to callback
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_timer_setup( void *vtmr, timer_function_t func, SYS_UINTPTR data )   
{
}

/**
*   Purpose:    Sets a timer's time out.
*
*               See HostOS_timer_setup
*
*   Imports:    vtmr    - pointer to the timer to deactivate
*               timeout - timeout value from HostOS_timer_expire_seconds
*                         or HostOS_timer_expire_ticks
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_timer_set_timeout( void *vtmr, SYS_ULONG timeout )  
{
}

/**
*   Purpose:    Calculate a future expiration point.
*               In seconds.
*
*   Imports:    future - number of seconds into the future
*
*   Exports:    A number for use in the timer expiration member
*
*PUBLIC**************************/
SYS_ULONG HostOS_timer_expire_seconds( SYS_UINT32 future )
{

    return( 0 ) ;
}

/**
*   Purpose:    Calculate a future expiration point.
*              In jiffies.
*
*   Imports:    future - number of ticks into the future
*
*   Exports:    A number for use in the timer expiration member
*
*PUBLIC**************************/
SYS_ULONG HostOS_timer_expire_ticks( SYS_UINT32 future )
{

    return( 0 ) ;
}


/**************************************************************

    wait queue timer - support

    This structure contains everything necessary to
    use timers and wait queues from the !GPL side.

    You allocate one of these wqt_t things and then
    pass its pointer back on the various calls.

***************************************************************/

typedef struct hostos_w_q_t
{
} 
wqt_t ;


/**
*   Purpose:    Allocates a wqt entry from the heap.
*
*   Imports:
*
*   Exports:    !0 = void pointer to opaque data (really a wqt_t)
*               0 = allocation failure
*
*PUBLIC**************************/
void *HostOS_wqt_alloc( void )
{

    return( (void *)0 ) ;
}

/**
*   Purpose:    De-allocates a wqt entry from the heap.
*
*   Imports:    vwqt - pointer to the wqt_t to be freed
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_wqt_free( void *vwqt )
{

}

/**
*   Purpose:    Initialize a timer and register it.
*
*   Imports:    vwqt - pointer to the wqt_t with timer to initialize
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_wqt_timer_init( void *vwqt )
{
}

/**
*   Purpose:    Deactivate a wqt timer
*
*   Imports:    vwqt - pointer to the wqt_t with timer to deactivate
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_wqt_timer_del( void *vwqt )
{
}

/**
*   Purpose:    Deactivate a registered timer
*               And ensure the timer is not running on any CPU.
*               Can sleep.
*
*   Imports:    vwqt - pointer to the wqt_t with timer to deactivate
*
*   Exports:    1 = the timer was active (not expired)
*               0 = the timer was not active
*
*PUBLIC**************************/
int HostOS_wqt_timer_del_sync( void *vwqt )
{

    return( 0 ) ;
}

/**
*   Purpose:    Modifies a timer
*
*   Imports:    vwqt    - pointer to the wqt_t with timer to modify
*               timeout - timeout value from HostOS_timer_expire_seconds
*                        or HostOS_timer_expire_ticks
*
*   Exports:    1 = the timer was active (not expired)
*               0 = the timer was not active
*
*PUBLIC**************************/
int HostOS_wqt_timer_mod( void *vwqt, SYS_ULONG timeout )  
{

    return( 0 ) ;
}

/**
*   Purpose:    Add a timer
*               Call this after HostOS_timer_setup
*
*   Imports:    vwqt    - pointer to the wqt_t with timer to add
*
*   Exports:    1 = the timer was active (not expired)
*               0 = the timer was not active
*
*PUBLIC**************************/
void HostOS_wqt_timer_add( void *vwqt )
{
}

/**
*   Purpose:    Sets up a timer with function and user data.
*
*               See HostOS_timer_set_timeout
*
*   Imports:    vwqt    - pointer to the wqt_t with timer to  init
*               func    - callback function to call at timer expiration
*               data    - data to pass to callback
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_wqt_timer_setup( void *vwqt, timer_function_t func, SYS_UINTPTR data )  
{
}

/**
*   Purpose:    Sets a timer's time out.
*
*               See HostOS_timer_setup
*
*   Imports:    vwqt    - pointer to the wqt_t with timer to set
*               timeout - timeout value from HostOS_timer_expire_seconds
*                         or HostOS_timer_expire_ticks
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_wqt_timer_set_timeout( void *vwqt, SYS_ULONG timeout ) 
{
}

/**
*   Purpose:    Inits a wait queue head in a wqt_t.
*
*   Imports:    vwqt    - pointer to the wqt_t with wait q to init
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_wqt_waitq_init( void *vwqt )
{
}

/**
*   Purpose:    Wakes up a wait q in a wqt_t.
*
*   Imports:    vwqt    - pointer to the wqt_t with wait q to wake
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_wqt_waitq_wakeup_intr( void *vwqt )
{
}

/**
*   Purpose:    Waits for a wait q event in a wqt_t.
*
*   Imports:    vwqt    - pointer to the wqt_t with wait q to wait on
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_wqt_waitq_wait_event_intr( void *vwqt, HostOS_wqt_condition func, void *vp )
{
}

#if defined(PCI_DRVR_SUPPORT)
/**
*   Purpose:    PCI device register access.
*               These functions read or write the PCI bus device.
*
*   Imports:    ddev           - context structure with device pointer
*               reg            - PCI device bus address/register address
*               pVal or val    - where to put or get the data
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_ReadPciConfig_Word(void* ddev, SYS_UINT32 reg, SYS_UINT16* pVal)
{

}

/**
*   Purpose:    PCI device register access.
*               These functions read or write the PCI bus device.
*
*   Imports:    ddev           - context structure with pci_dev pointer
*               reg            - PCI device bus address/register address
*               pVal or val    - where to put or get the data
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_ReadPciConfig(void* ddev, SYS_UINT32 reg, SYS_UINT32* pVal)
{

}

/**
*   Purpose:    PCI device register access.
*               These functions read or write the PCI bus device.
*
*   Imports:    ddev           - context structure with device pointer
*               reg            - PCI device bus address/register address
*               pVal or val    - where to put or get the data
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_WritePciConfig_Word(void *ddev, SYS_UINT32 reg, SYS_UINT16 val)
{

}

/**
*   Purpose:    PCI device register access.
*               These functions read or write the PCI bus device.
*
*   Imports:    ddev           - context structure with device pointer
*               reg            - PCI device bus address/register address
*               pVal or val    - where to put or get the data
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_WritePciConfig(void* ddev, SYS_UINT32 reg, SYS_UINT32 val)
{

}

/**
*   Purpose:    Allocate DMA-addressable memory.
*
*   Imports:    ddev    - pci_dev pointer
*               size    - Requested block size
*               ppMemPa - Pointer to place to return the Physical memory address
*
*   Exports:    Virtual memory address (and Phys address), or NULL on failure
*
*PUBLIC**************************/
void *HostOS_AllocDmaMem(void *ddev, int size, void **ppMemPa)
{

    return (0);
}

/**
*   Purpose:    Free DMA-addressable memory.
*
*   Imports:    ddev   - pci_dev pointer
*               size   - Requested block size
*               pMemVa - Virtual memory address returned by HostOS_AllocDmaMem()
*               pMemPa - Physical memory address returned by    ditto
*
*   Exports:    Virtual memory address (and Phys address), or NULL on failure
*
*PUBLIC**************************/
void HostOS_FreeDmaMem(void *ddev, int size, void *pMemVa, void *pMemPa)
{
}
#endif

/**
*   Purpose:    Initialize a tasklet.
*
*   Imports:    vtl  - pointer to the tasklet structure
*               func - tasklet function
*               data - tasklet context data
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_task_init( void *vtl, void *func, unsigned long data )
{
}

/**
*   Purpose:    Schedule a tasklet.
*               Tasklet must already be initialized.
*
*   Imports:    vtl - pointer to the tasklet structure
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_task_schedule( void *vtl )
{
}

/**
*   Purpose:    Enable a tasklet.
*
*   Imports:    vtl - pointer to the tasklet structure
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_task_enable( void *vtl )
{
}

/**
*   Purpose:    Disable a tasklet.
*
*   Imports:    vtl - pointer to the tasklet structure
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_task_disable( void *vtl )
{
}

/**
*   Purpose:    Kill a tasklet.
*
*   Imports:    vtl - pointer to the tasklet structure
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_task_kill( void *vtl )
{
}

/**
*   Purpose:    Initialize a mutex.
*
*   Imports:    vmt - pointer to the mutex structure
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_mutex_init( void *vmt )
{
}

/**
*   Purpose:    Release (up) a mutex.
*
*   Imports:    vmt - pointer to the mutex structure
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_mutex_release( void *vmt )
{
}

/**
*   Purpose:    Acquire (down) a mutex.
*
*   Imports:    vmt - pointer to the mutex structure
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_mutex_acquire( void *vmt )
{
}

/**
*   Purpose:    Acquire (down) a mutex (Interruptible).
*
*   Imports:    vmt - pointer to the mutex structure
*
*   Exports:    0      - you got the mutex
*               -EINTR - you've been interrupted, no mutex
*
*PUBLIC**************************/
int HostOS_mutex_acquire_intr( void *vmt )
{

    return( 0 ) ;
}

/**
*   Purpose:    Copies a block from user space to kernel space.
*
*   Imports:    to     - pointer to a kernel space buffer to receive the data
*               from   - pointer to a user space buffer to source the data
*               nbytes - number of bytes to copy
*
*   Exports:    number of bytes copied 
*
*PUBLIC**************************/
unsigned long HostOS_copy_from_user( void *to, const void *from, unsigned long nbytes )
{

    return( 0 ) ;
}

/**
*   Purpose:    Copies a block from kernel space to user space.
*
*   Imports:    to     - pointer to a user space buffer to receive the data
*               from   - pointer to a kernel space buffer to source the data
*               nbytes - number of bytes to copy
*
*   Exports:    number of bytes copied 
*
*PUBLIC**************************/
unsigned long HostOS_copy_to_user( void *to, const void *from, unsigned long nbytes )
{

    return( 0 ) ;
}

#if defined(PCI_DRVR_SUPPORT)
/**
*   Purpose:    Check if the carrier is ok.
*
*   Imports:    kdev - context structure with device pointer
*
*   Exports:
*
*PUBLIC**************************/
unsigned long HostOS_netif_carrier_ok( void *kdev )
{

    return( 0 ) ;
}

/**
*   Purpose:    Sets net carrier on.
*
*   Imports:    kdev - context structure with device pointer
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_netif_carrier_on( void *kdev )
{
}

/**
*   Purpose:    Sets net carrier off.
*
*   Imports:    kdev - context structure with device pointer
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_netif_carrier_off( void *kdev )
{
}

/**
*   Purpose:    Sets the MAC address in the OS.
*
*   Imports:    mac_hi - Bytes 0-3 of MAC address
*               mac_lo - Bytes 4-5 of MAC address
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_set_mac_address( void *kdev, SYS_UINT32 mac_hi, SYS_UINT32 mac_lo )
{
}

/**
*   Purpose:    Brings up the Ethernet interface.
*
*   Imports:    kdev - context structure with device pointer
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_open( void *kdev )
{
}

/**
*   Purpose:    Shuts down the Ethernet interface.
*
*   Imports:    kdev - context structure with device pointer
*
*   Exports:
*
*PUBLIC**************************/
void HostOS_close( void *kdev )
{
}
#endif

#if defined(E1000_DRVR_SUPPORT) || defined(CANDD_DRVR_SUPPORT)

/**
*   Purpose:    Check pending signal of task.
*
*   Imports:    vtask - pointer to the task
*
*   Exports:    0 - no pending signal
*               1 - has pendign signal
*
*PUBLIC**************************/
int HostOS_signal_pending(void *vtask)
{
    return 0;
}

/**
*   Purpose:    Sleep waiting for waitqueue interruptions.
*
*   Imports:    msecs - Time in milliseconds to sleep for
*
*   Exports:    none
*
*PUBLIC**************************/
void HostOS_msleep_interruptible(unsigned int msecs)
{
    
}

/**
*   Purpose:    Create new kernel thread.
*
*   Imports:    pThreadID - pointer to new kernel threadID
*               pName     - pointer to thread name string
*               func      - pointer to thread internal function
*               arg       - argument to pass to thread function
*
*   Exports:    0  - OK
*               -1 - FAILED
*
*PUBLIC**************************/
int HostOS_thread_start(unsigned int *pThreadID, char *pName, void (*func)(void *), void *arg)
{
    return 0;
}

/**
*   Purpose:    Stop a kernel thread.
*
*   Imports:    threadID - kernel threadID that to be stopped
*
*   Exports:    0   - OK
*               -1  - FAILED
*
*PUBLIC**************************/
int HostOS_thread_stop(unsigned long threadID)
{
    return 0;
}

#endif

