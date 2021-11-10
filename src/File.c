/*
 * nPOP
 *
 * File.c
 *
 * Copyright (C) 1996-2020 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include "General.h"
#include "Memory.h"
#include "String.h"

/* Define */

/* Global Variables */
extern OPTION op;

extern HINSTANCE hInst;  // Local copy of hInstance
extern TCHAR *AppDir;
extern TCHAR *DataDir;
extern MAILBOX *MailBox;

/* Local Function Prototypes */
static int file_get_mail_count(char *buf, long Size);
static BOOL file_save_address_item(HANDLE hFile, MAILITEM *tpMailItem);

/*
 * log_init - ���O�̋�؂�̕ۑ�
 */
BOOL log_init(TCHAR *fpath, TCHAR *fname, TCHAR *buf)
{
#define LOG_SEP				TEXT("\r\n-------------------------------- ")
	TCHAR fDay[BUF_SIZE];
	TCHAR fTime[BUF_SIZE];
	TCHAR *p;
	BOOL ret;

	if (GetDateFormat(0, 0, NULL, NULL, fDay, BUF_SIZE - 1) == 0) {
		return FALSE;
	}
	if (GetTimeFormat(0, 0, NULL, NULL, fTime, BUF_SIZE - 1) == 0) {
		return FALSE;
	}
	p = mem_alloc(sizeof(TCHAR) * (lstrlen(LOG_SEP) + lstrlen(fDay) + 1 + lstrlen(fTime) + 2 + lstrlen(buf) + 2));
	if (p == NULL) {
		return FALSE;
	}
	str_join_t(p, LOG_SEP, fDay, TEXT(" "), fTime, TEXT(" ("), buf, TEXT(")"), (TCHAR *)-1);
	ret = log_save(fpath, fname, p);
	mem_free(&p);
	return ret;
}

/*
 * log_save - ���O�̕ۑ�
 */
BOOL log_save(TCHAR *fpath, TCHAR *fname, TCHAR *buf)
{
	HANDLE hFile;
	TCHAR path[BUF_SIZE];
	DWORD ret;

	// �t�@�C���ɕۑ�
	wsprintf(path, TEXT("%s%s"), fpath, fname);

	// �ۑ�����t�@�C�����J��
	hFile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		return FALSE;
	}
	SetFilePointer(hFile, 0, NULL, FILE_END);

	if (file_write_utf8(hFile, buf) == FALSE) {
		CloseHandle(hFile);
		return FALSE;
	}
	if (*(buf + lstrlen(buf) - 1) != TEXT('\n')) {
		if (WriteFile(hFile, "\r\n", 2, &ret, NULL) == FALSE) {
			CloseHandle(hFile);
			return FALSE;
		}
	}
	CloseHandle(hFile);
	return TRUE;
}

/*
 * log_clear - ���O�̃N���A
 */
BOOL log_clear(TCHAR *fpath, TCHAR *fname)
{
	TCHAR path[BUF_SIZE];

	wsprintf(path, TEXT("%s%s"), fpath, fname);
	return DeleteFile(path);
}

/*
 * dir_check - �f�B���N�g�����ǂ����`�F�b�N
 */
BOOL dir_check(TCHAR *path)
{
	WIN32_FIND_DATA FindData;
	HANDLE hFindFile;

	if ((hFindFile = FindFirstFile(path, &FindData)) == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	FindClose(hFindFile);

	if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		return TRUE;
	}
	return FALSE;
}

/*
 * dir_delete - �f�B���N�g�����̃t�@�C�����폜
 */
BOOL dir_delete(TCHAR *Path, TCHAR *file)
{
	WIN32_FIND_DATA FindData;
	HANDLE hFindFile;
	TCHAR sPath[BUF_SIZE];
	TCHAR buf[BUF_SIZE];

	if (Path == NULL || *Path == TEXT('\0')) {
		return FALSE;
	}
	wsprintf(sPath, TEXT("%s\\%s"), Path, file);

	if ((hFindFile = FindFirstFile(sPath, &FindData)) == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	do{
		if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			wsprintf(buf, TEXT("%s\\%s"), Path, FindData.cFileName);
			DeleteFile(buf);
		}
	} while (FindNextFile(hFindFile, &FindData) == TRUE);
	FindClose(hFindFile);
	return TRUE;
}

