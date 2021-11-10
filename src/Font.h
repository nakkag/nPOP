/*
 * nPOP
 *
 * Font.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_FONT_H
#define _INC_FONT_H

/* Include Files */
#include <windows.h>

/* Define */

/* Struct */
typedef struct _FONT_INFO {
	TCHAR *name;
	int size;
	int weight;
	int italic;
	int charset;
} FONT_INFO;

/* Function Prototypes */
HFONT font_create(HWND hWnd, FONT_INFO *fi);
HFONT font_copy(const HFONT hfont);
int font_get_charset(const HDC hdc);
BOOL font_select(const HWND hWnd, FONT_INFO *fi);

#endif
/* End of source */
