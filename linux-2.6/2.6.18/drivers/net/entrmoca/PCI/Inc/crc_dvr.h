/*******************************************************************************
*
* Src/crc_dvr.h
*
* Description: CRC computation related API
*
*******************************************************************************/

/*******************************************************************************
*                        Entropic Communications, Inc.
*                         Copyright (c) 2001-2008
*                          All rights reserved.
*******************************************************************************/

/*******************************************************************************
*
* Copyright (c) 1996 L. Peter Deutsch
*
* Permission is granted to copy and distribute this document for any
* purpose and without charge, including translations into other
* languages and incorporation into compilations, provided that the
* copyright notice and this notice are preserved, and that any
* substantive changes or deletions from the original are clearly
* marked.
*
* A pointer to the latest version of this and related documentation in
* HTML format can be found at the URL
* <ftp://ftp.uu.net/graphics/png/documents/zlib/zdoc-index.html>.
*
* Full Copyright Statement
*
* Copyright (C) IETF Trust (2008).
*
* This document is subject to the rights, licenses and restrictions
* contained in BCP 78 and at http://www.rfc-editor.org/copyright.html,
* and except as set forth therein, the authors retain all their rights.
*
* This document and the information contained herein are provided
* on an "AS IS" basis and THE CONTRIBUTOR, THE ORGANIZATION HE/SHE
* REPRESENTS OR IS SPONSORED BY (IF ANY), THE INTERNET SOCIETY AND
* THE INTERNET ENGINEERING TASK FORCE DISCLAIM ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY THAT
* THE USE OF THE INFORMATION HEREIN WILL NOT INFRINGE ANY RIGHTS OR
* ANY IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/

#ifndef _CRC_H_
#define _CRC_H_

#include "inctypes_dvr.h"

SYS_UINT32 compute_crc32(SYS_UINT8 *buf, SYS_UINT32 len, SYS_UINT32 crc);

#endif /* ! _CRC_H_ */
