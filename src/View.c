/*
 * nPOP
 *
 * View.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include <windows.h>
#include <imm.h>

#include "General.h"
#include "Memory.h"
#include "String.h"
#include "code.h"
#include "mime.h"
#include "multipart.h"
#include "Font.h"
#ifdef USE_NEDIT
#include "nEdit.h"
#endif
#include "dpi.h"

#include "global.h"
#include "md5.h"

/* Define */
#define WM_MODFYMESSAGE				(WM_APP + 101)
#define ID_MENU						(WM_APP + 102)

#define IDC_VCB						2000
#define IDC_VTB						2001
#define IDC_HEADER					2002
#define IDC_EDIT_BODY				2003

#define ID_CLICK_TIMER				1
#define ID_HIDECARET_TIMER			2

#define MENU_ATTACH_POS				9
#define ID_VIEW_SOURCE				400
#define ID_VIEW_PART				401
#define ID_VIEW_DELETE_ATTACH		402
#define ID_ATTACH					500

#define SAVE_HEADER					TEXT("From: %f\r\nTo: %t\r\nCc: %c\r\nSubject: %s\r\nDate: %d\r\nMessage-ID: %i\r\n\r\n")

/* Global Variables */
static WNDPROC EditWindowProcedure;
HWND hViewWnd = NULL;

int vSelBox;

static MULTIPART **tpMultiPart;
static int MultiPartCnt;

TCHAR *FindStr = NULL;
static DWORD FindPos;

static BOOL ESCFlag = FALSE;
static BOOL UnicodeEdit = 0;

int AttachProcess;

// 外部参照
extern OPTION op;

extern HINSTANCE hInst;  // Local copy of hInstance
extern HWND MainWnd;
extern HWND FocusWnd;
extern HFONT hListFont;
extern MAILBOX *MailBox;
extern int SelBox;
extern int RecvBox;
extern int MailBoxCnt;
extern BOOL ExecFlag;
extern BOOL PPCFlag;

extern HFONT hViewFont;
extern int font_charset;

extern TCHAR *DataDir;
extern TCHAR *AppDir;

/* Local Function Prototypes */
static void SetWindowString(HWND hWnd, TCHAR *MailBoxName, TCHAR *MailBoxName2, int No);
static void SetHeaderString(HWND hHeader, MAILITEM *tpMailItem);
static void SetHeaderSize(HWND hWnd);
static LRESULT TbNotifyProc(HWND hWnd,LPARAM lParam);
static LRESULT NotifyProc(HWND hWnd, LPARAM lParam);
static BOOL FindEditString(HWND hEdit, TCHAR *strFind, int CaseFlag);
static LRESULT CALLBACK SubClassEditProc(HWND hWnd, UINT msg, WPARAM wParam,LPARAM lParam);
static void SetEditSubClass(HWND hWnd);
static void DelEditSubClass(HWND hWnd);
static BOOL InitWindow(HWND hWnd, MAILITEM *tpMailItem);
static BOOL SetWindowSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
static void EndWindow(HWND hWnd);
static void SetEditMenu(HWND hWnd);
static void ModfyWindow(HWND hWnd, MAILITEM *tpMailItem, BOOL ViewSrc);
static MAILITEM *View_NextMail(HWND hWnd, BOOL St);
static void View_PrevMail(HWND hWnd);
static void View_NextNoReadMail(HWND hWnd);
static void View_NextScroll(HWND hEditWnd);
static BOOL ShellOpen(TCHAR *FileName);
static void OpenURL(HWND hWnd);
static void SetReMessage(HWND hWnd, int ReplyFag);
static BOOL Decode(HWND hWnd, int id);
static BOOL SaveViewMail(TCHAR *fname, HWND hWnd, int MailBoxIndex, MAILITEM *tpMailItem, TCHAR *head);
static BOOL AppViewMail(MAILITEM *tpMailItem);
static void SetMark(HWND hWnd, MAILITEM *tpMailItem, const int mark);
static void GetMarkStatus(HWND hWnd, MAILITEM *tpMailItem);
static LRESULT CALLBACK ViewProc(HWND hWnd, UINT msg, WPARAM wParam,LPARAM lParam);

/*
 * SetWindowString - ウィンドウタイトルの設定
 */
static void SetWindowString(HWND hWnd, TCHAR *MailBoxName, TCHAR *MailBoxName2, int No)
{
	TCHAR *buf, *r;
	TCHAR *p, *p2;

	p = (MailBoxName == NULL || *MailBoxName == TEXT('\0')) ? STR_MAILBOX_NONAME : MailBoxName;
	p2 = (MailBoxName2 == NULL || *MailBoxName2 == TEXT('\0')) ? TEXT("") : MailBoxName2;

	buf = (TCHAR *)mem_alloc(sizeof(TCHAR) *
		(lstrlen(STR_TITLE_MAILVIEW) + lstrlen(p) + lstrlen(p2) + lstrlen(TEXT(" -  ()") STR_TITLE_MAILVIEW_COUNT) + 10 + 1));
	if (buf == NULL) {
		SetWindowText(hWnd, STR_TITLE_MAILVIEW);
		return;
	}

	r = str_join_t(buf, STR_TITLE_MAILVIEW, TEXT(" - "), p, (TCHAR *)-1);
	if (*p2 != TEXT('\0')) {
		str_join_t(r, TEXT(" ("), p2, TEXT(")"), (TCHAR *)-1);
	} else if (No != 0) {
		wsprintf(r, STR_TITLE_MAILVIEW_COUNT, No);
	}
	if (lstrlen(buf) > BUF_SIZE) {
		*(buf + BUF_SIZE) = TEXT('\0');
	}
	SetWindowText(hWnd, buf);
	mem_free(&buf);
}

/*
 * SetHeaderString - メールヘッダ表示
 */
static void SetHeaderString(HWND hHeader, MAILITEM *tpMailItem)
{
	TCHAR *MyMailAddress = NULL;
	TCHAR *buf, *p;
	int len = 0;
	int i;
	BOOL ToFlag = FALSE;

	// 自分のメールアドレスの取得
	if (SelBox == MAILBOX_SAVE) {
		i = mailbox_name_to_index(tpMailItem->MailBox);
		if (i >= MAILBOX_USER) {
			MyMailAddress = (MailBox + i)->MailAddress;
		}
	} else {
		MyMailAddress = (MailBox + SelBox)->MailAddress;
	}

	// To を表示するべきかチェック
	if (tpMailItem->To != NULL) {
		buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(tpMailItem->To) + 1));
		if (buf != NULL) {
			p = GetMailAddress(tpMailItem->To, buf, FALSE);
			if (*buf != TEXT('\0') &&
				(*p != TEXT('\0') || MyMailAddress == NULL || lstrcmpi(MyMailAddress, buf) != 0)) {
				ToFlag = TRUE;
			}
			mem_free(&buf);
		}
	}

	// 作成する文字列のサイズを計算
	len += lstrlen(STR_VIEW_HEAD_FROM);
	if (tpMailItem->From != NULL) {
		len += lstrlen(tpMailItem->From);
	}
	len += lstrlen(STR_VIEW_HEAD_SUBJECT);
	if (tpMailItem->Subject != NULL) {
		len += lstrlen(tpMailItem->Subject);
	}
	if (ToFlag == TRUE) {
		len += SetCcAddressSize(tpMailItem->To);
	}
	len += SetCcAddressSize(tpMailItem->Cc);
	len += SetCcAddressSize(tpMailItem->Bcc);
	if (op.ViewShowDate == 1) {
		len += lstrlen(STR_VIEW_HEAD_DATE);
		if (tpMailItem->Date != NULL) {
			len += lstrlen(tpMailItem->Date);
		}
	}

	buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
	if (buf == NULL) {
		return;
	}
	*buf = TEXT('\0');

	// 表示する文字列を作成する
	p = str_join_t(buf, STR_VIEW_HEAD_FROM, tpMailItem->From, STR_VIEW_HEAD_SUBJECT, tpMailItem->Subject, (TCHAR *)-1);
	if (ToFlag == TRUE) {
		p = SetCcAddress(TEXT("To"), tpMailItem->To, p);
	}
	p = SetCcAddress(TEXT("Cc"), tpMailItem->Cc, p);
	p = SetCcAddress(TEXT("Bcc"), tpMailItem->Bcc, p);
	if (op.ViewShowDate == 1) {
		p = str_join_t(p, STR_VIEW_HEAD_DATE, tpMailItem->Date, (TCHAR *)-1);
	}
	SetWindowText(hHeader, buf);
	mem_free(&buf);
}

/*
 * SetHeaderSize - メールヘッダのサイズ設定
 */
