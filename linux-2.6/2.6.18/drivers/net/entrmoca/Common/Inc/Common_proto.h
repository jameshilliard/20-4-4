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


/*** public prototypes from Src/ClnkIo.c ***/
unsigned long clnkioc_driver_cmd( void *dkcp, void *arg );
unsigned long clnkioc_mbox_cmd_request( void *dkcp, void *arg, int response );
unsigned long clnkioc_mbox_unsolq_retrieve( void *dkcp, void *arg );
unsigned long clnkioc_mem_read( void *dkcp, void *arg );
#ifndef SONICS_ADDRESS_OF_sniff_buffer_pa
#error "Sniff buffer is not defined"
#else
#if SONICS_ADDRESS_OF_sniff_buffer_pa
unsigned long clnkioc_sniff_read( void *dkcp, void *arg );
#endif
#endif
unsigned long clnkioc_mem_write( void *dkcp, void *arg );
/* Do Not Edit! Contents produced by script.  Wed Jul 30 19:54:39 PDT 2008  */


/*** public prototypes from Src/ClnkIo_common.c ***/
int clnk_cmd_msg_send_recv( dc_context_t *dccp,    // control context
                            SYS_UINT32 rq_cmd,     // request command
                            SYS_UINT32 rq_scmd,    // subcommand
                            struct clnk_io *iob );
void clnk_blk_read( dc_context_t *dccp, 
                    SYS_UINT32 sourceClinkAddr, 
                    SYS_UINT32 *destHostAddr, 
                    SYS_UINT32 length);
void clnk_blk_write(dc_context_t  *dccp, 
                    SYS_UINT32 destClnkAddr, 
                    SYS_UINT32 *sourceHostAddr, 
                    SYS_UINT32 length);
int clnk_cmd_msg_send( dc_context_t *dccp,    // control context
                       SYS_UINT32 rq_cmd,     // request command
                       SYS_UINT32 rq_scmd,    // subcommand
                       struct clnk_io *iob );
/* Do Not Edit! Contents produced by script.  Wed Jul 30 19:54:39 PDT 2008  */


/*** public prototypes from Src/ClnkMbx_call.c ***/
int MbxSwUnsolRdyCallback(void *vcp, Clnk_MBX_Msg_t *pMsg);
void MbxReplyRdyCallback(void *vcp, Clnk_MBX_Msg_t* pMsg);
/* Do Not Edit! Contents produced by script.  Wed Jul 30 19:54:39 PDT 2008  */


/*** public prototypes from Src/ClnkMbx_dvr.c ***/
/* Do Not Edit! Contents produced by script.  Wed Jul 30 19:54:39 PDT 2008  */


/*** public prototypes from Src/ClnkMbx_ttask.c ***/
SYS_VOID clnketh_tt_timer(SYS_ULONG data);
SYS_VOID clnketh_tt_cmtask(SYS_ULONG data);
SYS_VOID clnketh_tt_umtask(SYS_ULONG data);
/* Do Not Edit! Contents produced by script.  Wed Jul 30 19:54:39 PDT 2008  */


/*** public prototypes from Src/util_dvr.c ***/
void clnk_reg_read(void *vcp, SYS_UINT32 addr, SYS_UINT32 *val);
void clnk_reg_write(void *vcp, SYS_UINT32 addr, SYS_UINT32 val);
void clnk_reg_write_nl(void *vcp, SYS_UINT32 addr, SYS_UINT32 val);
void clnk_reg_read_nl(void *vcp, SYS_UINT32 addr, SYS_UINT32 *val);
/* Do Not Edit! Contents produced by script.  Wed Jul 30 19:54:39 PDT 2008  */


/*** public prototypes from Src/ctx_setup.c ***/
int Clnk_init_dev(void **ddcp_dgcp, void *ddcp, void *dkcp, unsigned long dev_base );
void Clnk_exit_dev( void *vdgcp );
void *ctx_alloc_dc_context( void );
void ctx_free_dc_context( void *dccp );
