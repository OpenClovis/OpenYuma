/*
 * Copyright (c) 2008 - 2012, Andy Bierman, All Rights Reserved.
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.    
 */
#ifndef _H_procdefs
#define _H_procdefs
#define _INC_PROCDEFS
/*
 *  File: procdefs.h
 *
 *  Contains global constants and types used for portability
 *  defines UNIX/MSDOS variant strings
 *
 */

/* no user source files should include limits.h
 * they should include procdefs.h instead, 
 * and use the defined base types provided below
 */
#include    <limits.h>
#include    <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG
#define CPP_DEBUG           1
#endif  /* DEBUG */

#ifdef CPP_FULL_DEBUG
#define	    CPP_MTRACE	    1 
#define	    CPP_ICHK	    1
#define	    CPP_CHK_HEAP    1
#define     CPP_FULL_TRACE  1
#endif	/* CPP_FULL_DEBUG */

/* program environment and personality switches
 * SUN is the only supported unix platform at this time
 */
#if defined(SUN) || defined(LINUX) || defined(MACOSX)
#define	    CPV_UNIX		1
#define	    CPV_MSC		0
#define	    CPV_ALLOW_FAR	0
#define	    CPV_DOS		0
#else
/* !!! ASSUME MSC VERSION !!! */
#define	    CPV_UNIX		0
#define	    CPV_MSC		1
#define	    CPV_ALLOW_FAR	0
#define	    CPV_DOS		1
#endif	    /* SUN */


/* #define  CPP_NO_MACROS   0 -- ifdefs still used */
/* CPP_DEBUG assigned at the command line */
/* CPP_CHK_HEAP assigned at the command line */
/* CPP_ICHK assigned at the command line */

#if CPV_UNIX
#define	    NEWLN		0xa
#else
#define	    NEWLN		0xd
#endif	    /* CPV_UNIX */

/* only the module that contains 'main' should define _C_main */
#ifdef _C_main
#define PUBLIC 
#else
#define PUBLIC extern
#endif	    /* _C_main */

/* 16-bit word bit definitions */
#define		bit0	0x0001
#define		bit1	0x0002
#define		bit2	0x0004
#define		bit3	0x0008
#define		bit4	0x0010
#define		bit5	0x0020
#define		bit6	0x0040
#define		bit7	0x0080
#define		bit8	0x0100
#define		bit9	0x0200
#define		bit10	0x0400
#define		bit11	0x0800
#define		bit12	0x1000
#define		bit13	0x2000
#define		bit14	0x4000
#define		bit15	0x8000

/* 32 bit extended bit definitions */
#define		bit16	0x00010000
#define		bit17	0x00020000
#define		bit18	0x00040000
#define		bit19	0x00080000
#define		bit20	0x00100000
#define		bit21	0x00200000
#define		bit22	0x00400000
#define		bit23	0x00800000
#define		bit24	0x01000000
#define		bit25	0x02000000
#define		bit26	0x04000000
#define		bit27	0x08000000
#define		bit28	0x10000000
#define		bit29	0x20000000
#define		bit30	0x40000000
#define		bit31	0x80000000


#define m__setbit(F,B)  F |= (B)

#define m__clrbit(F,B)  F &= ~(B)

#define m__bitset(F,B)  F & B

#if CPV_UNIX
#define	    STATIC  
#define	    CONST
#define	    REG		register
#define	    REGVAR  
#define	    VOLATILE
#define	    FAR
#define	    NEAR
#define	    PASCAL
/* #define	    signed */
#define	    cdecl
#define	    memicmp	memcmp
#define	    strcmpi	strcasecmp
#define	    _strcmpi	strcasecmp
#define	    _strdup	strdup		
#define	    _chdir	chdir
#define     _getcwd	getcwd

#else
/*** CPV_DOS ***/
#define	    STATIC	static
#define	    CONST	const
#define	    REG		register
#define	    REGVAR	register
#define	    VOLATILE	volatile

#ifdef __STDC__
#ifndef WINDOWS
#define	    strcmpi	_strcmpi
#endif
#endif

#if CPV_ALLOW_FAR
#define	    FAR		far
#else
#define	    FAR 
#endif	    /* CPV_ALLOW_FAR */

#endif	    /* CPV_UNIX */

#ifndef __cplusplus
#ifndef min
#define	    min(A,B)	((A<B)?A:B)
#endif

#ifndef max
#define	    max(A,B)	((A>B)?A:B)
#endif
#endif

#ifdef CHAR_BIT
#define	    BITS_PER_BYTE	CHAR_BIT
#else
#define	    BITS_PER_BYTE	8
#endif	    /* CHAR_BIT */


/* assign debug flag */
#ifdef CPP_DEBUG
#define		DBG_STAMP	"DEBUG"
#else
#define		DBG_STAMP	"NODEBUG"
#endif	    /* CPP_DEBUG */

#if !CPV_UNIX
#ifndef WINDOWS
typedef		unsigned int		uint; 
#endif
#else
#include <sys/types.h>
#endif	    /* !CPV_UNIX */

#include <inttypes.h>
#include <stdbool.h>

typedef     uint8_t             uchar;
typedef		uint8_t             uint8;
typedef		uint16_t            uint16;
typedef		uint32_t            uint32;
typedef		uint64_t            uint64;
typedef		int8_t              int8;
typedef		int16_t             int16;
typedef		int32_t   		    int32;
typedef		int64_t        		int64;
typedef		uint8_t      		byte;
typedef		bool			    boolean;

#ifdef SUN413_GNU
typedef         unsigned long           ulong;
#endif

#if !CPV_UNIX
#ifndef __STDC__
#ifndef _WCHAR_T_DEFINED	/** MSC 7.0 **/
#define _WCHAR_T_DEFINED
typedef	unsigned short		wchar_t;
#endif	/* WCHAR_T_DEFINED */
#endif	/* __STDC__ */
#endif  /* !CPV_UNIX */

#ifndef WCHAR_MAX
#define		WCHAR_MAX		0xffff
#endif	    /* WCHAR_MAX */

#if CPV_UNIX	
typedef		double			LDBL;
#else
typedef		long double		LDBL;
#endif	    /* CPV_UNIX */

/* MSC symbolic constants have to be faked in unix */
#if CPV_UNIX
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2
#endif	    /* CPV_UNIX */

/* c-portability adjustment ...(avoid lint error)--version for runtine code */
#if !defined(FALSE) && !defined(TRUE)
#define		FALSE	 false
#define		TRUE	 true
#endif	    /* FALSE | TRUE */


/* memory for these vars in ncx/ncx.c */
extern uint32  malloc_cnt;
extern uint32  free_cnt;

#ifndef m__getMem
#define m__getMem(X)   malloc(X);malloc_cnt++
#endif		/* m__getMem */

#ifndef m__free
#define m__free(X)    do { if ( X ) { free(X); free_cnt++; } } while(0)
#endif		/* m__free */

#ifndef m__getObj
#define m__getObj(OBJ)	(OBJ *)malloc(sizeof(OBJ));malloc_cnt++
#endif		/* m__getObj */

#ifdef __cplusplus
}  /* end extern 'C' */
#endif

#endif		/* _H_procdefs */