static void SetHeaderSize(HWND hWnd)
{
	HWND hHeader;
	HDC hdc;
	TEXTMETRIC lptm;
	RECT rcClient, StRect;
	int Height = 0;
	int FontHeight;
	int HLine;
	HMENU hMenu;
	HFONT hFont;
	RECT ToolbarRect;

	hHeader = GetDlgItem(hWnd, IDC_HEADER);

	hMenu = GetSubMenu(GetMenu(hWnd), 1);
	CheckMenuItem(hMenu, ID_MENUITEM_SHOW_DATE, (op.ViewShowDate == 1) ? MF_CHECKED : MF_UNCHECKED);

	GetWindowRect(GetDlgItem(hWnd, IDC_VTB), &ToolbarRect);
	Height = ToolbarRect.bottom - ToolbarRect.top;
	GetClientRect(hWnd, &rcClient);

	hdc = GetDC(hHeader);
	hFont = SelectObject(hdc, (hListFont != NULL) ? hListFont : GetStockObject(DEFAULT_GUI_FONT));
	// フォントの高さを取得
	GetTextMetrics(hdc, &lptm);
	if (hFont != NULL) {
		SelectObject(hdc, hFont);
	}
	ReleaseDC(hHeader, hdc);
	HLine = (op.ViewShowDate == 1) ? 3 : 2;
	FontHeight = (lptm.tmHeight + lptm.tmExternalLeading) * HLine;

	// 一時的に設定してサイズを再計算する
	MoveWindow(hHeader, 0, Height, rcClient.right, FontHeight, TRUE);
	GetClientRect(hHeader, &StRect);
	FontHeight = FontHeight + (FontHeight - StRect.bottom) + 1;
	MoveWindow(hHeader, 0, Height, rcClient.right, FontHeight, TRUE);

	InvalidateRect(hHeader, NULL, FALSE);
	UpdateWindow(hHeader);

	MoveWindow(GetDlgItem(hWnd, IDC_EDIT_BODY), 0, Height + FontHeight + 1,
		rcClient.right, rcClient.bottom - Height - FontHeight - 1, TRUE);
}

/*
 * TbNotifyProc - ツールバーの通知メッセージ (Win32)
 */
static LRESULT TbNotifyProc(HWND hWnd,LPARAM lParam)
{
	TOOLTIPTEXT *pTT;

	pTT = (TOOLTIPTEXT*)lParam;
	pTT->hinst = hInst;
	pTT->lpszText = MAKEINTRESOURCE(pTT->hdr.idFrom);
	return 0;
}

/*
 * NotifyProc - コントロールの通知メッセージ
 */
static LRESULT NotifyProc(HWND hWnd, LPARAM lParam)
{
	NMHDR *CForm = (NMHDR *)lParam;

	if (CForm->code == TTN_NEEDTEXT) {
		return TbNotifyProc(hWnd, lParam);
	}
	return FALSE;
}

/*
 * FindEditString - EDIT内の文字列を検索する
 */
static BOOL FindEditString(HWND hEdit, TCHAR *strFind, int CaseFlag)
{
	DWORD dwStart;
	DWORD dwEnd;
	TCHAR *buf = NULL;
	TCHAR *p;

#ifdef UNICODE
	// 検索位置の取得
	SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
	// 現在位置が前回検索位置と違う場合は現在位置を検索位置にする
	if ((dwStart + 1U) != FindPos) {
		FindPos = dwStart;
	}

	// エディットから文字列を取得する
	AllocGetText(hEdit, &buf);
	p = str_find(strFind, buf + FindPos, CaseFlag);

	// 検索文字列が見つからなかった場合
	if (*p == TEXT('\0')) {
		mem_free(&buf);
		return FALSE;
	}

	// 文字列が見つかった場合はその位置を選択状態にする
	FindPos = p - buf;
	SendMessage(hEdit, EM_SETSEL, FindPos, FindPos + lstrlen(strFind));
	SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
	mem_free(&buf);
	FindPos++;
	return TRUE;
#else
	// エディットから文字列を取得する
	AllocGetText(hEdit, &buf);
	if (UnicodeEdit == 2) {
		int st, len;
		WCHAR *wbuf;

		len = MultiByteToWideChar(CP_ACP, 0, buf, -1, NULL, 0);
		wbuf = (WCHAR *)mem_alloc(sizeof(WCHAR) * (len + 1));
		if (wbuf == NULL) {
			mem_free(&buf);
			return FALSE;
		}
		MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, len);
		// 検索位置の取得
		SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
		st = WideCharToMultiByte(CP_ACP, 0, wbuf, dwStart, NULL, 0, NULL, NULL);
		mem_free(&wbuf);
		// 現在位置が前回検索位置と違う場合は現在位置を検索位置にする
		if ((st + 1U) != FindPos) {
			FindPos = st;
		}
		p = str_find(strFind, buf + FindPos, CaseFlag);
		// 検索文字列が見つからなかった場合
		if (*p == TEXT('\0')) {
			mem_free(&buf);
			return FALSE;
		}
		st = MultiByteToWideChar(CP_ACP, 0, buf, p - buf, NULL, 0);
		len = MultiByteToWideChar(CP_ACP, 0, strFind, -1, NULL, 0) - 1;
		// 文字列が見つかった場合はその位置を選択状態にする
		SendMessage(hEdit, EM_SETSEL, st, st + len);
		FindPos = p - buf;
	} else {
		// 検索位置の取得
		SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
		// 現在位置が前回検索位置と違う場合は現在位置を検索位置にする
		if ((dwStart + 1U) != FindPos) {
			FindPos = dwStart;
		}
		p = str_find(strFind, buf + FindPos, CaseFlag);
		// 検索文字列が見つからなかった場合
		if (*p == TEXT('\0')) {
			mem_free(&buf);
			return FALSE;
		}
		// 文字列が見つかった場合はその位置を選択状態にする
		FindPos = p - buf;
		SendMessage(hEdit, EM_SETSEL, FindPos, FindPos + lstrlen(strFind));
	}
	SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
	mem_free(&buf);
	FindPos++;
	return TRUE;
#endif
}

/*
 * SetWordBreakMenu - 折り返しのメニューのチェック切り替え
 */
void SetWordBreakMenu(HWND hWnd, HMENU hEditMenu, int Flag)
{
	HMENU hMenu;

	hMenu = GetMenu(hWnd);
	CheckMenuItem(GetSubMenu(hMenu, 1), ID_MENUITEM_WORDBREAK, Flag);
}

/*
 * SetWordBreak - 折り返しの切り替え
 */
int SetWordBreak(HWND hWnd)
{
	HWND hEdit;
	RECT rcClient;
	RECT HeaderRect;
	RECT ToolbarRect;
	TCHAR *buf;
	int hHeight = 0, tHeight = 0;
	int len, ret;
	int i;
	BOOL ModifyFlag;

	GetClientRect(hWnd, &rcClient);
	GetWindowRect(GetDlgItem(hWnd, IDC_HEADER), &HeaderRect);
	hHeight = HeaderRect.bottom - HeaderRect.top;
	GetWindowRect(GetDlgItem(hWnd, IDC_VTB), &ToolbarRect);
	tHeight = ToolbarRect.bottom - ToolbarRect.top;

	hEdit = GetDlgItem(hWnd, IDC_EDIT_BODY);
	i = GetWindowLong(hEdit, GWL_STYLE);
	if (i & WS_HSCROLL) {
		i ^= WS_HSCROLL;
		SetWordBreakMenu(hWnd, NULL, MF_CHECKED);
		ret = 1;
	} else {
		i |= WS_HSCROLL;
		SetWordBreakMenu(hWnd, NULL, MF_UNCHECKED);
		ret = 0;
	}

	ModifyFlag = SendMessage(hEdit, EM_GETMODIFY, 0, 0);

	len = SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0) + 1;
	buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
	if (buf == NULL) {
		return !ret;
	}
	*buf = TEXT('\0');
	SendMessage(hEdit, WM_GETTEXT, len, (LPARAM)buf);

	DestroyWindow(hEdit);
	hEdit = CreateWindowEx(
		0,
		TEXT("EDIT"), TEXT(""), i,
		0, tHeight + hHeight + 1, rcClient.right,
		rcClient.bottom - tHeight - hHeight - 1,
		hWnd, (HMENU)IDC_EDIT_BODY, hInst, NULL);
	if (hViewFont != NULL) {
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hViewFont, MAKELPARAM(TRUE,0));
	}
	SetFocus(hEdit);

	SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)buf);
	SendMessage(hEdit, EM_SETMODIFY, (WPARAM)ModifyFlag, 0);
	mem_free(&buf);
	return ret;
}

/*
 * SubClassEditProc - サブクラス化したウィンドウプロシージャ
 */
static LRESULT CALLBACK SubClassEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CHAR:
		if ((TCHAR)wParam == TEXT(' ')) {
			if (GetKeyState(VK_SHIFT) < 0) {
				SendMessage(hWnd, WM_VSCROLL, SB_PAGEUP, 0);
			} else if (GetKeyState(VK_CONTROL) < 0) {
				View_NextNoReadMail(GetParent(hWnd));
			} else {
				View_NextScroll(hWnd);
			}
		}
	case WM_DEADCHAR:
	case WM_CUT:
	case WM_CLEAR:
	case WM_PASTE:
	case EM_UNDO:
		return 0;

	case WM_LBUTTONDBLCLK:
		SetTimer(GetParent(hWnd), ID_CLICK_TIMER, 100, NULL);
		break;

	case WM_CONTEXTMENU:
		SetEditMenu(GetParent(hWnd));
		ShowMenu(GetParent(hWnd), GetMenu(GetParent(hWnd)), 1, 0, FALSE);
		return 0;
	}
	return CallWindowProc(EditWindowProcedure, hWnd, msg, wParam, lParam);
}

/*
 * SetEditSubClass - ウィンドウのサブクラス化
 */
static void SetEditSubClass(HWND hWnd)
{
	SendMessage(hWnd, EM_LIMITTEXT, 1, 0);
	EditWindowProcedure = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (long)SubClassEditProc);
}

/*
 * DelEditSubClass - ウィンドウクラスを標準のものに戻す
 */