/*
 * filename_conv - �t�@�C�����ɂł��Ȃ������� _ �ɕϊ�����
 */
void filename_conv(TCHAR *buf)
{
	TCHAR *p = buf, *r;

	while (*p != TEXT('\0')) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE && *(p + 1) != TEXT('\0')) {
			// 2�o�C�g�R�[�h
			p += 2;
			continue;
		}
#endif
		// �󔒂̏���
		if (*p == TEXT(' ') || *p == TEXT('\t')) {
			r = p + 1;
			for (; *r == TEXT(' ') || *r == TEXT('\t'); r++);
			if (p + 1 != r) {
				lstrcpy(p + 1, r);
			}
		}
		// �t�@�C�����ɂł��Ȃ������͎w��̕����ɕϊ�
		switch (*p) {
		case TEXT('\\'): case TEXT('/'): case TEXT(':'):
		case TEXT(','): case TEXT(';'): case TEXT('*'):
		case TEXT('?'): case TEXT('\"'): case TEXT('<'):
		case TEXT('>'): case TEXT('|'):
			*p = TEXT('_');
			break;
		}
		p++;
	}
}

/*
 * filename_select - �t�@�C�����̎擾
 */
BOOL filename_select(HWND hWnd, TCHAR *ret, TCHAR *DefExt, TCHAR *filter, BOOL OpenSave)
{
	OPENFILENAME of;
	TCHAR path[BUF_SIZE];

	// �t�@�C���ɕۑ�
	lstrcpy(path, ret);
	ZeroMemory(&of, sizeof(OPENFILENAME));
	of.lStructSize = sizeof(OPENFILENAME);
	of.hInstance = hInst;
	of.hwndOwner = hWnd;
	if (filter == NULL) {
		of.lpstrFilter = STR_FILE_FILTER;
	} else {
		of.lpstrFilter = filter;
	}
	of.nFilterIndex = 1;
	if (OpenSave == TRUE) {
		of.lpstrTitle = STR_TITLE_OPEN;
	} else {
		of.lpstrTitle = STR_TITLE_SAVE;
	}
	of.lpstrFile = path;
	of.nMaxFile = BUF_SIZE - 1;
	of.lpstrDefExt = DefExt;
	of.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	// �t�@�C���I���_�C�A���O��\������
	if (OpenSave == TRUE) {
		of.Flags |= OFN_FILEMUSTEXIST;
		if (GetOpenFileName((LPOPENFILENAME)&of) == FALSE) {
			return FALSE;
		}
	} else {
		if (GetSaveFileName((LPOPENFILENAME)&of) == FALSE) {
			return FALSE;
		}
	}
	lstrcpy(ret, path);
	return TRUE;
}

/*
 * file_get_size - �t�@�C���̃T�C�Y���擾����
 */
long file_get_size(TCHAR *FileName)
{
	WIN32_FIND_DATA FindData;
	HANDLE hFindFile;

	if ((hFindFile = FindFirstFile(FileName, &FindData)) == INVALID_HANDLE_VALUE) {
		return -1;
	}
	FindClose(hFindFile);

	if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		// �f�B���N�g���ł͂Ȃ��ꍇ�̓T�C�Y��Ԃ�
		return (long)FindData.nFileSizeLow;
	}
	return -1;
}

/*
 * file_get_mail_count - ���[���ꗗ�̕����񂩂烁�[���̐����擾
 */
static int file_get_mail_count(char *buf, long Size)
{
	char *p, *r, *t;
	int ret = 0;

	p = buf;
	while (Size > p - buf && *p != '\0') {
		for (t = r = p; Size > r - buf && *r != '\0'; r++) {
			if (*r == '\r' && *(r + 1) == '\n') {
				if (*t == '.' && (r - t) == 1) {
					break;
				}
				t = r + 2;
			}
		}
		p = r;
		if (Size > p - buf && *p != '\0') {
			p += 2;
		}
		ret++;
	}
	return ret;
}

