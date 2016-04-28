/*
 * Copyright (C) 2012 TiVo Inc. All Rights Reserved.
 *
 * Module linker script based on scripts/module-common.lds.
 * This script is used during the linking stage of the TiVo drivers.
 */
SECTIONS {
	/DISCARD/ : { *(.discard) }

	__ksymtab		: { *(SORT(___ksymtab+*)) }
	__ksymtab_gpl		: { *(SORT(___ksymtab_gpl+*)) }
	__ksymtab_unused	: { *(SORT(___ksymtab_unused+*)) }
	__ksymtab_unused_gpl	: { *(SORT(___ksymtab_unused_gpl+*)) }
	__ksymtab_gpl_future	: { *(SORT(___ksymtab_gpl_future+*)) }
	__kcrctab		: { *(SORT(___kcrctab+*)) }
	__kcrctab_gpl		: { *(SORT(___kcrctab_gpl+*)) }
	__kcrctab_unused	: { *(SORT(___kcrctab_unused+*)) }
	__kcrctab_unused_gpl	: { *(SORT(___kcrctab_unused_gpl+*)) }
	__kcrctab_gpl_future	: { *(SORT(___kcrctab_gpl_future+*)) }
}