static void DelEditSubClass(HWND hWnd)
{
	SetWindowLong(hWnd, GWL_WNDPROC, (long)EditWindowProcedure);
	EditWindowProcedure = NULL;
}

/*
 * InitWindow - ウィンドウの初期化
 */
static BOOL InitWindow(HWND hWnd, MAILITEM *tpMailItem)
{
	HWND hToolBar;
	TBBUTTON tbButton[] = {
		{0,	ID_MENUITEM_PREVMAIL,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{1,	ID_MENUITEM_NEXTMAIL,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{2,	ID_MENUITEM_NEXTNOREAD,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{0,	0,						TBSTATE_ENABLED,	TBSTYLE_SEP,	0, 0, 0, -1},
		{3,	ID_MENUITEM_REMESSEGE,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{4,	ID_MENUITEM_ALLREMESSEGE,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{0,	0,						TBSTATE_ENABLED,	TBSTYLE_SEP,	0, 0, 0, -1},
		{5,	ID_MENUITEM_DOWNMARK,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{6,	ID_MENUITEM_DELMARK,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
	};

	if (tpMailItem == NULL || tpMailItem->Body == NULL) {
		return FALSE;
	}

	if (GetAwareness() != PROCESS_DPI_UNAWARE && GetScale() >= 300) {
		hToolBar = CreateToolbarEx(hWnd, WS_CHILD | TBSTYLE_TOOLTIPS, IDC_VTB, 7, hInst, IDB_TOOLBAR_VIEW_48,
			tbButton, sizeof(tbButton) / sizeof(TBBUTTON), 0, 0, 48, 48, sizeof(TBBUTTON));
	}
	else if (GetAwareness() != PROCESS_DPI_UNAWARE && GetScale() >= 150) {
		hToolBar = CreateToolbarEx(hWnd, WS_CHILD | TBSTYLE_TOOLTIPS, IDC_VTB, 7, hInst, IDB_TOOLBAR_VIEW_32,
			tbButton, sizeof(tbButton) / sizeof(TBBUTTON), 0, 0, 32, 32, sizeof(TBBUTTON));
	}
	else {
		hToolBar = CreateToolbarEx(hWnd, WS_CHILD | TBSTYLE_TOOLTIPS, IDC_VTB, 7, hInst, IDB_TOOLBAR_VIEW,
			tbButton, sizeof(tbButton) / sizeof(TBBUTTON), 0, 0, TB_ICONSIZE, TB_ICONSIZE, sizeof(TBBUTTON));
	}
	SetWindowLong(hToolBar, GWL_STYLE, GetWindowLong(hToolBar, GWL_STYLE) | TBSTYLE_FLAT);
	SendMessage(hToolBar, TB_SETINDENT, 5, 0);
	ShowWindow(hToolBar, SW_SHOW);

	// ヘッダを表示するSTATICコントロールの作成
	CreateWindowEx(
		0,
		TEXT("STATIC"), TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_BORDER | SS_LEFTNOWORDWRAP | SS_NOPREFIX,
		0, 0, 0, 0, hWnd, (HMENU)IDC_HEADER, hInst, NULL);
	// フォントの設定
	SendDlgItemMessage(hWnd, IDC_HEADER, WM_SETFONT,
		(WPARAM)((hListFont != NULL) ? hListFont : GetStockObject(DEFAULT_GUI_FONT)), MAKELPARAM(TRUE,0));

#ifndef USE_NEDIT
	CreateWindowEx(
		0,
		TEXT("EDIT"), TEXT(""),
		WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_VSCROLL | ((op.WordBreakFlag == 1) ? 0 : WS_HSCROLL),
		0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_BODY, hInst, NULL);
#else
	CreateWindowEx(
		0,
		NEDIT_WND_CLASS, TEXT(""),
		WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_VSCROLL | ((op.WordBreakFlag == 1) ? 0 : WS_HSCROLL),
		0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_BODY, hInst, NULL);
#endif
	if (hViewFont != NULL) {
		SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_SETFONT, (WPARAM)hViewFont, MAKELPARAM(TRUE,0));
	}
	SetHeaderSize(hWnd);
#ifndef UNICODE
	if (UnicodeEdit == 0) {
		int i, j;
		SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_SETTEXT, (WPARAM)0, (LPARAM)TEXT("あ"));
		SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
		SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_GETSEL, (WPARAM)&i, (LPARAM)&j);
		UnicodeEdit = (j != 2) ? 2 : 1;
	}
#endif

	SetFocus(GetDlgItem(hWnd, IDC_EDIT_BODY));
	SetWordBreakMenu(hWnd, NULL, (op.WordBreakFlag == 1) ? MF_CHECKED : MF_UNCHECKED);

	// IMEをオフにする
	ImmAssociateContext(GetDlgItem(hWnd, IDC_EDIT_BODY), (HIMC)NULL);

	SetEditSubClass(GetDlgItem(hWnd, IDC_EDIT_BODY));
	SetTimer(hWnd, ID_HIDECARET_TIMER, 10, NULL);
	return TRUE;
}

/*
 * SetWindowSize - ウィンドウのサイズ変更
 */
static BOOL SetWindowSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HWND hHeader, hBody;
	RECT rcClient, HeaderRect, ToolbarRect;
	int hHeight = 0, tHeight = 0;

	hHeader = GetDlgItem(hWnd, IDC_HEADER);
	hBody = GetDlgItem(hWnd, IDC_EDIT_BODY);

	SendDlgItemMessage(hWnd, IDC_VTB, WM_SIZE, wParam, lParam);

	GetClientRect(hWnd, &rcClient);
	GetWindowRect(hHeader, &HeaderRect);
	hHeight = HeaderRect.bottom - HeaderRect.top;
	GetWindowRect(GetDlgItem(hWnd, IDC_VTB), &ToolbarRect);
	tHeight = ToolbarRect.bottom - ToolbarRect.top;

	MoveWindow(hHeader, 0, tHeight, rcClient.right, hHeight, TRUE);
	InvalidateRect(hHeader, NULL, FALSE);
	UpdateWindow(hHeader);

	MoveWindow(hBody, 0, tHeight + hHeight + 1,
		rcClient.right, rcClient.bottom - tHeight - hHeight - 1, TRUE);
	return TRUE;
}

/*
 * EndWindow - ウィンドウの終了処理
 */
static void EndWindow(HWND hWnd)
{
	multipart_free(&tpMultiPart, MultiPartCnt);
	tpMultiPart = NULL;
	MultiPartCnt = 0;

	DelEditSubClass(GetDlgItem(hWnd, IDC_EDIT_BODY));
	DestroyWindow(GetDlgItem(hWnd, IDC_VTB));
	DestroyWindow(GetDlgItem(hWnd, IDC_HEADER));
	DestroyWindow(GetDlgItem(hWnd, IDC_EDIT_BODY));

	DestroyWindow(hWnd);
}

/*
 * SetEditMenu - 編集メニューの活性／非活性の切り替え
 */
static void SetEditMenu(HWND hWnd)
{
	HMENU hMenu;
	int i, j;

	hMenu = GetMenu(hWnd);
	// エディットボックスの選択位置の取得
	SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_GETSEL, (WPARAM)&i, (LPARAM)&j);
	EnableMenuItem(hMenu, ID_MENUITEM_COPY, (i < j) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, ID_MENUITEM_FIND, (SelBox != MAILBOX_SEND) ? MF_ENABLED : MF_GRAYED);
}

/*
 * SetAttachMenu - 表示するpartの選択と添付メニューの設定
 */
static int SetAttachMenu(HWND hWnd, MAILITEM *tpMailItem, BOOL ViewSrc)
{
	HMENU hMenu;
	TCHAR *str;
	TCHAR *p, *r;
	int i, ret = -1;
	int mFlag;
	BOOL AppendFlag = FALSE;

	hMenu = GetSubMenu(GetMenu(hWnd), 1);
	// メニューを初期化する
	while (DeleteMenu(hMenu, MENU_ATTACH_POS, MF_BYPOSITION) == TRUE);

	if (MultiPartCnt == 1 &&
		(*tpMultiPart)->sPos == tpMailItem->Body && (*tpMultiPart)->ePos == NULL &&
		(*tpMultiPart)->ContentType != NULL &&
		str_cmp_ni((*tpMultiPart)->ContentType, "text", tstrlen("text")) == 0) {
		return 0;
	}
	if (MultiPartCnt == 1 && ViewSrc == TRUE) {
		AppendMenu(hMenu, MF_STRING, ID_VIEW_PART, STR_VIEW_MENU_ATTACH);
		return 0;
	}

	for (i = 0; i < MultiPartCnt; i++) {
		if (ret == -1 && ((*(tpMultiPart + i))->ContentType == NULL ||
			str_cmp_ni((*(tpMultiPart + i))->ContentType, "text", tstrlen("text")) == 0)) {
			// 一番目に出現したテキストデータは本文にする
			ret = i;
			if (MultiPartCnt == 1 && tpMailItem->Multipart == TRUE) {
				AppendMenu(hMenu, MF_STRING, ID_VIEW_SOURCE, STR_VIEW_MENU_SOURCE);
			}
		} else {
			if (AppendFlag == FALSE) {
				if (MultiPartCnt > 1) {
					AppendMenu(hMenu, MF_STRING, ID_VIEW_SOURCE, STR_VIEW_MENU_SOURCE);
					AppendMenu(hMenu, MF_STRING, ID_VIEW_DELETE_ATTACH, STR_VIEW_MENU_DELATTACH);
				}
				// 区切りの追加
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
				AppendFlag = TRUE;
			}
			// 途中で切れているデータの場合はメニューを非活性にする
			mFlag = (i != 0 && (*(tpMultiPart + i))->ePos == NULL) ? MF_GRAYED : MF_ENABLED;

			// ファイル名をメニューに追加する
			str = alloc_char_to_tchar((*(tpMultiPart + i))->ContentType);
			if (str != NULL) {
				for (r = str; *r != TEXT('\0') && *r != TEXT(';'); r++);
				*r = TEXT('\0');
			}
#ifdef UNICODE
			if ((*(tpMultiPart + i))->Filename != NULL) {
				p = alloc_char_to_tchar((*(tpMultiPart + i))->Filename);
				mem_free(&str);
				str = p;
			} else {
				p = str;
			}
#else
			p = ((*(tpMultiPart + i))->Filename != NULL) ? (*(tpMultiPart + i))->Filename : str;
#endif
			AppendMenu(hMenu, MF_STRING | mFlag, ID_ATTACH + i, ((p != NULL && *p != TEXT('\0')) ? p : TEXT("Attach")));
			mem_free(&str);
		}
	}
	return ret;
}

/*
 * ModfyWindow - 内容の変更
 */
static void ModfyWindow(HWND hWnd, MAILITEM *tpMailItem, BOOL ViewSrc)
{
	TCHAR *buf;
	int LvFocus;
	OSVERSIONINFO os_info;

	if (tpMailItem == NULL || tpMailItem->Body == NULL) {
		return;
	}

	SwitchCursor(FALSE);

	vSelBox = SelBox;
	LvFocus = ListView_GetNextItem(GetDlgItem(MainWnd, IDC_LISTVIEW), -1, LVNI_FOCUSED);

	// 開封済みにする
	if (tpMailItem->MailStatus != ICON_NON && tpMailItem->MailStatus < ICON_SENDMAIL) {
		tpMailItem->MailStatus = ICON_READ;
	}

	// 一覧のアイコンの設定
	if (tpMailItem->Status != ICON_DOWN && tpMailItem->Status != ICON_DEL) {
		tpMailItem->Status = tpMailItem->MailStatus;
		ListView_RedrawItems(GetDlgItem(MainWnd, IDC_LISTVIEW), LvFocus, LvFocus);
		UpdateWindow(GetDlgItem(MainWnd, IDC_LISTVIEW));
	}

	// ウィンドウタイトルの設定
	SetWindowString(hWnd, (MailBox + SelBox)->Name , tpMailItem->MailBox, tpMailItem->No);
	// ヘッダの設定
	SetHeaderString(GetDlgItem(hWnd, IDC_HEADER), tpMailItem);
	UpdateWindow(GetDlgItem(hWnd, IDC_HEADER));

	SetItemCntStatusText(MainWnd, NULL);

	// マーク状態取得
	GetMarkStatus(hWnd, tpMailItem);

	multipart_free(&tpMultiPart, MultiPartCnt);
	tpMultiPart = NULL;
	MultiPartCnt = 0;
	// マルチパートの展開
	buf = MIME_body_decode(tpMailItem, ViewSrc, &tpMultiPart, &MultiPartCnt);

	// 表示するpartの選択と添付メニューの設定
	SetAttachMenu(hWnd, tpMailItem, ViewSrc);

	// 検索位置の初期化
	FindPos = 0;

	// 本文の表示
	if (buf != NULL) {
		// OS情報の取得
		os_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&os_info);

		if (os_info.dwPlatformId != VER_PLATFORM_WIN32_NT && lstrlen(buf) > EDITMAXSIZE) {
			*(buf + EDITMAXSIZE) = TEXT('\0');
		}
		SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_SETTEXT, 0, (LPARAM)buf);
		mem_free(&buf);
	} else {
		SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_SETTEXT, 0, (LPARAM)TEXT(""));
	}

	ShowWindow(hWnd, SW_SHOW);
	if (IsIconic(hWnd) != 0) {
		ShowWindow(hWnd, SW_RESTORE);
	}
	HideCaret(GetDlgItem(hWnd, IDC_EDIT_BODY));
	SwitchCursor(TRUE);
}

