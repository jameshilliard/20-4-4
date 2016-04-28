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

/* Do Not Edit! Contents produced by script.  Wed Jul 30 19:54:39 PDT 2008  */


/*** public prototypes from Common/gpl_ctx_abs.c ***/
void *dk_to_dd( void *dkcp );
void *dd_to_dg( void *ddcp );
void *dg_to_dc( void *dgcp );
void *dg_to_dd( void *dgcp );
void *dk_to_dc( void *dkcp );
void *dd_to_dc( void *ddcp );
void *dk_to_dg( void *dkcp );
void *dc_to_dd( void *dccp );

/*** public prototypes from Common/gpl_ctx_setup.c ***/
void *ctx_alloc_dg_context( void );
void ctx_free_dg_context( void *dgcp );
void ctx_link_dg_context( void *vdgcp, void *ddcp, void *dccp );
void Clnk_init_os_context( void *vdgcp ); 
void ctx_linkage_lister( void *dkcp, unsigned int **vp, int len );

/*** public prototypes from Common/hostos.c ***/
void HostOS_Memset(void *pMem, int val, int size);
void HostOS_Memcpy(void *pTo, void *pFrom, int size);
void HostOS_Sscanf(const char *buf, const char *fmt, ...);
void* HostOS_Alloc(int size);
void HostOS_Free(void* pMem, int size);
void HostOS_Sleep(int timeInUsec);
void HostOS_lock_init( void *vlk);
void HostOS_Lock(void *vlk);
void HostOS_Lock_Irqsave(void *vlk);
int HostOS_Lock_Try(void *vlk);
void HostOS_Unlock(void *vlk);
void HostOS_Unlock_Irqrestore(void *vlk);
void HostOS_TermLock(void *vlk);
void HostOS_PrintLog(SYS_INT32 lev, const char *fmt, ...);
SYS_UINT32 HostOS_Read_Word( SYS_UINT32 *addr );
void HostOS_Write_Word( SYS_UINT32 val, SYS_UINT32 *addr );
void HostOS_timer_init( void *vtmr );
int HostOS_timer_del( void *vtmr );
int HostOS_timer_del_sync( void *vtmr );
int HostOS_timer_mod( void *vtmr, SYS_ULONG timeout );
void HostOS_timer_add( void *vtmr );
void HostOS_timer_setup( void *vtmr, timer_function_t func, SYS_UINTPTR data );
void HostOS_timer_set_timeout( void *vtmr, SYS_ULONG timeout );
SYS_ULONG HostOS_timer_expire_seconds( SYS_UINT32 future );
SYS_ULONG HostOS_timer_expire_ticks( SYS_UINT32 future );
void *HostOS_wqt_alloc( void );
void HostOS_wqt_free( void *vwqt );
void HostOS_wqt_timer_init( void *vwqt );
void HostOS_wqt_timer_del( void *vwqt );
int HostOS_wqt_timer_del_sync( void *vwqt );
int HostOS_wqt_timer_mod( void *vwqt, SYS_ULONG timeout );
void HostOS_wqt_timer_add( void *vwqt );
void HostOS_wqt_timer_setup( void *vwqt, timer_function_t func, SYS_UINTPTR data );
void HostOS_wqt_timer_set_timeout( void *vwqt, SYS_ULONG timeout );
void HostOS_wqt_waitq_init( void *vwqt );
void HostOS_wqt_waitq_wakeup_intr( void *vwqt );
void HostOS_wqt_waitq_wait_event_intr( void *vwqt, HostOS_wqt_condition func, void *vp );
void HostOS_ReadPciConfig_Word(void* ddev, SYS_UINT32 reg, SYS_UINT16* pVal);
void HostOS_ReadPciConfig(void* ddev, SYS_UINT32 reg, SYS_UINT32* pVal);
void HostOS_WritePciConfig_Word(void *ddev, SYS_UINT32 reg, SYS_UINT16 val);
void HostOS_WritePciConfig(void* ddev, SYS_UINT32 reg, SYS_UINT32 val);
void *HostOS_AllocDmaMem(void *ddev, int size, void **ppMemPa);
void HostOS_FreeDmaMem(void *ddev, int size, void *pMemVa, void *pMemPa);
void HostOS_task_init( void *vtl, void *func, unsigned long data );
void HostOS_task_schedule( void *vtl );
void HostOS_task_enable( void *vtl );
void HostOS_task_disable( void *vtl );
void HostOS_task_kill( void *vtl );
void HostOS_mutex_init( void *vmt );
void HostOS_mutex_release( void *vmt );
void HostOS_mutex_acquire( void *vmt );
int HostOS_mutex_acquire_intr( void *vmt );
unsigned long HostOS_copy_from_user( void *to, const void *from, unsigned long nbytes );
unsigned long HostOS_copy_to_user( void *to, const void *from, unsigned long nbytes );
unsigned long HostOS_netif_carrier_ok( void *kdev );
void HostOS_netif_carrier_on( void *kdev );
void HostOS_netif_carrier_off( void *kdev );
void HostOS_set_mac_address( void *kdev, SYS_UINT32 mac_hi, SYS_UINT32 mac_lo );
void HostOS_open( void *kdev );
void HostOS_close( void *kdev );
int HostOS_signal_pending(void *pTask);
void HostOS_msleep_interruptible(unsigned int msecs);
int HostOS_thread_start(unsigned int *pThreadID, char *thName, void (*func)(void *), void *arg);
int HostOS_thread_stop(unsigned long threadID);