/*
 * file_read - �t�@�C����ǂݍ���
 */
char *file_read(TCHAR *path, long FileSize)
{
	HANDLE hFile;
	DWORD ret;
	char *cBuf;

	// �t�@�C�����J��
	hFile = CreateFile(path, GENERIC_READ, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		return NULL;
	}
	cBuf = (char *)mem_calloc(FileSize + 1);
	if (cBuf == NULL) {
		CloseHandle(hFile);
		return NULL;
	}

	if (ReadFile(hFile, cBuf, FileSize, &ret, NULL) == FALSE) {
		mem_free(&cBuf);
		CloseHandle(hFile);
		return NULL;
	}
	CloseHandle(hFile);
	return cBuf;
}

/*
 * file_read_select - �t�@�C����I�����ēǂݍ���
 */
BOOL file_read_select(HWND hWnd, TCHAR **buf)
{
	TCHAR path[BUF_SIZE];
	char *cBuf;
	long FileSize;

	*buf = NULL;

	// �t�@�C�����擾
	lstrcpy(path, TEXT("*.txt"));
	if (filename_select(hWnd, path, TEXT("txt"), STR_TEXT_FILTER, TRUE) == FALSE) {
		return TRUE;
	}

	// �t�@�C���̃T�C�Y���擾
	FileSize = file_get_size(path);
	if (FileSize <= 0) {
		return TRUE;
	}

	// �t�@�C����ǂݍ���
	SwitchCursor(FALSE);
	cBuf = file_read(path, FileSize);
	if (cBuf == NULL) {
		SwitchCursor(TRUE);
		return FALSE;
	}

#ifdef UNICODE
	int len;
	int bom = 0;
	// BOM���X�L�b�v
	if (FileSize > 3 && *cBuf == 0xEF && *(cBuf + 1) == 0xBB && *(cBuf + 2) == 0xBF) {
		bom = 3;
	}
	// UTF-8����UTF-16�ɕϊ�
	len = MultiByteToWideChar(CP_UTF8, 0, cBuf + bom, -1, NULL, 0);
	if ((*buf = (WCHAR*)mem_alloc(sizeof(WCHAR) * len)) == NULL) {
		mem_free(&cBuf);
		SwitchCursor(TRUE);
		return FALSE;
	}
	MultiByteToWideChar(CP_UTF8, 0, cBuf + bom, -1, *buf, len);
	mem_free(&cBuf);
#else
	*buf = cBuf;
#endif
	SwitchCursor(TRUE);
	return TRUE;
}

/*
 * file_read_mailbox - �t�@�C�����烁�[���A�C�e���̍쐬
 */
BOOL file_read_mailbox(TCHAR *FileName, MAILBOX *tpMailBox)
{
	MAILITEM *tpMailItem;
#ifndef _NOFILEMAP
	HANDLE hFile;
	HANDLE hMapFile;
#endif
	TCHAR path[BUF_SIZE];
	char *p, *r, *s, *t;
	char *FileBuf;
	long FileSize;
	int i;
	int bom = 0;

	SetCurrentDirectory(AppDir);

	str_join_t(path, DataDir, FileName, (TCHAR *)-1);

	FileSize = file_get_size(path);
	if (FileSize <= 0) {
		return TRUE;
	}
#ifdef _NOFILEMAP
	FileBuf = file_read(path, FileSize);
	if (FileBuf == NULL) {
		return FALSE;
	}
#else	// _NOFILEMAP
	// �t�@�C�����J��
	hFile = CreateFile(path, GENERIC_READ, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL) {
		return FALSE;
	}

	hMapFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMapFile == NULL) {
		CloseHandle(hFile);
		return FALSE;
	}
	FileBuf = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
	if (FileBuf == NULL) {
		CloseHandle(hMapFile);
		CloseHandle(hFile);
		return FALSE;
	}