/*
 * View_NextMail - 次のメールを表示
 */
static MAILITEM *View_NextMail(HWND hWnd, BOOL St)
{
	MAILITEM *tpMailItem;
	HWND hListView;
	int Index;
	int j;

	if (SelBox == MAILBOX_SEND) {
		return NULL;
	}
	hListView = GetDlgItem(MainWnd, IDC_LISTVIEW);
	if (St == FALSE) {
		Index = ListView_GetMemToItem(hListView,
			(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA));
	} else {
		Index = -1;
	}

	j = ListView_GetNextMailItem(hListView, Index);
	if (j == -1) {
		return NULL;
	}
	ListView_SetItemState(hListView, -1, 0, LVIS_SELECTED);
	ListView_SetItemState(hListView,
		j, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(hListView, j, TRUE);

	tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, j);
	SetWindowLong(hWnd, GWL_USERDATA, (long)tpMailItem);
	ModfyWindow(hWnd, tpMailItem, FALSE);
	return tpMailItem;
}

/*
 * View_PrevMail - 前のメールを表示
 */
static void View_PrevMail(HWND hWnd)
{
	MAILITEM *tpMailItem;
	HWND hListView;
	int Index;
	int j;

	if (SelBox == MAILBOX_SEND) {
		return;
	}
	hListView = GetDlgItem(MainWnd, IDC_LISTVIEW);
	Index = ListView_GetMemToItem(hListView,
		(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA));

	j = ListView_GetPrevMailItem(hListView, Index);
	if (j == -1) {
		return;
	}
	ListView_SetItemState(hListView, -1, 0, LVIS_SELECTED);
	ListView_SetItemState(hListView,
		j, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(hListView, j, TRUE);

	tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, j);
	SetWindowLong(hWnd, GWL_USERDATA, (long)tpMailItem);
	ModfyWindow(hWnd, tpMailItem, FALSE);
}

/*
 * View_NextNoReadMail - 次の未開封メールを表示
 */
static void View_NextNoReadMail(HWND hWnd)
{
	MAILITEM *tpMailItem;
	HWND hListView;
	int Index;
	int i, j;

	if (SelBox == MAILBOX_SEND) {
		return;
	}
	hListView = GetDlgItem(MainWnd, IDC_LISTVIEW);
	Index = ListView_GetMemToItem(hListView,
		(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA));

	// 未開封メールのインデックスを取得
	j = ListView_GetNextNoReadItem(hListView, Index,
		ListView_GetItemCount(hListView));
	if (j == -1) {
		j = ListView_GetNextNoReadItem(hListView, -1, Index);
	}
	if (j == -1) {
		if (op.MoveAllMailBox == 0) {
			return;
		}

		// 次の未開封があるメールボックスのインデックスを取得
		SwitchCursor(FALSE);
		i = mailbox_next_unread(SelBox + 1, MailBoxCnt);
		if (i == -1) {
			i = mailbox_next_unread(MAILBOX_USER, SelBox);
		}
		if (i == -1) {
			SwitchCursor(TRUE);
			return;
		}
		// メールボックスの選択
		mailbox_select(MainWnd, i);
		j = ListView_GetNextNoReadItem(hListView, -1,
			ListView_GetItemCount(hListView));
		SwitchCursor(TRUE);
	}
	ListView_SetItemState(hListView, -1, 0, LVIS_SELECTED);
	ListView_SetItemState(hListView,
		j, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(hListView, j, TRUE);

	tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, j);
	SetWindowLong(hWnd, GWL_USERDATA, (long)tpMailItem);
	ModfyWindow(hWnd, tpMailItem, FALSE);
	SendMessage(MainWnd, WM_INITTRAYICON, 0, 0);
}

/*
 * View_NextScroll - 表示メールをPageDownして一番下に来たら次の未開封メールに移動する
 */
static void View_NextScroll(HWND hEditWnd)
{
	TEXTMETRIC lptm;
	SCROLLINFO ScrollInfo;
	RECT rRect;
	HDC hdc;
	HFONT hFont;
	BOOL Next = FALSE;

	// フォントの高さを取得
	hdc = GetDC(hEditWnd);
	if (hViewFont != NULL) {
		hFont = SelectObject(hdc, hViewFont);
	}
	GetTextMetrics(hdc, &lptm);
	if (hViewFont != NULL) {
		SelectObject(hdc, hFont);
	}
	ReleaseDC(hEditWnd, hdc);

	// スクロール情報の取得
	ScrollInfo.cbSize = sizeof(SCROLLINFO);
	ScrollInfo.fMask = SIF_POS | SIF_RANGE;
	GetScrollInfo(hEditWnd, SB_VERT, &ScrollInfo);

	GetClientRect(hEditWnd,(LPRECT)&rRect);
	ScrollInfo.nMax -= ((rRect.bottom - rRect.top) / (lptm.tmHeight + lptm.tmExternalLeading));
	if (ScrollInfo.nPos >= ScrollInfo.nMax) {
		Next = TRUE;
	}
	// PageDown
	SendMessage(hEditWnd, WM_VSCROLL, SB_PAGEDOWN, 0);
	if (Next == TRUE) {
		// 次の未開封メールへ移動する
		View_NextNoReadMail(GetParent(hEditWnd));
	}
}

/*
 * View_FindMail - メール内から文字列を検索
 */
