/*
 * nPOP
 *
 * Memory.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include <windows.h>
#include <tchar.h>

/* Define */

/* Global Variables */

/* Local Function Prototypes */
#ifdef _DEBUG
static long all_alloc_size = 0;

//#define MEM_CHECK
#ifdef MEM_CHECK
#define ADDRESS_CNT		100000
#define DEBUG_ADDRESS	0
static long address[ADDRESS_CNT];
static int address_index;
#endif	// MEM_CHECK

#endif	// _DEBUG

/*
 * mem_alloc - �o�b�t�@���m��
 */
void *mem_alloc(const DWORD size)
{
#ifdef _DEBUG
	void *mem;

	mem = HeapAlloc(GetProcessHeap(), 0, size);
	all_alloc_size += HeapSize(GetProcessHeap(), 0, mem);
#ifdef MEM_CHECK
	if (address_index < ADDRESS_CNT) {
		if (address_index == DEBUG_ADDRESS) {
			address[address_index] = (long)mem;
		} else {
			address[address_index] = (long)mem;
		}
		address_index++;
	}
#endif	// MEM_CHECK
	return mem;
#else	// _DEBUG
	return HeapAlloc(GetProcessHeap(), 0, size);
#endif	// _DEBUG
}

/*
 * mem_calloc - ���������ăo�b�t�@���m��
 */
void *mem_calloc(const DWORD size)
{
#ifdef _DEBUG
	void *mem;

	mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	if (mem == NULL) {
		return mem;
	}
	all_alloc_size += HeapSize(GetProcessHeap(), 0, mem);
#ifdef MEM_CHECK
	if (address_index < ADDRESS_CNT) {
		if (address_index == DEBUG_ADDRESS) {
			address[address_index] = (long)mem;
		} else {
			address[address_index] = (long)mem;
		}
		address_index++;
	}
#endif	// MEM_CHECK
	return mem;
#else	// _DEBUG
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
#endif	// _DEBUG
}

/*
 * mem_free - �o�b�t�@�����
 */
void mem_free(void **mem)
{
	if (*mem != NULL) {
#ifdef _DEBUG
		all_alloc_size -= HeapSize(GetProcessHeap(), 0, *mem);
#ifdef MEM_CHECK
		{
			int i;

			for (i = 0; i < ADDRESS_CNT; i++) {
				if (address[i] == (long)*mem) {
					address[i] = 0;
					break;
				}
			}
		}
#endif	// MEM_CHECK
#endif	// _DEBUG
		HeapFree(GetProcessHeap(), 0, *mem);
		*mem = NULL;
	}
}

/*
 * mem_debug - ���������̕\��
 */
#ifdef _DEBUG
void mem_debug(void)
{
	TCHAR buf[256];

	if (all_alloc_size == 0) {
		return;
	}

	wsprintf(buf, TEXT("Memory leak: %lu bytes"), all_alloc_size);
	MessageBox(NULL, buf, TEXT("debug"), 0);
#ifdef MEM_CHECK
	{
		int i;

		for (i = 0; i < ADDRESS_CNT; i++) {
			if (address[i] != 0) {
				wsprintf(buf, TEXT("leak address: %u, %lu"), i, address[i]);
				MessageBox(NULL, buf, TEXT("debug"), 0);
				break;
			}
		}
	}
#endif	// MEM_CHECK
}
#endif	// _DEBUG
/* End of source */
