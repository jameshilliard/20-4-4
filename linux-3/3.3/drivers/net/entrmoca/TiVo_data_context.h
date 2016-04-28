/*
 * Copyright (c) 2010, TiVo, Inc.  All Rights Reserved
 *
 * This file is licensed under GNU General Public license.                      
 *                                                                              
 * This file is free software: you can redistribute and/or modify it under the  
 * terms of the GNU General Public License, Version 2, as published by the Free 
 * Software Foundation.                                                         
 *                                                                              
 * This program is distributed in the hope that it will be useful, but AS-IS and
 * WITHOUT ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE, or NONINFRINGEMENT. Redistribution, 
 * except as permitted by the GNU General Public License is prohibited.         
 *                                                                              
 * You should have received a copy of the GNU General Public License, Version 2 
 * along with this file; if not, see <http://www.gnu.org/licenses/>.            
 */
#ifndef __TiVo_data_context_h__
#define __TiVo_data_context_h__

struct _driver_kernel_context_
{
	struct net_device *dev;
	void *priv ;    
};
typedef struct _driver_kernel_context_ dk_context_t ;

struct _driver_data_context_
{
	void *p_dg_ctx;    
};
typedef struct _driver_data_context_ dd_context_t ;


#endif /* __TiVo_data_context_h__ */

