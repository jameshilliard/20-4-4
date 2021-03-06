/* -mlong-double-64 compatibility mode for stdio functions.
   Copyright (C) 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _STDIO_H
# error "Never include <bits/stdio-ldbl.h> directly; use <stdio.h> instead."
#endif

__BEGIN_NAMESPACE_STD
__LDBL_REDIR_DECL (fprintf)
__LDBL_REDIR_DECL (printf)
__LDBL_REDIR_DECL (sprintf)
__LDBL_REDIR_DECL (vfprintf)
__LDBL_REDIR_DECL (vprintf)
__LDBL_REDIR_DECL (vsprintf)
__LDBL_REDIR_DECL (fscanf)
__LDBL_REDIR_DECL (scanf)
__LDBL_REDIR_DECL (sscanf)
__END_NAMESPACE_STD

#if defined __USE_BSD || defined __USE_ISOC99 || defined __USE_UNIX98
__BEGIN_NAMESPACE_C99
__LDBL_REDIR_DECL (snprintf)
__LDBL_REDIR_DECL (vsnprintf)
__END_NAMESPACE_C99
#endif

#ifdef	__USE_ISOC99
__BEGIN_NAMESPACE_C99
__LDBL_REDIR_DECL (vfscanf)
__LDBL_REDIR_DECL (vsscanf)
__LDBL_REDIR_DECL (vscanf)
__END_NAMESPACE_C99
#endif

#ifdef __USE_GNU
__LDBL_REDIR_DECL (vdprintf)
__LDBL_REDIR_DECL (dprintf)
__LDBL_REDIR_DECL (vasprintf)
__LDBL_REDIR_DECL (__asprintf)
__LDBL_REDIR_DECL (asprintf)
__LDBL_REDIR_DECL (obstack_printf)
__LDBL_REDIR_DECL (obstack_vprintf)
#endif

#if __USE_FORTIFY_LEVEL > 0 && !defined __cplusplus
__LDBL_REDIR_DECL (__sprintf_chk)
__LDBL_REDIR_DECL (__vsprintf_chk)
# if defined __USE_BSD || defined __USE_ISOC99 || defined __USE_UNIX98
__LDBL_REDIR_DECL (__snprintf_chk)
__LDBL_REDIR_DECL (__vsnprintf_chk)
# endif
# if __USE_FORTIFY_LEVEL > 1
__LDBL_REDIR_DECL (__fprintf_chk)
__LDBL_REDIR_DECL (__printf_chk)
__LDBL_REDIR_DECL (__vfprintf_chk)
__LDBL_REDIR_DECL (__vprintf_chk)
# endif
#endif
