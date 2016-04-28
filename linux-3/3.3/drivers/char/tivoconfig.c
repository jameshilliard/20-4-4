// Copyright 2013 TiVo Inc. All Rights Reserved.

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

    unsigned long flags;
  
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
    TivoConfig **table, *entry;
    
    // First, walk the config table looking for an exact match on BoardID (ignoring LSBs)
    for (table=gGlobalConfigTable; *table; table++)
        for (entry=*table; entry->selector; entry++)
            if ((entry->selector == kTivoConfigBoardID) && (entry->value == (boardID & 0xffffff00)))
            {
                printk("Found configuration for 0x%X\n", (uint32_t)boardID & 0xffffff00);
                goto found;
            }    

    // No exact match. Try again, looking for an entry that specifies correct range BoardIDBase to BoardIDMax   
    for (table=gGlobalConfigTable; *table; table++)
    {
        TivoConfigValue base=0, bmax=0; 
        for (entry=*table; entry->selector; entry++)
        {
            if ((entry->selector == kTivoConfigBoardIDBase) && (entry->value <= boardID)) base=entry->value;
            if ((entry->selector == kTivoConfigBoardIDMax) && (entry->value >= boardID)) bmax=entry->value;
            if (base && bmax)
            {  
                printk("Found configuration for 0x%X-0x%X\n", (uint32_t)base, (uint32_t)bmax);
                goto found;
            }   
        }
    }    
    
    printk("WARNING: unrecognized boardID 0x%X\n", (uint32_t)boardID);
    return;

    found:
    // Here, *table is our config.
    // Install the specified BoardID as canonical
    SetTivoConfig(kTivoConfigBoardID, boardID & 0xffffff00);
    // Copy the current table, except for boardID
    for (entry=*table; entry->selector; entry++) 
        if (entry->selector != kTivoConfigBoardID) 
            SetTivoConfig(entry->selector, entry->value);
}