#endif	// _NOFILEMAP

	// BOM���X�L�b�v
	if (FileSize > 3 && (BYTE)(*FileBuf) == 0xEF && (BYTE)(*(FileBuf + 1)) == 0xBB && (BYTE)(*(FileBuf + 2)) == 0xBF) {
		bom = 3;
	}

	// ���[�������̃��������m��
	tpMailBox->AllocCnt = tpMailBox->MailItemCnt = file_get_mail_count(FileBuf + bom, FileSize - bom);
	tpMailBox->tpMailItem = (MAILITEM **)mem_calloc(sizeof(MAILITEM *) * tpMailBox->MailItemCnt);
	if (tpMailBox->tpMailItem == NULL) {
		tpMailBox->AllocCnt = tpMailBox->MailItemCnt = 0;
#ifdef _NOFILEMAP
		mem_free(&FileBuf);
#else	// _NOFILEMAP
		UnmapViewOfFile(FileBuf);
		CloseHandle(hMapFile);
		CloseHandle(hFile);
#endif	// _NOFILEMAP
		return FALSE;
	}

	i = 0;
	p = FileBuf + bom;
	FileSize -= bom;
	while (FileSize > p - FileBuf && *p != '\0') {
		// �w�b�_���烁�[���A�C�e�����쐬
		if (bom == 3) {
			tpMailItem = *(tpMailBox->tpMailItem + i) = item_string_to_item_utf8(tpMailBox, p);
		}
		else {
			tpMailItem = *(tpMailBox->tpMailItem + i) = item_string_to_item(tpMailBox, p);
		}

		// Body�ʒu�̎擾
		p = GetBodyPointa(p);
		if (p == NULL) {
			break;
		}
		// ���[���̏I���̈ʒu���擾
		for (t = r = p; *r != '\0'; r++) {
			if (*r == '\r' && *(r + 1) == '\n') {
				if (*t == '.' && (r - t) == 1) {
					t -= 2;
					for (; FileSize > r - FileBuf && (*r == '\r' || *r == '\n'); r++);
					break;
				}
				t = r + 2;
			}
		}
		if (tpMailItem != NULL) {
			// Body���R�s�[
			if ((t - p) > 0) {
				tpMailItem->Body = (char *)mem_alloc(sizeof(char) * (t - p + 1));
				if (tpMailItem->Body != NULL) {
					for (s = tpMailItem->Body; p < t; p++, s++) {
						*s = *p;
					}
					*s = '\0';
				}
			}
			if (tpMailItem->Body == NULL && tpMailItem->MailStatus < ICON_SENDMAIL) {
				if (tpMailItem->Download == TRUE) {
					tpMailItem->Body = (char *)mem_alloc(sizeof(char));
					if (tpMailItem->Body != NULL) {
						*tpMailItem->Body = '\0';
					}
				} else {
					tpMailItem->MailStatus = ICON_NON;
					if (tpMailItem->Status != ICON_DOWN && tpMailItem->Status != ICON_DEL && tpMailItem->Status != ICON_SEND) {
						tpMailItem->Status = ICON_NON;
					}
					tpMailItem->Download = FALSE;
				}
			}
		}
		p = r;
		i++;
	}

#ifdef _NOFILEMAP
	mem_free(&FileBuf);
#else	// _NOFILEMAP
	UnmapViewOfFile(FileBuf);
	CloseHandle(hMapFile);
	CloseHandle(hFile);
#endif	// _NOFILEMAP
	return TRUE;
}

/*
 * file_read_address_book - �t�@�C������A�h���X����ǂݍ���
 */
