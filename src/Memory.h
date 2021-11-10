/*
 * nPOP
 *
 * Memory.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_MEMORY_H
#define _INC_MEMORY_H

/* Include Files */
#include <windows.h>

/* Define */
#ifndef CopyMemory
#define CopyMemory					memcpy
#endif
#ifndef MoveMemory
#define MoveMemory					memmove
#endif
#ifndef ZeroMemory
#define ZeroMemory(p, len)			(memset(p, 0, len))
#endif
#ifndef FillMemory
#define FillMemory(p, len, fill)	(memset(p, fill, len))
#endif

/* Struct */

/* Function Prototypes */
void *mem_alloc(const DWORD size);
void *mem_calloc(const DWORD size);
void mem_free(void **mem);
#ifdef _DEBUG
void mem_debug(void);
#endif

#endif
/* End of source */