void View_FindMail(HWND hWnd, BOOL FindSet)
{
	MAILITEM *tpMailItem;
	HWND hEdit;
	TCHAR *buf = NULL;
	TCHAR *p;
	DWORD dwStart;
	DWORD dwEnd;

	if (SelBox == MAILBOX_SEND) {
		return;
	}

	hEdit = GetDlgItem(hWnd, IDC_EDIT_BODY);
	if (FindSet == TRUE || FindStr == NULL) {
#ifdef UNICODE
		// 選択文字列を取得
		SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
		if (dwStart != dwEnd) {
			AllocGetText(hEdit, &buf);
			if (buf != NULL) {
				mem_free(&FindStr);
				FindStr = (TCHAR *)mem_alloc(sizeof(TCHAR) * (dwEnd - dwStart + 1));
				if (FindStr != NULL) {
					str_cpy_n_t(FindStr, buf + dwStart, dwEnd - dwStart + 1);
				}
				mem_free(&buf);
			}
		}
#else
		if (UnicodeEdit == 2) {
			WCHAR *wbuf;
			int len;
			// 選択文字列を取得
			SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
			if (dwStart != dwEnd) {
				AllocGetText(hEdit, &buf);
				if (buf != NULL) {
					len = MultiByteToWideChar(CP_ACP, 0, buf, -1, NULL, 0);
					wbuf = (WCHAR *)mem_alloc(sizeof(WCHAR) * (len + 1));
					if (wbuf == NULL) {
						mem_free(&buf);
						return;
					}
					MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, len);
					
					len = WideCharToMultiByte(CP_ACP, 0, wbuf + dwStart, dwEnd - dwStart, NULL, 0, NULL, NULL);
					mem_free(&FindStr);
					FindStr = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
					if (FindStr != NULL) {
						WideCharToMultiByte(CP_ACP, 0, wbuf + dwStart, dwEnd - dwStart, FindStr, len + 1, NULL, NULL);
						*(FindStr + len) = TEXT('\0');
					}
					mem_free(&buf);
					mem_free(&wbuf);
				}
			}
		} else {
			// 選択文字列を取得
			SendMessage(hEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
			if (dwStart != dwEnd) {
				AllocGetText(hEdit, &buf);
				if (buf != NULL) {
					mem_free(&FindStr);
					FindStr = (TCHAR *)mem_alloc(sizeof(TCHAR) * (dwEnd - dwStart + 1));
					if (FindStr != NULL) {
						str_cpy_n_t(FindStr, buf + dwStart, dwEnd - dwStart + 1);
					}
					mem_free(&buf);
				}
			}
		}
#endif

		// 検索条件の設定
		if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_FIND), hWnd, SetFindProc) == FALSE) {
			return;
		}
	}

	while (1) {
		// 本文から検索して見つかった位置を選択状態にする
		if (FindEditString(hEdit, FindStr, op.MstchCase) == TRUE) {
			break;
		}

		// １メール内での検索の場合
		if (op.AllFind == 0) {
			SendMessage(hEdit, EM_SETSEL, 0, 0);
			if (FindEditString(hEdit, FindStr, op.MstchCase) == TRUE) {
				break;
			}
			buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(FindStr) + lstrlen(STR_MSG_NOFIND) + 1));
			if (buf == NULL) {
				break;
			}
			wsprintf(buf, STR_MSG_NOFIND, FindStr);
			MessageBox(hWnd, buf, STR_TITLE_FIND, MB_ICONINFORMATION);
			mem_free(&buf);
			break;
		}

		// 次のメールに移動
		tpMailItem = View_NextMail(hWnd, FALSE);
		if (tpMailItem == NULL) {
			buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(FindStr) + lstrlen(STR_TITLE_ALLFIND) + 1));
			if (buf == NULL) {
				break;
			}
			wsprintf(buf, STR_TITLE_ALLFIND, FindStr);
			if (MessageBox(hWnd, STR_Q_NEXTFIND, buf, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDNO) {
				mem_free(&buf);
				break;
			}
			mem_free(&buf);
			tpMailItem = View_NextMail(hWnd, TRUE);
		}

		// ESC が押された場合は検索終了
		if (GetForegroundWindow() == hWnd && GetAsyncKeyState(VK_ESCAPE) < 0) {
			ESCFlag = TRUE;
			break;
		}

		// Subjectから検索
		if (op.SubjectFind != 0 && tpMailItem->Subject != NULL) {
			p = str_find(FindStr, tpMailItem->Subject, op.MstchCase);
			if (*p != TEXT('\0')) {
				break;
			}
		}
	}
}

/*
 * ShellOpen - ファイルを関連付けで実行
 */
static BOOL ShellOpen(TCHAR *FileName)
{
	SHELLEXECUTEINFO sei;

	ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
	sei.cbSize = sizeof(sei);
	sei.fMask = 0;
	sei.hwnd = NULL;
	sei.lpVerb = NULL;
	if (op.URLApp == NULL || *op.URLApp == TEXT('\0')) {
		sei.lpFile = FileName;
		sei.lpParameters = NULL;
	} else {
		sei.lpFile = op.URLApp;
		sei.lpParameters = FileName;
	}
	sei.lpDirectory = NULL;
	sei.nShow = SW_SHOWNORMAL;
	sei.hInstApp = hInst;
	return ShellExecuteEx(&sei);
}

/*
 * OpenURL - エディットボックスから選択されたURLを抽出して開く
 */
static void OpenURL(HWND hWnd)
{
	TCHAR *buf;
	TCHAR *str;
	TCHAR *p, *r, *s;
	int i, j;
	int len;
	int MailToFlag = 0;

	// エディットボックスの選択位置の取得
	SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_GETSEL, (WPARAM)&i, (LPARAM)&j);
	if (i >= j) {
		return;
	}

	// エディットボックスに設定された文字列の取得
	len = SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_GETTEXTLENGTH, 0, 0) + 1;
	buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
	if (buf == NULL) {
		return;
	}
	*buf = TEXT('\0');
	SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_GETTEXT, len, (LPARAM)buf);

	for (; *(buf + j) != TEXT('\0') && *(buf + j) != TEXT('\r') && *(buf + j) != TEXT('\n') && *(buf + j) != TEXT(' '); j++);
#ifdef UNICODE
	str = (TCHAR *)mem_alloc(sizeof(TCHAR) * (j - i + 2));
	if (str == NULL) {
		mem_free(&buf);
		return;
	}
	// 選択文字列の抽出
	for (p = buf + i, r = str; p < (buf + j); p++, r++) {
		*r = *p;
	}
	*r = TEXT('\0');
	mem_free(&buf);
#else
	if (UnicodeEdit == 2) {
		// WindowsXP
		WCHAR *wbuf, *wstr;
		WCHAR *wst, *wen, *wr;

		len = MultiByteToWideChar(CP_ACP, 0, buf, -1, NULL, 0);
		wbuf = (WCHAR *)mem_alloc(sizeof(WCHAR) * (len + 1));
		if (wbuf == NULL) {
			mem_free(&buf);
			return;
		}
		MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuf, len);

		for (wst = wbuf + i; wst > wbuf && *wst != L'\r' && *wst != L'\n' && *wst != L'\t' && *wst != L' '; wst--);
		if (wst != wbuf) wst++;
		for (wen = wst; *wen != L'\0' && *wen != L'\r' && *wen != L'\n' && *wen != L'\t' && *wen != L' '; wen++);

		wstr = (WCHAR *)mem_calloc(sizeof(WCHAR) * (wen - wst + 1));
		if (wstr == NULL) {
			mem_free(&wbuf);
			mem_free(&buf);
			return;
		}
		// 選択文字列の抽出
		for (wr = wstr; wst < wen; wst++, wr++) {
			*wr = *wst;
		}
		*wr = L'\0';
		mem_free(&wbuf);
		mem_free(&buf);

		len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
		str = (char *)mem_alloc(len + 1);
		if (str == NULL) {
			mem_free(&wstr);
			return;
		}
		WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
		mem_free(&wstr);
	} else {
		str = (TCHAR *)mem_alloc(sizeof(TCHAR) * (j - i + 2));
		if (str == NULL) {
			mem_free(&buf);
			return;
		}
		// 選択文字列の抽出
		for (p = buf + i, r = str; p < (buf + j); p++, r++) {
			*r = *p;
		}
		*r = TEXT('\0');
		mem_free(&buf);
	}