int file_read_address_book(TCHAR* FileName, MAILBOX* tpMailBox)
{
	MAILITEM* tpMailItem;
	TCHAR path[BUF_SIZE];
	TCHAR* MemFile, * AllocBuf = NULL;
	TCHAR* p, * r, * s;
	char* FileBuf;
	long FileSize;
	int LineCnt = 0;
	int i;
	int bom = 0;

	SetCurrentDirectory(AppDir);

	str_join_t(path, DataDir, FileName, (TCHAR*)-1);

	FileSize = file_get_size(path);
	if (FileSize < 0) {
		return 1;
	}
	if (FileSize == 0) {
		return 1;
	}
	FileBuf = file_read(path, FileSize);
	if (FileBuf == NULL) {
		return -1;
	}

	// BOM���X�L�b�v
	if (FileSize >= 3 && (BYTE)(*FileBuf) == 0xEF && (BYTE)(*(FileBuf + 1)) == 0xBB && (BYTE)(*(FileBuf + 2)) == 0xBF) {
		bom = 3;
	}
	if (bom == 3) {
		if (FileSize == 3) {
			mem_free(&FileBuf);
			return 1;
		}
#ifdef UNICODE
		// UTF-8����UTF-16�ɕϊ�
		int len = MultiByteToWideChar(CP_UTF8, 0, FileBuf + bom, -1, NULL, 0);
		if ((MemFile = (WCHAR*)mem_alloc(sizeof(WCHAR) * len)) == NULL) {
			mem_free(&FileBuf);
			return -1;
		}
		MultiByteToWideChar(CP_UTF8, 0, FileBuf + bom, -1, MemFile, len);
		mem_free(&FileBuf);
		FileSize = len - bom;
#else
		// UTF-8����UTF-16�ɕϊ�
		WCHAR* wbuf;
		int len = MultiByteToWideChar(CP_UTF8, 0, FileBuf + bom, -1, NULL, 0);
		if ((wbuf = (WCHAR*)mem_alloc(sizeof(WCHAR) * len)) == NULL) {
			mem_free(&FileBuf);
			return -1;
		}
		MultiByteToWideChar(CP_UTF8, 0, FileBuf + bom, -1, wbuf, len);
		mem_free(&FileBuf);
		// UTF-16����ASCII�ɕϊ�
		len = WideCharToMultiByte(CP_ACP, 0, wbuf, -1, NULL, 0, NULL, NULL);
		if ((AllocBuf = (BYTE*)mem_alloc(sizeof(BYTE) * len)) == NULL) {
			mem_free(&wbuf);
			return -1;
		}
		WideCharToMultiByte(CP_ACP, 0, wbuf, -1, AllocBuf, len, NULL, NULL);
		mem_free(&wbuf);
		MemFile = AllocBuf;
		FileSize = len - bom;
#endif
	}
	else {
		TCHAR backup_path[MAX_PATH];
		wsprintf(backup_path, TEXT("%s.back"), FileName);
		CopyFile(FileName, backup_path, FALSE);
#ifdef UNICODE
		// UNICODE�ɕϊ�
		int len = char_to_tchar_size(FileBuf);
		MemFile = (TCHAR*)mem_alloc(sizeof(TCHAR) * (len + 1));
		if (MemFile == NULL) {
			mem_free(&FileBuf);
			return -1;
		}
		char_to_tchar(FileBuf, MemFile, len);
		mem_free(&FileBuf);
		FileSize = len;
#else	// UNICODE
		MemFile = (TCHAR*)FileBuf;
#endif	// UNICODE
	}
	// �s���̃J�E���g
	for (LineCnt = 0, p = MemFile; *p != TEXT('\0'); p++) {
		if (*p == TEXT('\n')) {
			LineCnt++;
		}
	}
	if (LineCnt == 0) {
		mem_free(&MemFile);
		return 1;
	}
	tpMailBox->AllocCnt = tpMailBox->MailItemCnt = LineCnt;
	tpMailBox->tpMailItem = (MAILITEM**)mem_calloc(sizeof(MAILITEM*) * tpMailBox->MailItemCnt);
	if (tpMailBox->tpMailItem == NULL) {
		tpMailBox->AllocCnt = tpMailBox->MailItemCnt = 0;
		mem_free(&MemFile);
		return -1;
	}
	i = 0;
	p = MemFile;
	while (FileSize > p - MemFile && *p != TEXT('\0')) {
		tpMailItem = *(tpMailBox->tpMailItem + i) = (MAILITEM*)mem_calloc(sizeof(MAILITEM));

		// ���[���A�h���X
		for (r = p; *r != TEXT('\0') && *r != TEXT('\t') && *r != TEXT('\r') && *r != TEXT('\n'); r++);
		if (tpMailItem != NULL) {
			tpMailItem->To = (TCHAR*)mem_alloc(sizeof(TCHAR) * (r - p + 1));
			if (tpMailItem->To != NULL) {
				for (s = tpMailItem->To; p < r; p++, s++) {
					*s = *p;
				}
				*s = '\0';
			}
		}
		if (*r == TEXT('\t')) r++;

		// �R�����g
		for (p = r; *r != TEXT('\0') && *r != TEXT('\t') && *r != TEXT('\r') && *r != TEXT('\n'); r++);
		if (tpMailItem != NULL) {
			tpMailItem->Subject = (TCHAR*)mem_alloc(sizeof(TCHAR) * (r - p + 1));
			if (tpMailItem->Subject != NULL) {
				for (s = tpMailItem->Subject; p < r; p++, s++) {
					*s = *p;
				}
				*s = '\0';
			}
		}
		for (; *p != TEXT('\0') && *p != TEXT('\r') && *p != TEXT('\n'); p++);
		for (; *p == TEXT('\r') || *p == TEXT('\n'); p++);
		i++;
	}
	mem_free(&MemFile);
	return 1;
}

