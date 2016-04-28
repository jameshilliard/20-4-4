/*******************************************************************************
*
* Common/Inc/inctypes_dvr.h
*
* Description: System definitions
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

#ifndef __INCTYPES_H__
#define __INCTYPES_H__

/*******************************************************************************
*                             # d e f i n e s                                  *
********************************************************************************/

//  System Constants
#define SYS_TRUE     1
#define SYS_FALSE    0
#define SYS_NULL     0
#define SYS_SUCCESS  0

// NOTE: The error constants defined below MUST be the same value as the 
//       corresponding error codes defined by your specific OS.  If not, then
//       change the values of the defines below to match your specific OS error
//       values.
// 
//       Entropic has defined the error constants below to match the OS we use in
//       our evaluation platforms and may differ from the error values defined by 
//       your specific OS.
#define SYS_INPUT_OUTPUT_ERROR     5
#define SYS_OUT_OF_MEMORY_ERROR    12
#define SYS_PERMISSION_ERROR       13
#define SYS_INVALID_ADDRESS_ERROR  14
#define SYS_INVALID_ARGUMENT_ERROR 22
#define SYS_OUT_OF_SPACE_ERROR     28
#ifdef CONFIG_TIVO
#define SYS_DIR_NOT_EMPTY_ERROR    93  /* ENOTEMPTY */
#define SYS_BAD_MSG_TYPE_ERROR     35  /* ENOMSG    */
#define SYS_TIMEOUT_ERROR         145  /* ETIMEDOUT */
#else
#define SYS_DIR_NOT_EMPTY_ERROR    39
#define SYS_BAD_MSG_TYPE_ERROR     42
#define SYS_TIMEOUT_ERROR         110
#endif

#define DOWHILE0(x)  do { x; } while(0)

#define INCTYPES_MIN(x,y)		  ((x) < (y) ? (x) : (y))

/*******************************************************************************
*                       G l o b a l   D a t a   T y p e s                      *
********************************************************************************/

// System Variable Types
typedef void            SYS_VOID;
typedef void*           SYS_VOID_PTR;

typedef char            SYS_CHAR;
typedef char*           SYS_CHAR_PTR;
typedef signed char     SYS_INT8;
typedef signed char*    SYS_INT8_PTR;
typedef unsigned char   SYS_UCHAR;
typedef unsigned char*  SYS_UCHAR_PTR;
typedef unsigned char   SYS_UINT8;
typedef unsigned char*  SYS_UINT8_PTR;
typedef unsigned char   SYS_BOOLEAN; 

/** Used in places where the type is a boolean type but 32 bit words reduce
 * code space by avoiding the need for various masking */
typedef unsigned char   SYS_BOOLEAN32; 

typedef unsigned short  SYS_UINT16;
typedef unsigned short* SYS_UINT16_PTR;
typedef short           SYS_INT16;
typedef short*          SYS_INT16_PTR;

typedef int             SYS_INT32;
typedef int*            SYS_INT32_PTR;
typedef unsigned int    SYS_UINT32;
typedef unsigned int*   SYS_UINT32_PTR;

typedef long            SYS_LONG;
typedef long*           SYS_LONG_PTR;
typedef unsigned long   SYS_ULONG;
typedef unsigned long*  SYS_ULONG_PTR;

#if defined(_LP64)
typedef unsigned long   SYS_UINTPTR;
#else
typedef unsigned int    SYS_UINTPTR;
#endif

typedef struct {
	SYS_INT32 I;
	SYS_INT32 Q;
} SYS_COMPLEX_INT32;

typedef struct {
	SYS_UINT32 		hi;
	SYS_UINT32 		lo;
} mac_addr_t;

typedef void (*SYS_VDFCVD_PTR) (void *);

/** Query if the mac address represents a multicast */
#define INCTYPES_MAC_ADDR_IS_MULTICAST( _mac_addr ) \
  (((_mac_addr).hi & 0x01000000) ? SYS_TRUE : SYS_FALSE)


/** Query if two mac address match (low order bits masked off) */
#define INCTYPES_MAC_ADDR_MATCH( _mac_addr_a, _mac_addr_b )              \
  ((                                                                     \
      (_mac_addr_a).hi == (_mac_addr_b).hi                               \
   ) &&                                                                  \
   (                                                                     \
      ((_mac_addr_a).lo & 0xFFFF0000) == ((_mac_addr_b).lo & 0xFFFF0000) \
   ))


typedef struct {
    SYS_UINT32 hi;  // Bytes 5432 of flow id
    SYS_UINT32 lo;  // Bytes 10xx of flow id, bottom 2 bytes zeroed always
} clink_flow_id_t;

typedef clink_flow_id_t flow_id_t;

/* persistent name for a clink node, may differ from devices mac address */
typedef mac_addr_t guid_mac_t;

typedef mac_addr_t clink_guid_t;

typedef SYS_UINT8 clink_node_id_t;

typedef SYS_UINT32 clink_nodemask_t;

typedef SYS_UINT8  moca_version_t;

#define MOCA_FROM_NPS(_nps) ((moca_version_t)(((_nps) >> 24) & 0xFF))