#endif

	// 開始位置の取得
	for (s = str; *s == TEXT('(') || *s == TEXT(')') || *s == TEXT('\"') ||
		*s == TEXT('<') || *s == TEXT('>') || *s == TEXT('\t') || *s == TEXT(' '); s++);
	// URLの開始位置を取得
	for (p = s; *p != TEXT('\0'); p++) {
		if (str_cmp_ni_t(p, URL_HTTP, lstrlen(URL_HTTP)) == 0 ||
			str_cmp_ni_t(p, URL_HTTPS, lstrlen(URL_HTTPS)) == 0 ||
			str_cmp_ni_t(p, URL_FTP, lstrlen(URL_FTP)) == 0 ||
			str_cmp_ni_t(p, URL_MAILTO, lstrlen(URL_MAILTO)) == 0) {
			s = p;
			break;
		}
	}
	// 終了位置の取得
	for (p = s; *p != TEXT('\0'); p++) {
		if (*p == TEXT('(') || *p == TEXT(')') || *p == TEXT('\"') ||
			*p == TEXT('<') || *p == TEXT('>') ||
			*p == TEXT('\r') || *p == TEXT('\n') || *p == TEXT('\t') ||
			*p == TEXT(' ')) { // || (IsDBCSLeadByte((BYTE)*p) == TRUE && *(p + 1) != TEXT('\0'))) {
			*p = TEXT('\0');
			break;
		}
		if (*p == TEXT('@')) {
			if (MailToFlag != 0) {
				MailToFlag = -1;
			} else {
				MailToFlag = 1;
			}
		}
	}
	if (*s == TEXT('\0')) {
		mem_free(&str);
		return;
	}

	// URLのチェック
	if (str_cmp_ni_t(s, URL_HTTP, lstrlen(URL_HTTP)) == 0 ||
		str_cmp_ni_t(s, URL_HTTPS, lstrlen(URL_HTTPS)) == 0 ||
		str_cmp_ni_t(s, URL_FTP, lstrlen(URL_FTP)) == 0) {
		SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_SETSEL, (WPARAM)i, (LPARAM)i);
		ShellOpen(s);

	} else if (str_cmp_ni_t(s, URL_MAILTO, lstrlen(URL_MAILTO)) == 0 ||
		MailToFlag == 1) {
		SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_SETSEL, (WPARAM)i, (LPARAM)i);
		if (Edit_MailToSet(hInst, hWnd, s, vSelBox) == EDIT_INSIDEEDIT) {
		}
	}
	mem_free(&str);
}

/*
 * SetReMessage - 返信の設定を行う
 */
static void SetReMessage(HWND hWnd, int ReplyFag)
{
	int ret;
	if (item_is_mailbox(MailBox + vSelBox,
		(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == FALSE) {
		ErrorMessage(hWnd, STR_ERR_NOMAIL);
		return;
	}

	ret = Edit_InitInstance(hInst, hWnd, vSelBox,
		(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA), EDIT_REPLY, ReplyFag);
	if (ret != EDIT_NONEDIT) {
		if (op.ViewClose == 1) {
			SendMessage(hWnd, WM_CLOSE, 0, 0);
		}
	}
}

/*
 * Decode - 添付ファイルをデコードして保存
 */
static BOOL Decode(HWND hWnd, int id)
{
	ATTACHINFO ai;
	TCHAR *fname, *ext = NULL;
	TCHAR buf[BUF_SIZE];
	TCHAR *p, *r;
	TCHAR *str;
	char *b64str, *dstr, *endpoint;
	int len;
	int EncodeFlag = 0;
	BOOL ret = TRUE;

	SwitchCursor(FALSE);

	if ((*(tpMultiPart + id))->ePos != NULL) {
		len = (*(tpMultiPart + id))->ePos - (*(tpMultiPart + id))->sPos;
	} else {
		len = tstrlen((*(tpMultiPart + id))->sPos) + 1;
	}
	b64str = (char *)mem_alloc(sizeof(char) * (len + 1));
	if (b64str == NULL) {
		SwitchCursor(TRUE);
		return FALSE;
	}
	if (len == 0) {
		*b64str = '\0';
	} else {
		str_cpy_n(b64str, (*(tpMultiPart + id))->sPos, len - 1);
	}

	if ((*(tpMultiPart + id))->Encoding != NULL) {
		if (str_cmp_i((*(tpMultiPart + id))->Encoding, "base64") == 0) {
			EncodeFlag = 1;
		} else if (str_cmp_i((*(tpMultiPart + id))->Encoding, "quoted-printable") == 0) {
			EncodeFlag = 2;
		}
	}
	switch (EncodeFlag) {
	case 1:
	case 2:
		// エンコードされている場合はデコードを行う
		dstr = (char *)mem_alloc(tstrlen(b64str));
		if (dstr == NULL) {
			mem_free(&b64str);
			SwitchCursor(TRUE);
			return FALSE;
		}
		endpoint = ((EncodeFlag == 1) ? base64_decode : QuotedPrintable_decode)(b64str, dstr);
		mem_free(&b64str);
		break;

	default:
		dstr = b64str;
		endpoint = dstr + tstrlen(dstr);
		break;
	}

	str = alloc_char_to_tchar((*(tpMultiPart + id))->ContentType);
	if (str != NULL) {
		for (r = str; *r != TEXT('\0') && *r != TEXT(';'); r++);
		*r = TEXT('\0');
	}

	p = GetMIME2Extension(str, NULL);
	if (p != NULL) {
		ext = alloc_copy_t(p + 1);
	}

	if ((*(tpMultiPart + id))->Filename == NULL || *(*(tpMultiPart + id))->Filename == TEXT('\0')) {
		// ファイル名が無い場合
		if (p != NULL) {
			fname = mem_calloc(sizeof(TCHAR) * (8 + lstrlen(p) + 1));
			if (fname != NULL) {
				MD5_CTX context;
				unsigned char digest[16];

				MD5Init(&context);
				MD5Update(&context, dstr, endpoint - dstr);
				MD5Final(digest, &context);
				wsprintf(fname, TEXT("%02X%02X%02X%02X%s"), digest[0], digest[4], digest[8], digest[12], p);
				mem_free(&p);
			} else {
				fname = p;
			}
		} else {
			fname = alloc_copy_t(str);
		}
	} else {
		mem_free(&p);
		fname = alloc_char_to_tchar((*(tpMultiPart + id))->Filename);
	}

	if (fname != NULL) {
		// ファイル名に使えない文字は _ に変換する
		filename_conv(fname);
		// ファイル名が長すぎる場合は途中で切る
		if (lstrlen(fname) > BUF_SIZE - 50) {
			*(fname + BUF_SIZE - 50) = TEXT('\0');
		}
	}
	SwitchCursor(TRUE);

	// 確認ダイアログ
	AttachProcess = 0;
	ai.from = ((MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA))->From;
	ai.fname = fname;
	ai.mime = str;
	ai.size = endpoint - dstr;
	if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_ATTACH_NOTICE), hWnd, AttachNoticeProc, (LPARAM)&ai) != FALSE) {
		switch (AttachProcess) {
		case 0:
			// 保存
			ret = file_save(hWnd, fname, ext, dstr, endpoint - dstr);
			break;

		case 1:
			// 保存して実行
			if (op.AttachWarning == 0 ||
				MessageBox(hWnd, STR_Q_ATTACH, STR_TITLE_ATTACH_MSG, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2) == IDYES) {
				if (op.AttachDelete != 0) {
					wsprintf(buf, TEXT("%s%s"), ATTACH_FILE, fname);
					p = buf;
				} else {
					p = fname;
				}
				ret = file_save_exec(hWnd, p, dstr, endpoint - dstr);
			}
			break;
		}
	}
	mem_free(&str);
	mem_free(&ext);
	mem_free(&fname);
	mem_free(&dstr);
	return ret;
}

/*
 * DeleteAttachFile - 添付ファイルの削除
 */
static BOOL DeleteAttachFile(HWND hWnd, MAILITEM *tpMailItem)
{
	MULTIPART **tpPart = NULL;
	char *mBody;
#ifdef UNICODE
	char *ContentType;
#endif
	int cnt = 0;
	int TextIndex = -1;
	int i;

	// マルチパートの解析
#ifdef UNICODE
	ContentType = alloc_tchar_to_char(tpMailItem->ContentType);
	cnt = multipart_parse(ContentType, tpMailItem->Body, &tpPart, 0);
	mem_free(&ContentType);
#else
	cnt = multipart_parse(tpMailItem->ContentType, tpMailItem->Body, &tpPart, 0);
#endif
	if (cnt == 0) {
		return FALSE;
	}

	// テキストのパートを検索
	for (i = 0; i < cnt; i++) {
		if ((*tpPart + i)->ContentType == NULL ||
			str_cmp_ni((*tpPart + i)->ContentType, "text", tstrlen("text")) == 0) {
			TextIndex = i;
			break;
		}
	}
	if (TextIndex == -1) {
		multipart_free(&tpPart, cnt);
		return FALSE;
	}
	if ((*tpPart + TextIndex)->ePos == NULL) {
		mBody = alloc_copy((*tpPart + TextIndex)->sPos);
		if (mBody == NULL) {
			multipart_free(&tpPart, cnt);
			return FALSE;
		}
	} else {
		i = (*tpPart + TextIndex)->ePos - (*tpPart + TextIndex)->sPos;
		mBody = (char *)mem_alloc(sizeof(char) * (i + 1));
		if (mBody == NULL) {
			multipart_free(&tpPart, cnt);
			return FALSE;
		}
		if (i == 0) {
			*mBody = '\0';
		} else {
			str_cpy_n(mBody, (*tpPart + TextIndex)->sPos, i - 1);
		}
	}

	// 内容の置き換え
	mem_free(&tpMailItem->Body);
	tpMailItem->Body = mBody;
	mem_free(&tpMailItem->Encoding);
	tpMailItem->Encoding = alloc_char_to_tchar((*tpPart + TextIndex)->Encoding);
	mem_free(&tpMailItem->ContentType);
	tpMailItem->ContentType = alloc_char_to_tchar((*tpPart + TextIndex)->ContentType);
	mem_free(&tpMailItem->Attach);
	tpMailItem->Attach = alloc_copy_t(TEXT("_"));

	multipart_free(&tpPart, cnt);
	return TRUE;
}

/*
 * SaveViewMail - メールをファイルに保存
 */