/*
 * file_write - �}���`�o�C�g�ɕϊ����ĕۑ�
 */
BOOL file_write(HANDLE hFile, char *buf, int len)
{
	DWORD ret;

	if (len == 0) return TRUE;
	return WriteFile(hFile, buf, len, &ret, NULL);
}

/*
 * file_write_ascii - �}���`�o�C�g�ɕϊ����ĕۑ�
 */
BOOL file_write_ascii(HANDLE hFile, TCHAR *buf, int len)
{
	DWORD ret;
#ifdef UNICODE
	char *str;
	int clen;

	if (len == 0) return TRUE;

	clen = tchar_to_char_size(buf);
	str = (char *)mem_alloc(clen + 1);
	if (str == NULL) {
		return FALSE;
	}
	tchar_to_char(buf, str, clen);
	if (WriteFile(hFile, str, clen - 1, &ret, NULL) == FALSE) {
		mem_free(&str);
		return FALSE;
	}
	mem_free(&str);
	return TRUE;

#else
	if (len == 0) return TRUE;
	return WriteFile(hFile, buf, len, &ret, NULL);
#endif
}

/*
 * file_write_utf8 - UTF-8�ɕϊ����ĕۑ�
 */
BOOL file_write_utf8(HANDLE hFile, TCHAR* buf)
{
	DWORD ret;
	WCHAR* wbuf;
	BYTE* cbuf;
	int len;

#ifdef UNICODE
	wbuf = buf;
#else
	// ASCII����UTF-16�ɕϊ�
	len = MultiByteToWideChar(CP_ACP, 0, buf, -1, NULL, 0);
	if ((wbuf = (WCHAR*)mem_alloc(sizeof(WCHAR) * len)) == NULL) {
		return FALSE;
	}
	MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, len);
#endif
	// UTF-16����UTF-8�ɕϊ�
	len = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);
	if ((cbuf = (BYTE*)mem_alloc(sizeof(BYTE) * len)) == NULL) {
#ifndef UNICODE
		mem_free(&wbuf);
#endif
		return FALSE;
	}
	WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, cbuf, len, NULL, NULL);
	if (WriteFile(hFile, cbuf, sizeof(BYTE) * (len - 1), &ret, NULL) == FALSE) {
		mem_free(&cbuf);
#ifndef UNICODE
		mem_free(&wbuf);
#endif
		return FALSE;
	}
	mem_free(&cbuf);
#ifndef UNICODE
	mem_free(&wbuf);
#endif
	return TRUE;
}

/*
 * file_save - �t�@�C���̕ۑ�
 */
BOOL file_save(HWND hWnd, TCHAR *FileName, TCHAR *Ext, char *buf, int len)
{
	HANDLE hFile;
	TCHAR path[BUF_SIZE];
	DWORD ret;

	// �t�@�C���ɕۑ�
	if (FileName == NULL) {
		*path = TEXT('\0');
	} else {
		lstrcpy(path, FileName);
	}
	if (filename_select(hWnd, path, Ext, NULL, FALSE) == FALSE) {
		return TRUE;
	}

	// �ۑ�����t�@�C�����J��
	hFile = CreateFile(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		return FALSE;
	}
	SwitchCursor(FALSE);
	if (WriteFile(hFile, buf, len, &ret, NULL) == FALSE) {
		CloseHandle(hFile);
		SwitchCursor(TRUE);
		return FALSE;
	}
	SwitchCursor(TRUE);
	CloseHandle(hFile);
	return TRUE;
}

