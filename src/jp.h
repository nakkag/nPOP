/*
 * nPOP
 *
 * jp.h (Japanese encode)
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_JP_H
#define _INC_JP_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
int IsDependenceString(TCHAR *buf);
void sjis_iso2022jp(unsigned char *buf,unsigned char *ret);
char *iso2022jp_sjis(char *buf, char *ret);

#endif
/* End of source */
