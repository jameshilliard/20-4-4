/*
 * TiVo configuration registry
 *
 * Copyright (C) 2001, 2002 TiVo Inc. All Rights Reserved.
 */

#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/tivoconfig.h>

// This will instantiate the initialization array:
//
#include <linux/tivoconfig-init.h>

EXPORT_SYMBOL(GetTivoConfig);
EXPORT_SYMBOL(SetTivoConfig);
DEFINE_SPINLOCK(gTivoConfigSpinlock);

TivoConfig gTivoConfig[ kTivoConfigMaxNumSelectors ];
int gTivoConfigCount = 0;

TStatus SetTivoConfig( TivoConfigSelector selector, TivoConfigValue value )
{
    TStatus result = -1;

    long flags;    
  
    spin_lock_irqsave(&gTivoConfigSpinlock, flags);

    {
	int i;
	for( i = 0; i < gTivoConfigCount; ++i )
	{
	    if( gTivoConfig[ i ].selector == selector )
	    {
		gTivoConfig[ i ].value = value;	    
		result = 0;
		break;	    
	    }	
	}
	if( result != 0 )
	{
	    if( gTivoConfigCount < kTivoConfigMaxNumSelectors )
	    {
		TivoConfig *tivoConfig = &gTivoConfig[ gTivoConfigCount++ ];

		tivoConfig->selector = selector;
		tivoConfig->value = value;	
	
		result = 0;	     	
	    }        
	    else
	    {
		result = -ENOMEM;	
	    }	
	}
    }

    spin_unlock_irqrestore(&gTivoConfigSpinlock, flags);

    return result;    
}

TStatus GetTivoConfig( TivoConfigSelector selector, TivoConfigValue *value )
{
    TStatus result = -1;

    int i;
    
    for( i = 0; i < gTivoConfigCount; ++i )
    {
	if( gTivoConfig[ i ].selector == selector )
	{
	    *value = gTivoConfig[ i ].value;	    
	    result = 0;
	    break;	    
	}	
    }
    return result;    
}
 
void __init InitTivoConfig( TivoConfigValue boardID )
{

    TivoConfig *table;
    
    int tableIndex = 0;

    while( ( table = gGlobalConfigTable[ tableIndex++ ] ) )
    {
	if( table[0].value == boardID )
	{
	    int selectorIndex = 0;

	    while( table[ selectorIndex ].selector != 0 )
	    {
		SetTivoConfig( table[ selectorIndex ].selector, table[ selectorIndex ].value );

		selectorIndex += 1;
	    }
	    break;
	}
    }

    if (!table) {
	printk("InitTivoConfig: unrecognized board/platform id: 0x%x\n", (unsigned int)boardID);
    }
}