/*
 * file_save_exec - �t�@�C����ۑ����Ď��s
 */
BOOL file_save_exec(HWND hWnd, TCHAR *FileName, char *buf, int len)
{
	WIN32_FIND_DATA FindData;
	SHELLEXECUTEINFO sei;
	HANDLE hFindFile;
	HANDLE hFile;
	TCHAR path[BUF_SIZE];
	TCHAR tmp_path[BUF_SIZE];
	DWORD ret;

	if (FileName == NULL) {
		return FALSE;
	}
	SwitchCursor(FALSE);

	// �f�B���N�g���̌���
	wsprintf(path, TEXT("%s%s"), DataDir, op.AttachPath);
	hFindFile = FindFirstFile(path, &FindData);
	FindClose(hFindFile);
	if (hFindFile == INVALID_HANDLE_VALUE || !(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		CreateDirectory(path, NULL);
	}

	wsprintf(path, TEXT("%s%s\\%s"), DataDir, op.AttachPath, FileName);
	if (op.AttachDelete == 0) {
		// �ꎞ�t�@�C���ɕۑ�
		wsprintf(tmp_path, TEXT("%s_%lX.tmp"), DataDir, (long)buf);
		hFile = CreateFile(tmp_path, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == NULL || hFile == (HANDLE)-1) {
			SwitchCursor(TRUE);
			return FALSE;
		}
		if (WriteFile(hFile, buf, len, &ret, NULL) == FALSE) {
			CloseHandle(hFile);
			DeleteFile(tmp_path);
			SwitchCursor(TRUE);
			return FALSE;
		}
		FlushFileBuffers(hFile);
		CloseHandle(hFile);

		// �t�@�C���̈ړ�
		if (CopyFile(tmp_path, path, FALSE) == FALSE) {
			DeleteFile(tmp_path);
			SwitchCursor(TRUE);
			return FALSE;
		}
		DeleteFile(tmp_path);
	} else {
		// �t�@�C���ɕۑ�
		hFile = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == NULL || hFile == (HANDLE)-1) {
			SwitchCursor(TRUE);
			return FALSE;
		}
		if (WriteFile(hFile, buf, len, &ret, NULL) == FALSE) {
			CloseHandle(hFile);
			SwitchCursor(TRUE);
			return FALSE;
		}
		FlushFileBuffers(hFile);
		CloseHandle(hFile);
	}
	SwitchCursor(TRUE);

	// ���s
	ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
	sei.cbSize = sizeof(sei);
	sei.fMask = 0;
	sei.hwnd = NULL;
	sei.lpVerb = NULL;
	sei.lpFile = path;
	sei.lpParameters = NULL;
	sei.lpDirectory = NULL;
	sei.nShow = SW_SHOWNORMAL;
	sei.hInstApp = hInst;
	ShellExecuteEx(&sei);
	return TRUE;
}

/*
 * file_save_mailbox - ���[���{�b�N�X���̃��[����ۑ�
 */
BOOL file_save_mailbox(TCHAR *FileName, MAILBOX *tpMailBox, int SaveFlag)
{
	HANDLE hFile;
	TCHAR path[BUF_SIZE];
	char *tmp, *p;
	int len = 0;
	int i;

	SetCurrentDirectory(AppDir);
	str_join_t(path, DataDir, FileName, (TCHAR *)-1);

	if (SaveFlag == 0) {
		// �ۑ����Ȃ��ꍇ�͍폜
		DeleteFile(path);
		return TRUE;
	}

#ifndef DIV_SAVE
	// ���[��������쐬
	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		if (*(tpMailBox->tpMailItem + i) == NULL) {
			continue;
		}
		len += item_to_string_size(*(tpMailBox->tpMailItem + i), (SaveFlag == 1) ? FALSE : TRUE);
	}
#ifdef UNICODE
	len += 3;	// BOM