#define MOCA_01  ((moca_version_t)0x01)
#define MOCA_10  ((moca_version_t)0x10)
#define MOCA_11  ((moca_version_t)0x11)
#define MOCA_1C  ((moca_version_t)0x1C) /* Future compatibility testing */

/** Used internally to indicate that the true node protocol support field
 * values are not yet known. */
#define MOCA_UNSPECIFIED  ((moca_version_t)0x02)

#define MOCA_TO_NPS(moca_version) ((moca_version) << 24)

#define MOCA_VERSION_SPECIFIED(nps) (MOCA_FROM_NPS(nps)!=MOCA_UNSPECIFIED)

/** Causes a compile time error if a condition is not true at compile time
 * from within a function definition.
 * 
 * Note that the arguments to this must be evaluatable at compile time. If
 * this check fails, you will see an error like: 'the size of an array must
 * be greater than zero'. 
 */
#define INCTYPES_COMPILE_TIME_ASSERT(_condition)      \
do                                                    \
{                                                     \
    extern int inctypes_assert[(_condition) ? 1 : 0]; \
    (void)inctypes_assert;                            \
} while (0)


/** Given a reference to an array of definite size, return num elements */
#define INCTYPES_ARRAY_LEN(_arrayVar) ( sizeof(_arrayVar) / sizeof(*_arrayVar) )

/**
 * A special macro that when passed the name of a structure and a size (in 
 * units of size_t (i.e. bytes) will cause a compilation error if the 
 * structure is larger than the specified size.  
 * 
 * This is used as a verification against structures declared in C code
 * exceeding the size of their MoCA counterparts (indicating bad assumptions
 * about byte packing or bit packing).
 */
#define INCTYPES_VERIFY_STRUCT_LESS_OR_EQUAL(_struct,_size)    \
struct _struct##_size_verify_lte_t                             \
{                                                              \
    int lo[(int)1 + (int)(_size) - (int)sizeof(_struct)];      \
};

/**
 * A special macro that when passed the name of a structure and a size (in 
 * units of size_t (i.e. bytes) will cause a compilation error if the 
 * structure is smaller than the specified size.  
 * 
 * This is used as a verification against structures declared in C code
 * exceeding the size of their MoCA counterparts (indicating bad assumptions
 * about byte packing or bit packing).
 */
#define INCTYPES_VERIFY_STRUCT_GREATER_OR_EQUAL(_struct,_size) \
struct _struct##_size_verify_gte_t                             \
{                                                              \
    int hi[(int)1 + (int)sizeof(_struct) - (int)(_size)];      \
}

/**
 * A special macro that when passed the name of a structure and a size (in 
 * units of size_t (i.e. bytes) will cause a compilation error if the 
 * structure is not equal to the specified size.  
 * 
 * This is used as a verification against structures declared in C code
 * exceeding the size of their MoCA counterparts (indicating bad assumptions
 * about byte packing or bit packing).
 */
#define INCTYPES_VERIFY_STRUCT_SIZE(_struct,_size)          \
    INCTYPES_VERIFY_STRUCT_LESS_OR_EQUAL(_struct,_size)     \
    INCTYPES_VERIFY_STRUCT_GREATER_OR_EQUAL(_struct,_size)

/**
 * A special macro that when passed the name of an enumeration or define
 * will cause a compilation error if the value is larger than another 
 * value.
 */
#define INCTYPES_VERIFY_TOKEN_LESS_THAN(_token,_size) \
struct _token##_size_verify_lt_t                      \
{                                                     \
    int lo[(int)(_size) - (int)(_token)];             \
};

/**
 * Utility to safely copy pointers to structures of the same size. This only 
 * works in evironments implementing memcpy.
 */
#define INCTYPES_SAFE_PTR_COPY(_destPtr,_srcPtr)                             \
do                                                                           \
{                                                                            \
    INCTYPES_COMPILE_TIME_ASSERT(sizeof(*(_destPtr)) == sizeof(*(_srcPtr))); \
                                                                             \
    memcpy((_destPtr),(_srcPtr),sizeof(*(_destPtr)));                        \
} while (0)

/**
 * Utility to safely copy variables of the same size.  This only works in 
 * evironments implementing memcpy.
 */
#define INCTYPES_SAFE_VAR_COPY(_dest,_src) \
        INCTYPES_SAFE_PTR_COPY((&(_dest)),(&(_src)))

/**
 * Utility to safely zero a structure referred to by a typed pointer. This only 
 * works in evironments implementing memset.
 */
#define INCTYPES_SAFE_PTR_ZERO(_destPtr)        \
    memset((_destPtr),0,sizeof(*(_destPtr)))    

/**
 * Utility to safely zero a variable. This only works in evironments 
 * implementing memset.
 */
#define INCTYPES_SAFE_VAR_ZERO(_destVar)        \
    memset(&(_destVar),0,sizeof(_destVar))



/*******************************************************************************
*                        G l o b a l   C o n s t a n t s                       *
********************************************************************************/

/* None */

/*******************************************************************************
*                           G l o b a l   D a t a                              *
********************************************************************************/

/* None */

/*******************************************************************************
*                            I n t e r f a c e s                               *
********************************************************************************/


#endif /* __INCTYPES_H__ */