static BOOL SaveViewMail(TCHAR *fname, HWND hWnd, int MailBoxIndex, MAILITEM *tpMailItem, TCHAR *head)
{
	MULTIPART **tpPart = NULL;
	HANDLE hFile;
	TCHAR path[BUF_SIZE];
	TCHAR *buf;
	TCHAR *tmp;
	int len = 0;
	int cnt = 0;

	if (fname == NULL) {
		// ファイルの作成
		if (tpMailItem->Subject != NULL) {
			str_cpy_n_t(path, tpMailItem->Subject, BUF_SIZE - 50);
			filename_conv(path);
			lstrcpy(path + lstrlen(path), TEXT(".txt"));
		} else {
			lstrcpy(path, TEXT(".txt"));
		}
		// ファイル名の取得
		if (filename_select(hWnd, path, TEXT("txt"), STR_TEXT_FILTER, FALSE) == FALSE) {
			return TRUE;
		}
		if (item_is_mailbox(MailBox + MailBoxIndex, tpMailItem) == FALSE) {
			return FALSE;
		}
		fname = path;
	}

	// 保存するファイルを開く
	hFile = CreateFile(fname, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		return FALSE;
	}
	// ヘッダの保存
	len = CreateHeaderStringSize(head, tpMailItem);
	tmp = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
	if (tmp == NULL) {
		CloseHandle(hFile);
		return FALSE;
	}
	CreateHeaderString(head, tmp, tpMailItem);

#ifdef UNICODE
	if (file_write_utf8(hFile, tmp) == FALSE) {
#else
	if (file_write_ascii(hFile, tmp, len) == FALSE) {
#endif
		mem_free(&tmp);
		CloseHandle(hFile);
		return FALSE;
	}
	mem_free(&tmp);

	buf = MIME_body_decode(tpMailItem, FALSE, &tpPart, &cnt);
	multipart_free(&tpPart, cnt);

	// 本文の保存
#ifdef UNICODE
	if (buf != NULL && file_write_utf8(hFile, buf) == FALSE) {
#else
	if (buf != NULL && file_write_ascii(hFile, buf, lstrlen(buf)) == FALSE) {
#endif
		CloseHandle(hFile);
		mem_free(&buf);
		return FALSE;
	}
	CloseHandle(hFile);
	mem_free(&buf);
	return TRUE;
}

/*
 * AppViewMail - メールをファイルに保存して外部アプリケーションで表示
 */
static BOOL AppViewMail(MAILITEM *tpMailItem)
{
	MULTIPART **tpPart = NULL;
	SHELLEXECUTEINFO sei;
	TCHAR path[BUF_SIZE];
	TCHAR *p;
	int len = 0;
	int cnt = 0;

	SetCurrentDirectory(AppDir);

	// メールをファイルに保存
	str_join_t(path, DataDir, VIEW_FILE, TEXT("."), op.ViewFileSuffix, (TCHAR *)-1);
	if (SaveViewMail(path, NULL, 0, tpMailItem, op.ViewFileHeader) == FALSE) {
		return FALSE;
	}

	ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
	sei.cbSize = sizeof(sei);
	sei.fMask = 0;
	sei.hwnd = NULL;
	sei.lpVerb = NULL;

	p = NULL;
	if (op.ViewApp == NULL || *op.ViewApp == TEXT('\0')) {
		sei.lpFile = path;
		sei.lpParameters = NULL;
	} else {
		sei.lpFile = op.ViewApp;
		p = CreateCommandLine(op.ViewAppCmdLine, path, FALSE);
		sei.lpParameters = (p != NULL) ? p : path;
	}
	sei.lpDirectory = NULL;
	sei.nShow = SW_SHOWNORMAL;
	sei.hInstApp = hInst;
	// 起動
	if (ShellExecuteEx(&sei) == FALSE) {
		mem_free(&p);
		return FALSE;
	}
	mem_free(&p);
	return TRUE;
}

/*
 * SetMark - アイテムにマークを付加
 */
static void SetMark(HWND hWnd, MAILITEM *tpMailItem, const int mark)
{
	HWND hListView;
	int i;

	hListView = GetDlgItem(MainWnd, IDC_LISTVIEW);
	i = ListView_GetMemToItem(hListView, tpMailItem);

	if (tpMailItem->Status == mark) {
		tpMailItem->Status = tpMailItem->MailStatus;
		if (i != -1) {
			if (tpMailItem->Download == FALSE) {
				ListView_SetItemState(hListView, i, LVIS_CUT, LVIS_CUT);
			}
			ListView_RedrawItems(hListView, i, i);
		}
	} else {
		tpMailItem->Status = mark;
		if (i != -1) {
			ListView_SetItemState(hListView, i, 0, LVIS_CUT);
			ListView_RedrawItems(hListView, i, i);
		}
	}
	UpdateWindow(hListView);
}

/*
 * GetMarkStatus - アイテム状態取得
 */
static void GetMarkStatus(HWND hWnd, MAILITEM *tpMailItem)
{
	HMENU hMenu;
	HWND htv;
	int enable;

	hMenu = GetMenu(hWnd);
	htv = GetDlgItem(hWnd, IDC_VTB);

	enable = (SelBox != MAILBOX_SAVE && !(vSelBox == RecvBox && ExecFlag == TRUE));

	CheckMenuItem(hMenu, ID_MENUITEM_DOWNMARK, (tpMailItem->Status == ICON_DOWN) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_MENUITEM_DELMARK, (tpMailItem->Status == ICON_DEL) ? MF_CHECKED : MF_UNCHECKED);
	EnableMenuItem(hMenu, ID_MENUITEM_DOWNMARK, !enable);
	EnableMenuItem(hMenu, ID_MENUITEM_DELMARK, !enable);

	SendMessage(htv, TB_CHECKBUTTON, ID_MENUITEM_DOWNMARK, (LPARAM) MAKELONG((tpMailItem->Status == ICON_DOWN) ? 1 : 0, 0));
	SendMessage(htv, TB_PRESSBUTTON, ID_MENUITEM_DOWNMARK, (LPARAM) MAKELONG((tpMailItem->Status == ICON_DOWN) ? 1 : 0, 0));

	SendMessage(htv, TB_CHECKBUTTON, ID_MENUITEM_DELMARK, (LPARAM) MAKELONG((tpMailItem->Status == ICON_DEL) ? 1 : 0, 0));
	SendMessage(htv, TB_PRESSBUTTON, ID_MENUITEM_DELMARK, (LPARAM) MAKELONG((tpMailItem->Status == ICON_DEL) ? 1 : 0, 0));

	SendMessage(htv, TB_ENABLEBUTTON, ID_MENUITEM_DOWNMARK, (LPARAM)MAKELONG(enable, 0));
	SendMessage(htv, TB_ENABLEBUTTON, ID_MENUITEM_DELMARK, (LPARAM)MAKELONG(enable, 0));
}

/*
 * ViewProc - メール表示プロシージャ
 */
static LRESULT CALLBACK ViewProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CREATE:
		if (InitWindow(hWnd, (MAILITEM *)(((CREATESTRUCT *)lParam)->lpCreateParams)) == FALSE) {
			DestroyWindow(hWnd);
			break;
		}
		lParam = (LPARAM)(((CREATESTRUCT *)lParam)->lpCreateParams);
	case WM_MODFYMESSAGE:
		SetWindowLong(hWnd, GWL_USERDATA, lParam);
		ModfyWindow(hWnd, (MAILITEM *)lParam, FALSE);
		_SetForegroundWindow(hWnd);
		break;

	case WM_CHANGE_MARK:
		if (item_is_mailbox(MailBox + vSelBox,
			(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == FALSE) {
			break;
		}
		GetMarkStatus(hWnd, (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA));
		break;

	case WM_SIZE:
		SetWindowSize(hWnd, wParam, lParam);
		break;

	case WM_EXITSIZEMOVE:
		if (IsWindowVisible(hWnd) != 0 && IsIconic(hWnd) == 0 && IsZoomed(hWnd) == 0) {
			GetWindowRect(hWnd, (LPRECT)&op.ViewRect);
			op.ViewRect.right -= op.ViewRect.left;
			op.ViewRect.bottom -= op.ViewRect.top;
		}
		break;

	case WM_SETFOCUS:
		SetFocus(GetDlgItem(hWnd, IDC_EDIT_BODY));
		HideCaret(GetDlgItem(hWnd, IDC_EDIT_BODY));
		FocusWnd = hWnd;
		ESCFlag = FALSE;
		break;

	case WM_ENDCLOSE:
		EndWindow(hWnd);
		hViewWnd = NULL;
		break;

	case WM_CLOSE:
		if (ESCFlag == TRUE) {
			ESCFlag = FALSE;
			break;
		}
		EndWindow(hWnd);
		hViewWnd = NULL;
		break;

	case WM_INITMENUPOPUP:
		if (LOWORD(lParam) == 1) {
			SetEditMenu(hWnd);
		}
		break;

	case WM_TIMER:
		switch (wParam) {
		case ID_CLICK_TIMER:
			KillTimer(hWnd, wParam);
			OpenURL(hWnd);
			break;

		case ID_HIDECARET_TIMER:
			KillTimer(hWnd, wParam);
			HideCaret(GetDlgItem(hWnd, IDC_EDIT_BODY));
			break;
		}
		break;

	case WM_NOTIFY:
		return NotifyProc(hWnd, lParam);

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		case ID_KEY_UP:
			SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_VSCROLL, SB_LINEUP, 0);
			break;

		case ID_KEY_DOWN:
			SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_VSCROLL, SB_LINEDOWN, 0);
			break;

		case ID_KEY_LEFT:
			if (op.WordBreakFlag == 1) {
				SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_VSCROLL, SB_PAGEUP, 0);
			} else {
				SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_HSCROLL, SB_LINELEFT, 0);
			}
			break;

		case ID_KEY_RIGHT:
			if (op.WordBreakFlag == 1) {
				View_NextScroll(GetDlgItem(hWnd, IDC_EDIT_BODY));
			} else {
				SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_HSCROLL, SB_LINERIGHT, 0);
			}
			break;

		case ID_MENUITEM_NEXTMAIL:
			View_NextMail(hWnd, FALSE);
			break;

		case ID_MENUITEM_PREVMAIL:
			View_PrevMail(hWnd);
			break;

		case ID_MENUITEM_NEXTNOREAD:
			View_NextNoReadMail(hWnd);
			break;

		case ID_MENUITEM_REMESSEGE:
			SetReMessage(hWnd, 0);
			break;

		case ID_MENUITEM_ALLREMESSEGE:
			SetReMessage(hWnd, 1);
			break;

		// 受信用にマーク
		case ID_MENUITEM_DOWNMARK:
			if (item_is_mailbox(MailBox + vSelBox,
				(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == FALSE) {
				ErrorMessage(hWnd, STR_ERR_NOMAIL);
				break;
			}
			if (vSelBox == MAILBOX_SAVE || vSelBox == MAILBOX_SEND) {
				break;
			}
			SetMark(hWnd, (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA), ICON_DOWN);
			GetMarkStatus(hWnd, (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA));
			break;

		// 削除用にマーク
		case ID_MENUITEM_DELMARK:
			if (item_is_mailbox(MailBox + vSelBox,
				(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == FALSE) {
				ErrorMessage(hWnd, STR_ERR_NOMAIL);
				break;
			}
			if (vSelBox == MAILBOX_SAVE || vSelBox == MAILBOX_SEND) {
				break;
			}
			SetMark(hWnd, (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA), ICON_DEL);
			GetMarkStatus(hWnd, (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA));
			break;

		case ID_MENUITEM_SAVE:
			if (item_is_mailbox(MailBox + vSelBox,
				(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == FALSE) {
				ErrorMessage(hWnd, STR_ERR_NOMAIL);
				break;
			}
			if (SaveViewMail(NULL, hWnd, vSelBox, (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA), SAVE_HEADER) == FALSE) {
				ErrorMessage(hWnd, STR_ERR_SAVE);
			}
			break;

		case ID_MENUITEM_PROP:
			if (item_is_mailbox(MailBox + vSelBox,
				(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == FALSE) {
				ErrorMessage(hWnd, STR_ERR_NOMAIL);
				break;
			}
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_MAILPROP), hWnd, MailPropProc, GetWindowLong(hWnd, GWL_USERDATA));
			break;

		case ID_MENUITEM_CLOSE:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		case ID_MENUITEM_COPY:
			SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_COPY , 0, 0);
			break;

		case ID_MENUITEM_ALLSELECT:
			SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_SETSEL, 0, -1);
			break;

		case ID_MENUITEM_FIND:
			View_FindMail(hWnd, TRUE);
			break;

		case ID_MENUITEM_NEXTFIND:
			View_FindMail(hWnd, FALSE);
			break;

		case ID_MENUITEM_WORDBREAK:
			DelEditSubClass(GetDlgItem(hWnd, IDC_EDIT_BODY));
			op.WordBreakFlag = SetWordBreak(hWnd);
			SetEditSubClass(GetDlgItem(hWnd, IDC_EDIT_BODY));
			HideCaret(GetDlgItem(hWnd, IDC_EDIT_BODY));
			break;

		case ID_MENUITEM_FONT:
			// フォント
			if (font_select(hWnd, &op.view_font) == TRUE) {
				if (hViewFont != NULL) {
					DeleteObject(hViewFont);
				}
				hViewFont = font_create(hWnd, &op.view_font);
				SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_SETFONT, (WPARAM)hViewFont, MAKELPARAM(TRUE,0));
				font_charset = op.view_font.charset;
				// Editウィンドウのフォントを設定
				EnumWindows((WNDENUMPROC)enum_windows_proc, 0);
				if (item_is_mailbox(MailBox + vSelBox, (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == TRUE) {
					ModfyWindow(hWnd, (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA), FALSE);
				}
			}
			break;

		case ID_MENUITEM_SHOW_DATE:
			op.ViewShowDate = (op.ViewShowDate == 1) ? 0 : 1;
			SetHeaderSize(hWnd);
			if (item_is_mailbox(MailBox + vSelBox,
				(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == TRUE) {
				SetHeaderString(GetDlgItem(hWnd, IDC_HEADER),
					(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA));
			}
			break;

		case ID_VIEW_SOURCE:
		case ID_VIEW_PART:
			if (item_is_mailbox(MailBox + vSelBox,
				(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == FALSE) {
				ErrorMessage(hWnd, STR_ERR_NOMAIL);
				break;
			}
			ModfyWindow(hWnd, (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA),
				((GET_WM_COMMAND_ID(wParam, lParam) == ID_VIEW_SOURCE) ? TRUE : FALSE));
			break;

		case ID_VIEW_DELETE_ATTACH:
			if (MessageBox(hWnd, STR_Q_DELATTACH, STR_TITLE_DELETE, MB_ICONEXCLAMATION | MB_YESNO) == IDNO) {
				break;
			}
			if (item_is_mailbox(MailBox + vSelBox,
				(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == FALSE) {
				ErrorMessage(hWnd, STR_ERR_NOMAIL);
				break;
			}
			DeleteAttachFile(hWnd, (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA));
			ModfyWindow(hWnd, (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA), FALSE);
			break;

		case ID_MENUITEM_VIEW:
			if (item_is_mailbox(MailBox + vSelBox,
				(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == FALSE) {
				ErrorMessage(hWnd, STR_ERR_NOMAIL);
				break;
			}
			if (AppViewMail((MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == FALSE) {
				ErrorMessage(hWnd, STR_ERR_VIEW);
				break;
			}
			if (op.ViewAppClose == 1) {
				SendMessage(hWnd, WM_CLOSE, 0, 0);
			}
			break;

		default:
			{
				int id = GET_WM_COMMAND_ID(wParam, lParam);
				if (id >= ID_ATTACH && id <= ID_ATTACH + 100) {
					if (item_is_mailbox(MailBox + vSelBox,
						(MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA)) == FALSE) {
						ErrorMessage(hWnd, STR_ERR_NOMAIL);
						break;
					}
					if (Decode(hWnd, id - ID_ATTACH) == FALSE) {
						ErrorMessage(hWnd, STR_ERR_SAVE);
					}
				}
			}
			break;
		}
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/*
 * View_InitApplication - ウィンドウクラスの登録
 */
BOOL View_InitApplication(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = 0;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU_VIEW);
	wc.lpfnWndProc = (WNDPROC)ViewProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_READ));
	wc.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
	wc.lpszClassName = VIEW_WND_CLASS;

	return RegisterClass(&wc);
}

/*
 * View_InitInstance - ウィンドウの作成
 */
BOOL View_InitInstance(HINSTANCE hInstance, LPVOID lpParam, BOOL NoAppFlag)
{
	int key;

	key = GetKeyState(VK_SHIFT);
	if (NoAppFlag == FALSE && ((op.DefViewApp == 1 && key >= 0) || (op.DefViewApp == 0 && key < 0))) {
		// ビューアで表示
		if (AppViewMail((MAILITEM *)lpParam) == FALSE) {
			ErrorMessage(MainWnd, STR_ERR_VIEW);
			return FALSE;
		}

		// 開封済みにする
		if (((MAILITEM *)lpParam)->MailStatus != ICON_NON && ((MAILITEM *)lpParam)->MailStatus < ICON_SENDMAIL) {
			((MAILITEM *)lpParam)->MailStatus = ICON_READ;
		}

		// 一覧のアイコンの設定
		if (((MAILITEM *)lpParam)->Status != ICON_DOWN && ((MAILITEM *)lpParam)->Status != ICON_DEL) {
			int LvFocus;
			LvFocus = ListView_GetNextItem(GetDlgItem(MainWnd, IDC_LISTVIEW), -1, LVNI_FOCUSED);
			((MAILITEM *)lpParam)->Status = ((MAILITEM *)lpParam)->MailStatus;
			ListView_RedrawItems(GetDlgItem(MainWnd, IDC_LISTVIEW), LvFocus, LvFocus);
			UpdateWindow(GetDlgItem(MainWnd, IDC_LISTVIEW));
		}
		return FALSE;
	}

	if (hViewWnd != NULL) {
		SendMessage(hViewWnd, WM_MODFYMESSAGE, 0, (LPARAM)lpParam);
		return TRUE;
	}
	hViewWnd = CreateWindow(VIEW_WND_CLASS,
		STR_TITLE_MAILVIEW,
		WS_OVERLAPPEDWINDOW,
		op.ViewRect.left,
		op.ViewRect.top,
		op.ViewRect.right,
		op.ViewRect.bottom,
		NULL, NULL, hInstance, lpParam);
	if (hViewWnd == NULL) {
		return FALSE;
	}

	ShowWindow(hViewWnd, SW_SHOW);
	UpdateWindow(hViewWnd);
	return TRUE;
}
/* End of source */