#endif // UNICODE
	p = tmp = (char*)mem_alloc(sizeof(char) * (len + 1));
	if (tmp == NULL) {
		return FALSE;
	}
#ifdef UNICODE
	*((BYTE*)p++) = 0xEF;
	*((BYTE*)p++) = 0xBB;
	*((BYTE*)p++) = 0xBF;
#endif // UNICODE
	*p = '\0';
	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		if (*(tpMailBox->tpMailItem + i) == NULL) {
			continue;
		}
		p = item_to_string(p, *(tpMailBox->tpMailItem + i), (SaveFlag == 1) ? FALSE : TRUE);
	}
#endif	// DIV_SAVE

	// �ۑ�
	hFile = CreateFile(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
#ifndef DIV_SAVE
		mem_free(&tmp);
#endif	// DIV_SAVE
		return FALSE;
	}
#ifndef DIV_SAVE
	if (file_write(hFile, tmp, len) == FALSE) {
		mem_free(&tmp);
		return FALSE;
	}
	mem_free(&tmp);
#else	// DIV_SAVE
	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		if (*(tpMailBox->tpMailItem + i) == NULL) {
			continue;
		}
		len = item_to_string_size(*(tpMailBox->tpMailItem + i), (SaveFlag == 1) ? FALSE : TRUE);

		p = tmp = (char *)mem_alloc(sizeof(char) * (len + 1));
		if (tmp == NULL) {
			CloseHandle(hFile);
			return FALSE;
		}
		item_to_string(tmp, *(tpMailBox->tpMailItem + i), (SaveFlag == 1) ? FALSE : TRUE);
		if (file_write(hFile, tmp, len) == FALSE) {
			mem_free(&tmp);
			CloseHandle(hFile);
			return FALSE;
		}
		mem_free(&tmp);
	}
#endif	// DIV_SAVE
	CloseHandle(hFile);
	return TRUE;
}

/*
 * file_save_address_item - �A�h���X����1���ۑ�
 */
static BOOL file_save_address_item(HANDLE hFile, MAILITEM *tpMailItem)
{
	TCHAR *tmp;
	int len = 0;

	if (tpMailItem->To != NULL) {
		len += lstrlen(tpMailItem->To);
	}
	if (tpMailItem->Subject != NULL && *tpMailItem->Subject != TEXT('\0')) {
		len += 1;	// TAB
		len += lstrlen(tpMailItem->Subject);
	}
	len += 2;		// CRLF

	tmp = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
	if (tmp == NULL) {
		return FALSE;
	}
	if (tpMailItem->Subject != NULL && *tpMailItem->Subject != TEXT('\0')) {
		str_join_t(tmp, tpMailItem->To, TEXT("\t"), tpMailItem->Subject, TEXT("\r\n"), (TCHAR *)-1);
	} else {
		str_join_t(tmp, tpMailItem->To, TEXT("\r\n"), (TCHAR *)-1);
	}

	if (file_write_utf8(hFile, tmp) == FALSE) {
		mem_free(&tmp);
		return FALSE;
	}
	mem_free(&tmp);
	return TRUE;
}

/*
 * file_save_address_book - �A�h���X�����t�@�C���ɕۑ�
 */
BOOL file_save_address_book(TCHAR *FileName, MAILBOX *tpMailBox)
{
	HANDLE hFile;
	TCHAR path[BUF_SIZE];
	DWORD ret;
	int i;

	SetCurrentDirectory(AppDir);

	str_join_t(path, DataDir, FileName, (TCHAR *)-1);

	// �ۑ�����t�@�C�����J��
	hFile = CreateFile(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		return FALSE;
	}

	// BOM�̏o��
	if (WriteFile(hFile, "\xEF\xBB\xBF", 3, &ret, NULL) == FALSE) {
		CloseHandle(hFile);
		return FALSE;
	}

	// �ۑ����镶������R�s�[
	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		if (*(tpMailBox->tpMailItem + i) == NULL) {
			continue;
		}
		if (file_save_address_item(hFile, *(tpMailBox->tpMailItem + i)) == FALSE) {
			CloseHandle(hFile);
			return FALSE;
		}
	}
	CloseHandle(hFile);
	return TRUE;
}
/* End of source */
