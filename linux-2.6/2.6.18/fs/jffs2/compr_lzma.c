/*
 * JFFS2 LZMA Compression Interface.
 *
 * Copyright (C) 2010 TiVo Inc. All rights reserved.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Derived from compr_zlib.c
 *
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/jffs2.h>
#include <linux/LzmaDec.h>
#include <linux/LzmaEnc.h>
#include "compr.h"

static int initLzmaEncProps = 0;
static CLzmaEncProps encProps ;

static void *Malloc( void *ptr, size_t sz )
{
        (void) ptr;
        return kmalloc(sz, GFP_KERNEL);
}

static void Free( void *ptr, void *addr )
{
        (void) ptr;
        kfree(addr);
}
static ISzAlloc Alloc = { Malloc, Free};
static ISzAlloc AllocBig = { Malloc, Free};

static void LzmaEnc_InitProps()
{
        /*defaults as metioned in LzmaEnc.h*/
        encProps.level = 9;
        encProps.dictSize= (1 << 12); 
        encProps.lc = 3;
        encProps.lp = 0;
        encProps.pb = 2;
        encProps.algo = 1;
        encProps.fb = 32;
        encProps.btMode = 1;     
        encProps.numHashBytes = 4;
        encProps.mc = 32;
        encProps.writeEndMark = 0;
        encProps.numThreads = 2;

        initLzmaEncProps = 1;
}

int jffs2_lzma_compress(unsigned char *data_in, unsigned char *cpage_out,
		uint32_t *sourcelen, uint32_t *dstlen, void *model)
{
        SizeT propsSize = LZMA_PROPS_SIZE;
        SizeT srcLen;
        SRes res;
        ICompressProgress progress;
        int writeEndMark = 0;

	if (*dstlen < propsSize )
		return -1;

	srcLen = *dstlen - propsSize;

        if ( !initLzmaEncProps ){
                LzmaEnc_InitProps();
        }

        if ( (res=LzmaEncode( cpage_out+propsSize, &srcLen, data_in, 
                    *sourcelen, &encProps, cpage_out, &propsSize, 
                    writeEndMark, &progress, &Alloc, &AllocBig)) != SZ_OK ){
                return -1;
        }

	*dstlen = srcLen + propsSize;

	return 0;
}

int jffs2_lzma_decompress(unsigned char *data_in, unsigned char *cpage_out,
		uint32_t srclen, uint32_t destlen, void *model)
{
	SRes res;
	SizeT propsSize = LZMA_PROPS_SIZE;
	SizeT sLen;
	ELzmaStatus status;
	
	if (srclen < propsSize )
                return -1;

	sLen = srclen - propsSize;

	if ( (res = LzmaDecode( cpage_out, &destlen, data_in+propsSize, &sLen, 
                        data_in, propsSize, LZMA_FINISH_ANY, &status, &Alloc)) != SZ_OK ){
		return -1;
        }

	return 0;
}

static struct jffs2_compressor jffs2_lzma_comp = {
	.priority = JFFS2_LZMA_PRIORITY,
	.name = "lzma",
#ifdef CONFIG_JFFS2_LZMA
	.disabled = 0,
#else
	.disabled = 1,
#endif
	.compr = JFFS2_COMPR_LZMA,
	.compress = &jffs2_lzma_compress,
	.decompress = &jffs2_lzma_decompress,
};

int jffs2_lzma_init(void)
{
	return jffs2_register_compressor(&jffs2_lzma_comp);
}

void jffs2_lzma_exit(void)
{
	jffs2_unregister_compressor(&jffs2_lzma_comp);
}
