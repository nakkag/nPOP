/*
 * nPOP
 *
 * Edit.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include "General.h"
#include "Memory.h"
#include "String.h"
#include "Charset.h"
#include "mime.h"
#include "multipart.h"
#ifdef USE_NEDIT
#include "nEdit.h"
#endif
#include "dpi.h"

/* Define */
#define ID_MENU				(WM_APP + 102)

#define IDC_VCB				2000
#define IDC_VTB				2001
#define IDC_HEADER			2002
#define IDC_EDIT_BODY		2003

#define ID_APP_TIMER		1
#define ID_WAIT_TIMER		2

#define REPLY_SUBJECT		TEXT("Re:")

/* Global Variables */
HWND hEditWnd = NULL;
TCHAR *tmp_attach;
static int EditMaxLength;
static BOOL ProcessFlag;

// 外部参照
extern OPTION op;

extern HINSTANCE hInst;  // Local copy of hInstance
extern TCHAR *AppDir;
extern TCHAR *DataDir;
extern HWND MainWnd;
extern HWND FocusWnd;
extern HWND hViewWnd;
extern HFONT hListFont;
extern MAILBOX *MailBox;
extern int SelBox;

extern SOCKET g_soc;
extern BOOL gSockFlag;

extern HWND hViewWnd;
extern HFONT hViewFont;
extern int font_charset;

/* Local Function Prototypes */
static int GetCcListSize(TCHAR *To, TCHAR *MyMailAddress, TCHAR *ToMailAddress);
static TCHAR *SetCcList(TCHAR *To, TCHAR *MyMailAddress, TCHAR *ToMailAddress, TCHAR *ret);
static void SetAllReMessage(MAILITEM *tpMailItem, MAILITEM *tpReMailItem);
static void SetReplyMessage(MAILITEM *tpMailItem, MAILITEM *tpReMailItem, int rebox, int ReplyFag);
static void SetReplyMessageBody(MAILITEM *tpMailItem, MAILITEM *tpReMailItem);
static void SetWindowString(HWND hWnd, TCHAR *Subject);
static void SetHeaderString(HWND hHeader, MAILITEM *tpMailItem);
static LRESULT TbNotifyProc(HWND hWnd,LPARAM lParam);
static LRESULT NotifyProc(HWND hWnd, LPARAM lParam);
static BOOL InitWindow(HWND hWnd, MAILITEM *tpMailItem);
static BOOL SetWindowSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL EndWindow(HWND hWnd);
static void SetEditMenu(HWND hWnd);
static BOOL SetItemToSendBox(HWND hWnd, BOOL BodyFlag, int EndFlag);
static BOOL CloseEditMail(HWND hWnd, BOOL SendFlag, BOOL ShowFlag);
static void ShowSendInfo(HWND hWnd);
static BOOL AppEditMail(HWND hWnd, long id, char *buf, MAILITEM *tpMailItem);
static BOOL ReadEditMail(HWND hWnd, long id, MAILITEM *tpMailItem, BOOL ReadFlag);
static LRESULT CALLBACK EditProc(HWND hWnd, UINT msg, WPARAM wParam,LPARAM lParam);

/*
 * enum_windows_proc - ウィンドウ列挙プロシージャ
 */
BOOL CALLBACK enum_windows_proc(const HWND hWnd, const LPARAM lParam)
{
	TCHAR class_name[BUF_SIZE];

	// クラス名取得
	GetClassName(hWnd, class_name, BUF_SIZE - 1);
	// フォント設定
	if (lstrcmp(class_name, EDIT_WND_CLASS) == 0) {
		SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_SETFONT, (WPARAM)hViewFont, MAKELPARAM(TRUE,0));
	}
	return TRUE;
}

/*
 * GetCcListSize - メールアドレスのリストの長さを取得する
 */
static int GetCcListSize(TCHAR *To, TCHAR *MyMailAddress, TCHAR *ToMailAddress)
{
	TCHAR *p;
	int cnt = 0;

	if (To == NULL) {
		return cnt;
	}

	p = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(To) + 1));
	if (p == NULL) {
		return cnt;
	}
	while (*To != TEXT('\0')) {
		GetMailAddress(To, p, FALSE);
		if ((MyMailAddress != NULL && lstrcmpi(MyMailAddress, p) == 0) ||
			(ToMailAddress != NULL && lstrcmpi(ToMailAddress, p) == 0)) {
			// 自分のアドレスか To に設定されたアドレスの場合はカウントしない
			To = GetMailString(To, p);
		} else {
			To = GetMailString(To, p);
			cnt += 4;
			// メールアドレスのサイズを取得
			cnt += lstrlen(p);
		}
		To = (*To != TEXT('\0')) ? To + 1 : To;
	}
	mem_free(&p);
	return cnt;
}

/*
 * SetCcList - メールアドレスをCcのリストに追加する
 */
static TCHAR *SetCcList(TCHAR *To, TCHAR *MyMailAddress, TCHAR *ToMailAddress, TCHAR *ret)
{
	TCHAR *p, *r;

	*ret = TEXT('\0');

	if (To == NULL) {
		return ret;
	}

	r = ret;
	p = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(To) + 1));
	if (p == NULL) {
		return ret;
	}
	while (*To != TEXT('\0')) {
		GetMailAddress(To, p, FALSE);
		if ((MyMailAddress != NULL && lstrcmpi(MyMailAddress, p) == 0) ||
			(ToMailAddress != NULL && lstrcmpi(ToMailAddress, p) == 0)) {
			// 自分のアドレスか To に設定されたアドレスの場合は追加しない
			To = GetMailString(To, p);
		} else {
			To = GetMailString(To, p);
			if (ret != r) {
				// 区切りの追加
				r = str_cpy_t(r, TEXT(",\r\n "));
			}
			r = str_cpy_t(r, p);
		}
		To = (*To != TEXT('\0')) ? To + 1 : To;
	}
	mem_free(&p);
	*r = TEXT('\0');
	return r;
}

/*
 * SetAllReMessage - 全員に返信の設定を行う
 */
static void SetAllReMessage(MAILITEM *tpMailItem, MAILITEM *tpReMailItem)
{
	TCHAR *MyMailAddress = NULL;
	TCHAR *ToMailAddress = NULL;
	TCHAR *r;
	int ToSize;
	int CcSize;
	int FromSize = 0;
	int i;

	// 自分のメールアドレスの取得
	i = mailbox_name_to_index(tpMailItem->MailBox);
	if (i != -1) {
		MyMailAddress = (MailBox + i)->MailAddress;
	}
	// 送信先のメールアドレスの取得
	if (tpMailItem->To != NULL) {
		ToMailAddress = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(tpMailItem->To) + 1));
		if (ToMailAddress != NULL) {
			GetMailAddress(tpMailItem->To, ToMailAddress, FALSE);
		}
	}

	// サイズを取得
	ToSize = GetCcListSize(tpReMailItem->To, MyMailAddress, ToMailAddress);
	CcSize = GetCcListSize(tpReMailItem->Cc, MyMailAddress, ToMailAddress);
	if (tpReMailItem->ReplyTo != NULL) {
		// ReplyTo が設定されている場合は From を Cc に追加する
		FromSize = GetCcListSize(tpReMailItem->From, MyMailAddress, ToMailAddress);
	}
	if ((FromSize + ToSize + CcSize) <= 0) {
		mem_free(&ToMailAddress);
		return;
	}
	ToSize += ((ToSize > 0 && CcSize > 0) ? 4 : 0);
	ToSize += (((ToSize > 0 || CcSize > 0) && FromSize > 0) ? 4 : 0);

	tpMailItem->Cc = (TCHAR *)mem_alloc(sizeof(TCHAR) * (ToSize + CcSize + FromSize + 1));
	if (tpMailItem->Cc == NULL) {
		mem_free(&ToMailAddress);
		return;
	}
	// To を Cc のリストに追加する
	*tpMailItem->Cc = TEXT('\0');
	r = SetCcList(tpReMailItem->To, MyMailAddress, ToMailAddress, tpMailItem->Cc);
	if (CcSize > 0 && *tpMailItem->Cc != TEXT('\0')) {
		r = str_cpy_t(r, TEXT(",\r\n "));
	}
	// Cc を Cc のリストに追加する
	r = SetCcList(tpReMailItem->Cc, MyMailAddress, ToMailAddress, r);
	// From を Cc のリストに追加する
	if (FromSize > 0) {
		if (*tpMailItem->Cc != TEXT('\0')) {
			r = str_cpy_t(r, TEXT(",\r\n "));
		}
		SetCcList(tpReMailItem->From, MyMailAddress, ToMailAddress, r);
	}
	mem_free(&ToMailAddress);
}

/*
 * SetReplyMessage - 返信メールの設定を行う
 */
static void SetReplyMessage(MAILITEM *tpMailItem, MAILITEM *tpReMailItem, int rebox, int ReplyFag)
{
	TCHAR *p;
	TCHAR *subject;
	int len = 0;

	// 返信のMailBoxの設定
	if (rebox >= MAILBOX_USER) {
		tpMailItem->MailBox = alloc_copy_t((MailBox + rebox)->Name);
	} else if (tpReMailItem->MailBox != NULL) {
		tpMailItem->MailBox = alloc_copy_t(tpReMailItem->MailBox);
	}

	// 返信の宛先の設定
	if (tpReMailItem->ReplyTo != NULL) {
		tpMailItem->To = alloc_copy_t(tpReMailItem->ReplyTo);
	} else if (tpReMailItem->From != NULL) {
		tpMailItem->To = alloc_copy_t(tpReMailItem->From);
	}

	// 全員に返信の場合は Cc を設定
	if (ReplyFag == 1) {
		SetAllReMessage(tpMailItem, tpReMailItem);
	}

	if (tpReMailItem->MessageID != NULL && *tpReMailItem->MessageID == TEXT('<')) {
		// 返信のIn-Reply-Toの設定
		tpMailItem->InReplyTo = alloc_copy_t(tpReMailItem->MessageID);

		// 返信のReferencesの設定
		if (tpReMailItem->InReplyTo != NULL && *tpReMailItem->InReplyTo == TEXT('<')) {
			tpMailItem->References = (TCHAR *)mem_alloc(
				sizeof(TCHAR) * (lstrlen(tpReMailItem->InReplyTo) + lstrlen(tpReMailItem->MessageID) + 2));
			if (tpMailItem->References != NULL) {
				str_join_t(tpMailItem->References, tpReMailItem->InReplyTo, TEXT(" "), tpReMailItem->MessageID, (TCHAR *)-1);
			}
		} else {
			tpMailItem->References = alloc_copy_t(tpReMailItem->MessageID);
		}
	}

	// 返信の件名を設定
	subject = (tpReMailItem->Subject != NULL) ? tpReMailItem->Subject : TEXT("");
	if (str_cmp_ni_t(subject, op.ReSubject, lstrlen(op.ReSubject)) == 0) {
		subject += lstrlen(op.ReSubject);
	} else if (str_cmp_ni_t(subject, REPLY_SUBJECT, lstrlen(REPLY_SUBJECT)) == 0) {
		subject += lstrlen(REPLY_SUBJECT);
	}
	for (; *subject == TEXT(' '); subject++);
	p = tpMailItem->Subject = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(subject) + lstrlen(op.ReSubject) + 1));
	if (tpMailItem->Subject != NULL) {
		str_join_t(p, op.ReSubject, subject, (TCHAR *)-1);
	}
}

/*
 * SetReplyMessageBody - 返信メールの本文の設定を行う
 */
static void SetReplyMessageBody(MAILITEM *tpMailItem, MAILITEM *tpReMailItem)
{
	MULTIPART **tpMultiPart = NULL;
	TCHAR *p, *mBody;
	TCHAR *body;
	int len;
	int cnt, i;

	// 本文の設定
	if (tpMailItem->Status == 1 && tpReMailItem != NULL && tpReMailItem->Body != NULL) {
		mBody = MIME_body_decode(tpReMailItem, FALSE, &tpMultiPart, &cnt);
		multipart_free(&tpMultiPart, cnt);

		len = CreateHeaderStringSize(op.ReHeader, tpReMailItem) + 2;
		len += GetReplyBodySize(mBody, op.QuotationChar);

		i = mailbox_name_to_index(tpMailItem->MailBox);
		if (i != -1 && (MailBox + i)->Signature != NULL && *(MailBox + i)->Signature != TEXT('\0')) {
			len += lstrlen((MailBox + i)->Signature) + 2;
		}

		body = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
		if (body != NULL) {
			p = CreateHeaderString(op.ReHeader, body, tpReMailItem);
			p = str_cpy_t(p, TEXT("\r\n"));
			if (mBody != NULL) {
				p = SetReplyBody(mBody, p, op.QuotationChar);
			}
			if (i != -1 && (MailBox + i)->Signature != NULL && *(MailBox + i)->Signature != TEXT('\0')) {
				str_join_t(p, TEXT("\r\n"), (MailBox + i)->Signature, (TCHAR *)-1);
			}
		}
		mem_free(&mBody);

		if (tpMailItem->BodyCharset == NULL) {
			tpMailItem->BodyCharset = alloc_copy_t(op.BodyCharset);
			tpMailItem->BodyEncoding = op.BodyEncoding;
		}
		if ((tpMailItem->Body = MIME_charset_encode(charset_to_cp((BYTE)font_charset), body, tpMailItem->BodyCharset)) == NULL) {
#ifdef UNICODE
			tpMailItem->Body = alloc_tchar_to_char(body);
#else
			tpMailItem->Body = alloc_copy(body);
#endif
		}
		mem_free(&body);
	} else {
		i = mailbox_name_to_index(tpMailItem->MailBox);
		if (i == -1 || (MailBox + i)->Signature == NULL || *(MailBox + i)->Signature == TEXT('\0')) {
			return;
		}
		len = lstrlen((MailBox + i)->Signature);
		body = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 3));
		if (body != NULL) {
			str_join_t(body, TEXT("\r\n"), (MailBox + i)->Signature, (TCHAR *)-1);
		}
		if (tpMailItem->BodyCharset == NULL) {
			tpMailItem->BodyCharset = alloc_copy_t(op.BodyCharset);
			tpMailItem->BodyEncoding = op.BodyEncoding;
		}
		if ((tpMailItem->Body = MIME_charset_encode(charset_to_cp((BYTE)font_charset), body, tpMailItem->BodyCharset)) == NULL) {
#ifdef UNICODE
			tpMailItem->Body = alloc_tchar_to_char(body);
#else
			tpMailItem->Body = alloc_copy(body);
#endif
		}
		mem_free(&body);
	}
}

/*
 * SetWindowString - ウィンドウタイトルの設定
 */
static void SetWindowString(HWND hWnd, TCHAR *Subject)
{
	TCHAR *buf;

	if (Subject == NULL) {
		SetWindowText(hWnd, STR_TITLE_MAILEDIT);
		return;
	}
	buf = (TCHAR *)mem_alloc(sizeof(TCHAR) *
		(lstrlen(STR_TITLE_MAILEDIT) + lstrlen(Subject) + lstrlen(TEXT(" - []")) + 1));
	if (buf == NULL) {
		SetWindowText(hWnd, STR_TITLE_MAILEDIT);
		return;
	}
	str_join_t(buf, STR_TITLE_MAILEDIT TEXT(" - ["), Subject, TEXT("]"), (TCHAR *)-1);
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
	TCHAR *buf, *p;
	int len = 0;

	len += lstrlen(STR_EDIT_HEAD_MAILBOX);
	if (tpMailItem->MailBox != NULL) {
		len += lstrlen(tpMailItem->MailBox);
	}
	len += lstrlen(STR_EDIT_HEAD_TO);
	if (tpMailItem->To != NULL) {
		len += lstrlen(tpMailItem->To);
	}
	len += SetCcAddressSize(tpMailItem->Cc);
	len += SetCcAddressSize(tpMailItem->Bcc);
	len += lstrlen(STR_EDIT_HEAD_SUBJECT);
	if (tpMailItem->Subject != NULL) {
		len += lstrlen(tpMailItem->Subject);
	}
	len += SetAttachListSize(tpMailItem->Attach);
	buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
	if (buf == NULL) {
		return;
	}

	p = str_join_t(buf, STR_EDIT_HEAD_MAILBOX, tpMailItem->MailBox, STR_EDIT_HEAD_TO, tpMailItem->To, (TCHAR *)-1);
	p = SetCcAddress(TEXT("Cc"), tpMailItem->Cc, p);
	p = SetCcAddress(TEXT("Bcc"), tpMailItem->Bcc, p);
	p = str_join_t(p, STR_EDIT_HEAD_SUBJECT, tpMailItem->Subject, (TCHAR *)-1);
	SetAttachList(tpMailItem->Attach, p);

	SetWindowText(hHeader, buf);
	mem_free(&buf);
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
 * InitWindow - ウィンドウの初期化
 */
static BOOL InitWindow(HWND hWnd, MAILITEM *tpMailItem)
{
	HDC hdc;
	HFONT hFont;
	HWND hToolBar;
	TEXTMETRIC lptm;
	RECT rcClient, StRect;
	TCHAR *buf;
	TCHAR *tmp;
	int Height;
	int FontHeight;
	TBBUTTON tbButton[] = {
		{0,	ID_MENUITEM_SEND,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{1,	ID_MENUITEM_SENDBOX,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{0,	0,						TBSTATE_ENABLED,	TBSTYLE_SEP,	0, 0, 0, -1},
		{2,	ID_MENUITEM_SENDINFO,	TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
	};
	RECT ToolbarRect;

	if (tpMailItem == NULL) {
		return FALSE;
	}
	SetWindowString(hWnd, tpMailItem->Subject);

	if (GetAwareness() != PROCESS_DPI_UNAWARE && GetScale() >= 300) {
		hToolBar = CreateToolbarEx(hWnd, WS_CHILD | TBSTYLE_TOOLTIPS, IDC_VTB, 3, hInst, IDB_TOOLBAR_EDIT_48,
			tbButton, sizeof(tbButton) / sizeof(TBBUTTON), 0, 0, 48, 48, sizeof(TBBUTTON));
	}
	else if (GetAwareness() != PROCESS_DPI_UNAWARE && GetScale() >= 150) {
		hToolBar = CreateToolbarEx(hWnd, WS_CHILD | TBSTYLE_TOOLTIPS, IDC_VTB, 3, hInst, IDB_TOOLBAR_EDIT_32,
			tbButton, sizeof(tbButton) / sizeof(TBBUTTON), 0, 0, 32, 32, sizeof(TBBUTTON));
	}
	else {
		hToolBar = CreateToolbarEx(hWnd, WS_CHILD | TBSTYLE_TOOLTIPS, IDC_VTB, 3, hInst, IDB_TOOLBAR_EDIT,
			tbButton, sizeof(tbButton) / sizeof(TBBUTTON), 0, 0, TB_ICONSIZE, TB_ICONSIZE, sizeof(TBBUTTON));
	}
	SetWindowLong(hToolBar, GWL_STYLE, GetWindowLong(hToolBar, GWL_STYLE) | TBSTYLE_FLAT);
	SendMessage(hToolBar, TB_SETINDENT, 5, 0);
	ShowWindow(hToolBar,SW_SHOW);

	GetWindowRect(hToolBar, &ToolbarRect);
	Height = ToolbarRect.bottom - ToolbarRect.top;
	GetClientRect(hWnd, &rcClient);

	// ヘッダを表示するSTATICコントロールの作成
	CreateWindowEx(
		0,
		TEXT("STATIC"), TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_BORDER | SS_LEFTNOWORDWRAP | SS_NOPREFIX,
		0, Height, rcClient.right, 0,
		hWnd, (HMENU)IDC_HEADER, hInst, NULL);

	// フォントの設定
	SendDlgItemMessage(hWnd, IDC_HEADER, WM_SETFONT,
		(WPARAM)((hListFont != NULL) ? hListFont : GetStockObject(DEFAULT_GUI_FONT)), MAKELPARAM(TRUE,0));

	hdc = GetDC(GetDlgItem(hWnd, IDC_HEADER));
	hFont = SelectObject(hdc, (hListFont != NULL) ? hListFont : GetStockObject(DEFAULT_GUI_FONT));
	// フォントの高さを取得
	GetTextMetrics(hdc, &lptm);
	if (hFont != NULL) {
		SelectObject(hdc, hFont);
	}
	ReleaseDC(GetDlgItem(hWnd, IDC_HEADER), hdc);
	FontHeight = (lptm.tmHeight + lptm.tmExternalLeading) * 3;

	// 一時的に設定してサイズを再計算する
	MoveWindow(GetDlgItem(hWnd, IDC_HEADER), 0, Height, rcClient.right, FontHeight, TRUE);
	GetClientRect(GetDlgItem(hWnd, IDC_HEADER), &StRect);
	FontHeight = FontHeight + (FontHeight - StRect.bottom) + 1;
	MoveWindow(GetDlgItem(hWnd, IDC_HEADER), 0, Height, rcClient.right, FontHeight, TRUE);

	// 本文を表示するEDITコントロールの作成
	Height += FontHeight + 1;
#ifndef USE_NEDIT
	CreateWindowEx(
		0,
		TEXT("EDIT"), TEXT(""),
		WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_VSCROLL | ((op.EditWordBreakFlag == 1) ? 0 : WS_HSCROLL),
		0, Height, rcClient.right, rcClient.bottom - Height,
		hWnd, (HMENU)IDC_EDIT_BODY, hInst, NULL);
#else
	CreateWindowEx(
		0,
		NEDIT_WND_CLASS, TEXT(""),
		WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_VSCROLL | ((op.EditWordBreakFlag == 1) ? 0 : WS_HSCROLL),
		0, Height, rcClient.right, rcClient.bottom - Height,
		hWnd, (HMENU)IDC_EDIT_BODY, hInst, NULL);
#endif
	if (hViewFont != NULL) {
		SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_SETFONT, (WPARAM)hViewFont, MAKELPARAM(TRUE,0));
	}
	SetFocus(GetDlgItem(hWnd, IDC_EDIT_BODY));
	SetWordBreakMenu(hWnd, NULL, (op.EditWordBreakFlag == 1) ? MF_CHECKED : MF_UNCHECKED);

	{
		OSVERSIONINFO os_info;

		os_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&os_info);

		EditMaxLength = (os_info.dwPlatformId == VER_PLATFORM_WIN32_NT) ? 0 : EDITMAXSIZE;
	}
	SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_LIMITTEXT, (WPARAM)EditMaxLength, 0);

	SetHeaderString(GetDlgItem(hWnd, IDC_HEADER), tpMailItem);
	if (tpMailItem->Body != NULL) {
		SwitchCursor(FALSE);
		if (tpMailItem->BodyCharset == NULL ||
			(tmp = MIME_charset_decode(charset_to_cp((BYTE)font_charset), tpMailItem->Body, tpMailItem->BodyCharset)) == NULL) {
#ifdef UNICODE
			tmp = alloc_char_to_tchar(tpMailItem->Body);
#else
			tmp = alloc_copy(tpMailItem->Body);
#endif
		}
		if (tmp != NULL) {
			buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(tmp) + 1));
			if (buf != NULL) {
				DelDot(tmp, buf);
				if (EditMaxLength != 0 && (int)lstrlen(buf) > EditMaxLength) {
					*(buf + EditMaxLength) = TEXT('\0');
				}
				SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_SETTEXT, 0, (LPARAM)buf);
				mem_free(&buf);
			}
		}
		mem_free(&tmp);
		SwitchCursor(TRUE);
	}
	SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_SETMODIFY, (WPARAM)FALSE, 0);

	tpMailItem->hEditWnd = hWnd;
	return TRUE;
}

/*
 * SetWindowSize - ウィンドウのサイズ変更
 */
static BOOL SetWindowSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HWND hHeader, hBody;
	RECT rcClient, HeaderRect, ToolbarRect;
	int hHeight, tHeight;

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
static BOOL EndWindow(HWND hWnd)
{
	MAILITEM *tpMailItem;

	tpMailItem = (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA);
	if (tpMailItem != NULL) {
		if (SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_GETMODIFY, 0, 0) == TRUE &&
			MessageBox(hWnd, STR_Q_EDITCANSEL,
				STR_TITLE_MAILEDIT, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2) == IDNO) {
			return FALSE;
		}
		tpMailItem->hEditWnd = NULL;
		if (tpMailItem->hProcess != NULL) {
			ReadEditMail(hWnd, (long)hWnd, tpMailItem, FALSE);
			tpMailItem->hProcess = NULL;
		}
		if (item_is_mailbox(MailBox + MAILBOX_SEND, tpMailItem) == FALSE) {
			item_free(&tpMailItem, 1);
		}
	}
	if (IsWindowVisible(hWnd) != 0 && IsIconic(hWnd) == 0 && IsZoomed(hWnd) == 0) {
		GetWindowRect(hWnd, (LPRECT)&op.EditRect);
		op.EditRect.right -= op.EditRect.left;
		op.EditRect.bottom -= op.EditRect.top;
	}
	DestroyWindow(GetDlgItem(hWnd, IDC_VTB));
	DestroyWindow(GetDlgItem(hWnd, IDC_HEADER));
	DestroyWindow(GetDlgItem(hWnd, IDC_EDIT_BODY));

	DestroyWindow(hWnd);
	hEditWnd = NULL;
	return TRUE;
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
	EnableMenuItem(hMenu, ID_MENUITEM_CUT, (i < j) ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, ID_MENUITEM_COPY, (i < j) ? MF_ENABLED : MF_GRAYED);
}

/*
 * SetItemToSendBox - 送信箱に保存
 */
static BOOL SetItemToSendBox(HWND hWnd, BOOL BodyFlag, int EndFlag)
{
	MAILITEM *tpMailItem;
	TCHAR *buf, *tmp;
	TCHAR numbuf[10];
	int len;
	int i;

	tpMailItem = (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA);
	if (tpMailItem == NULL) {
		return FALSE;
	}

	if (BodyFlag == FALSE) {
		// 機種依存文字のチェック
		if (EndFlag == 0 && CheckDependence(hWnd, IDC_EDIT_BODY) == FALSE) {
			return FALSE;
		}

		// 件名が設定されていない場合は送信情報を表示する
		if (EndFlag == 0 && (tpMailItem->Subject == NULL || *tpMailItem->Subject == TEXT('\0'))) {
			if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SETSEND), hWnd, SetSendProc,
				(LPARAM)tpMailItem) == FALSE) {
				return FALSE;
			}
		}

		// 本文を設定
		SwitchCursor(FALSE);
		mem_free(&tpMailItem->Body);
		len = SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_GETTEXTLENGTH, 0, 0) + 1;
		buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
		if (buf != NULL) {
			*buf = TEXT('\0');
			SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_GETTEXT, len, (LPARAM)buf);
			// 自動折り返し
			tmp = (TCHAR *)mem_alloc(
				sizeof(TCHAR) * (WordBreakStringSize(buf, op.QuotationChar, op.WordBreakSize, op.QuotationBreak) + 1));
			if (tmp != NULL) {
				WordBreakString(buf, tmp, op.QuotationChar, op.WordBreakSize, op.QuotationBreak);
				mem_free(&buf);
				buf = tmp;
			}
			// 行頭の . を .. に変換
			len = SetDotSize(buf);
			tmp = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
			if (tmp != NULL) {
				SetDot(buf, tmp);
			}
			mem_free(&buf);
			if (tpMailItem->BodyCharset == NULL) {
				tpMailItem->BodyCharset = alloc_copy_t(op.BodyCharset);
				tpMailItem->BodyEncoding = op.BodyEncoding;
			}
			if ((tpMailItem->Body = MIME_charset_encode(charset_to_cp((BYTE)font_charset), tmp, tpMailItem->BodyCharset)) == NULL) {
#ifdef UNICODE
				tpMailItem->Body = alloc_tchar_to_char(tmp);
#else
				tpMailItem->Body = alloc_copy(tmp);
#endif
			}
			mem_free(&tmp);
		}
		SwitchCursor(TRUE);
	}

	// サイズを設定
	mem_free(&tpMailItem->Size);
	wsprintf(numbuf, TEXT("%d"), (tpMailItem->Body != NULL) ? tstrlen(tpMailItem->Body) : 0);
	tpMailItem->Size = alloc_copy_t(numbuf);

	if (item_is_mailbox(MailBox + MAILBOX_SEND, tpMailItem) == FALSE) {
		if (item_add(MailBox + MAILBOX_SEND, tpMailItem) == FALSE) {
			return FALSE;
		}
		if (EndFlag == 0 && SelBox == MAILBOX_SEND) {
			ListView_InsertItemEx(GetDlgItem(MainWnd, IDC_LISTVIEW),
				(TCHAR *)LPSTR_TEXTCALLBACK, 0, I_IMAGECALLBACK, (long)tpMailItem,
				ListView_GetItemCount(GetDlgItem(MainWnd, IDC_LISTVIEW)));
			SetItemCntStatusText(MainWnd, NULL);
		}
	}
	if (EndFlag == 0) {
		if (op.SelectSendBox == 1 && SelBox != MAILBOX_SEND) {
			mailbox_select(MainWnd, MAILBOX_SEND);
		}
		if (SelBox == MAILBOX_SEND) {
			i = ListView_GetMemToItem(GetDlgItem(MainWnd, IDC_LISTVIEW), tpMailItem);
			if (i != -1) {
				ListView_SetItemState(GetDlgItem(MainWnd, IDC_LISTVIEW), i,
					((tpMailItem->Attach != NULL && *tpMailItem->Attach != TEXT('\0')) ? INDEXTOSTATEIMAGEMASK(1) : 0),
					LVIS_STATEIMAGEMASK)

				ListView_SetItemState(GetDlgItem(MainWnd, IDC_LISTVIEW), -1, 0, LVIS_SELECTED);
				ListView_SetItemState(GetDlgItem(MainWnd, IDC_LISTVIEW),
					i, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

				ListView_EnsureVisible(GetDlgItem(MainWnd, IDC_LISTVIEW), i, TRUE);
				ListView_RedrawItems(GetDlgItem(MainWnd, IDC_LISTVIEW), i, i);
				UpdateWindow(GetDlgItem(MainWnd, IDC_LISTVIEW));
			}
		}
	}
	return TRUE;
}

/*
 * CloseEditMail - 編集メールを閉じる
 */
static BOOL CloseEditMail(HWND hWnd, BOOL SendFlag, BOOL ShowFlag)
{
	MAILITEM *tpMailItem;

	tpMailItem = (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA);
	if (tpMailItem == NULL) {
		return FALSE;
	}

	tpMailItem->hEditWnd = NULL;
	tpMailItem->hProcess = NULL;

	if (op.AutoSave == 1) {
		// 送信箱をファイルに保存
		file_save_mailbox(SENDBOX_FILE, MailBox + MAILBOX_SEND, 2);
	}

	SetWindowLong(hWnd, GWL_USERDATA, (LPARAM)0);
	EndWindow(hWnd);
	if (SendFlag == TRUE) {
		SendMessage(MainWnd, WM_SMTP_SENDMAIL, 0, (LPARAM)tpMailItem);
	}
	return TRUE;
}

/*
 * ShowSendInfo - 送信情報表示
 */
static void ShowSendInfo(HWND hWnd)
{
	MAILITEM *tpMailItem;

	tpMailItem = (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA);
	if (tpMailItem == NULL) {
		return;
	}
	if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SETSEND), hWnd, SetSendProc,
		(LPARAM)tpMailItem) == FALSE) {
		return;
	}
	SetWindowString(hWnd, tpMailItem->Subject);
	SetHeaderString(GetDlgItem(hWnd, IDC_HEADER), tpMailItem);
}

/*
 * AppEditMail - 外部エディタで編集
 */
static BOOL AppEditMail(HWND hWnd, long id, char *buf, MAILITEM *tpMailItem)
{
	HANDLE hFile;
	TCHAR path[BUF_SIZE];
	TCHAR *p;

	SetCurrentDirectory(AppDir);

	wsprintf(path, TEXT("%s%ld.%s"), DataDir, id, op.EditFileSuffix);

	// 保存するファイルを開く
	hFile = CreateFile(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		return FALSE;
	}
	// メールの保存
	if (buf != NULL && file_write(hFile, buf, tstrlen(buf)) == FALSE) {
		CloseHandle(hFile);
		DeleteFile(path);
		return FALSE;
	}
	CloseHandle(hFile);

	{
		SHELLEXECUTEINFO sei;

		ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
		sei.cbSize = sizeof(sei);
		sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
		sei.hwnd = NULL;
		sei.lpVerb = NULL;
		p = NULL;
		if (op.EditApp == NULL || *op.EditApp == TEXT('\0')) {
			sei.lpFile = path;
			sei.lpParameters = NULL;
		} else {
			sei.lpFile = op.EditApp;
			p = CreateCommandLine(op.EditAppCmdLine, path, FALSE);
			sei.lpParameters = (p != NULL) ? p : path;
		}
		sei.lpDirectory = NULL;
		sei.nShow = SW_SHOWNORMAL;
		sei.hInstApp = hInst;
		// 起動
		if (ShellExecuteEx(&sei) == FALSE) {
			mem_free(&p);
			DeleteFile(path);
			return FALSE;
		}
		mem_free(&p);
		tpMailItem->hProcess = sei.hProcess;
	}
	if (tpMailItem->hProcess == NULL) {
		DeleteFile(path);
		return FALSE;
	}
	return TRUE;
}

/*
 * ReadEditMail - ファイルをメール本文として読み込む
 */
static BOOL ReadEditMail(HWND hWnd, long id, MAILITEM *tpMailItem, BOOL ReadFlag)
{
	TCHAR path[BUF_SIZE];
	TCHAR *tmp, *p;
	char *fbuf;
	int len;

	SetCurrentDirectory(AppDir);

	wsprintf(path, TEXT("%s%ld.%s"), DataDir, id, op.EditFileSuffix);

	if (ReadFlag == FALSE) {
		DeleteFile(path);
		return TRUE;
	}

	len = file_get_size(path);
	fbuf = file_read(path, len);
#ifdef UNICODE
	// UNICODEに変換
	tmp = alloc_char_to_tchar(fbuf);
	mem_free(&fbuf);
	p = tmp;
#else
	p = fbuf;
#endif
	if (p != NULL) {
		mem_free(&tpMailItem->Body);

		// 自動折り返し
		tmp = (TCHAR *)mem_alloc(
			sizeof(TCHAR) * (WordBreakStringSize(p, op.QuotationChar, op.WordBreakSize, op.QuotationBreak) + 1));
		if (tmp != NULL) {
			WordBreakString(p, tmp, op.QuotationChar, op.WordBreakSize, op.QuotationBreak);
			mem_free(&p);
			p = tmp;
		}

		// 行頭の . を .. に変換
		len = SetDotSize(p);
#ifdef UNICODE
		tmp = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
		if (tmp != NULL) {
			SetDot(p, tmp);
		}
		tpMailItem->Body = alloc_tchar_to_char(tmp);
		mem_free(&tmp);
#else
		tpMailItem->Body = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
		if (tpMailItem->Body != NULL) {
			SetDot(p, tpMailItem->Body);
		}
#endif
		mem_free(&p);
	}

	DeleteFile(path);
	return TRUE;
}

/*
 * EditProc - メール表示プロシージャ
 */
static LRESULT CALLBACK EditProc(HWND hWnd, UINT msg, WPARAM wParam,LPARAM lParam)
{
	MAILITEM *tpMailItem;

	switch (msg) {
	case WM_CREATE:
		if (InitWindow(hWnd, (MAILITEM *)(((CREATESTRUCT *)lParam)->lpCreateParams)) == FALSE) {
			DestroyWindow(hWnd);
			break;
		}
		SetWindowLong(hWnd, GWL_USERDATA, (LPARAM)((CREATESTRUCT *)lParam)->lpCreateParams);
		SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES);
		break;

	case WM_DROPFILES:
		tpMailItem = (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA);
		if (tpMailItem != NULL) {
			TCHAR fpath[BUF_SIZE], *p;
			int i, len, size = 0;

			len = DragQueryFile((HANDLE)wParam, 0xFFFFFFFF, NULL, 0);
			// サイズ取得
			for (i = 0; i < len; i++) {
				DragQueryFile((HANDLE)wParam, i, fpath, BUF_SIZE - 1);
				if (dir_check(fpath) == FALSE) {
					size += lstrlen(fpath) + 1;
				}
			}
			if (size == 0) {
				DragFinish((HANDLE)wParam);
				break;
			}
			// メモリ確保
			p = tmp_attach = (TCHAR *)mem_alloc(sizeof(TCHAR) * (size + 1));
			if (tmp_attach == NULL) {
				DragFinish((HANDLE)wParam);
				break;
			}
			// ファイルリスト生成
			for (i = 0; i < len; i++) {
				if (p != tmp_attach) {
					*(p++) = ATTACH_SEP;
				}
				DragQueryFile((HANDLE)wParam, i, fpath, BUF_SIZE - 1);
				if (dir_check(fpath) == FALSE) {
					p = str_cpy_t(p, fpath);
				}
			}
			DragFinish((HANDLE)wParam);
			// 添付設定ダイアログ表示
			SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_ATTACH, 0);
			mem_free(&tmp_attach);
			tmp_attach = NULL;
		}
		break;

	case WM_SIZE:
		SetWindowSize(hWnd, wParam, lParam);
		break;

	case WM_SETFOCUS:
		SetFocus(GetDlgItem(hWnd, IDC_EDIT_BODY));
		FocusWnd = hWnd;
		break;

	case WM_ENDCLOSE:
		{
			BOOL BodyFlag = FALSE;

			tpMailItem = (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA);
			if (tpMailItem != NULL && tpMailItem->hProcess != NULL) {
				ReadEditMail(hWnd, (long)hWnd, tpMailItem, TRUE);
				BodyFlag = TRUE;
				tpMailItem->hProcess = NULL;
			}
			if (SetItemToSendBox(hWnd, BodyFlag, wParam) == TRUE) {
				CloseEditMail(hWnd, FALSE, FALSE);
			}
		}
		break;

	case WM_CLOSE:
		if (EndWindow(hWnd) == FALSE) {
			break;
		}
		break;

	case WM_INITMENUPOPUP:
		if (LOWORD(lParam) == 1) {
			SetEditMenu(hWnd);
		}
		break;

	case WM_TIMER:
		switch (wParam) {
		case ID_APP_TIMER:
			KillTimer(hWnd, wParam);
			tpMailItem = (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA);
			if (tpMailItem == NULL) {
				break;
			}

			// 外部エディタ起動
			if (AppEditMail(hWnd, (long)hWnd, tpMailItem->Body, tpMailItem) == FALSE) {
				ShowWindow(hWnd, SW_SHOW);
				UpdateWindow(hWnd);
				break;
			}
			SetTimer(hWnd, ID_WAIT_TIMER, 1, NULL);
			break;

		case ID_WAIT_TIMER:
			tpMailItem = (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA);
			if (tpMailItem == NULL || tpMailItem->hProcess == NULL) {
				KillTimer(hWnd, wParam);
				break;
			}
			// 外部エディタの終了監視
			if (WaitForSingleObject(tpMailItem->hProcess, 0) == WAIT_TIMEOUT) {
				break;
			}
			KillTimer(hWnd, wParam);
			tpMailItem->hProcess = NULL;

			// ファイルの読み直し
			ReadEditMail(hWnd, (long)hWnd, tpMailItem, TRUE);

			// 送信箱に保存
			if (SetItemToSendBox(hWnd, TRUE, 0) == TRUE) {
				CloseEditMail(hWnd, FALSE, TRUE);
			} else {
				ShowWindow(hWnd, SW_SHOW);
				UpdateWindow(hWnd);
			}
			break;
		}
		break;

	case WM_NOTIFY:
		return NotifyProc(hWnd, lParam);

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam,lParam)) {
		case ID_MENUITEM_SEND:
			if (MessageBox(hWnd, STR_Q_SENDMAIL,
				STR_TITLE_SEND, MB_ICONQUESTION | MB_YESNO) == IDNO) {
				break;
			}
			if (g_soc != -1 || gSockFlag == TRUE) {
				ErrorMessage(NULL, STR_ERR_SENDLOCK);
				break;
			}
			if (SetItemToSendBox(hWnd, FALSE, 0) == TRUE) {
				CloseEditMail(hWnd, TRUE, TRUE);
			}
			break;

		case ID_MENUITEM_SENDBOX:
			if (SetItemToSendBox(hWnd, FALSE, 0) == TRUE) {
				CloseEditMail(hWnd, FALSE, TRUE);
			}
			break;

		case ID_MENUITEM_SENDINFO:
			ShowSendInfo(hWnd);
			break;

		case ID_MENUITEM_ATTACH:
			tpMailItem = (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA);
			if (tpMailItem != NULL &&
				DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_ATTACH), hWnd, SetAttachProc, (LPARAM)tpMailItem) == TRUE) {
				int st, i;

				SetHeaderString(GetDlgItem(hWnd, IDC_HEADER), tpMailItem);
				if (tpMailItem->Attach != NULL && *tpMailItem->Attach != TEXT('\0')) {
					tpMailItem->Multipart = TRUE;
					st = INDEXTOSTATEIMAGEMASK(1);
				} else {
					tpMailItem->Multipart = FALSE;
					st = 0;
				}
				if (SelBox == MAILBOX_SEND) {
					i = ListView_GetMemToItem(GetDlgItem(MainWnd, IDC_LISTVIEW), tpMailItem);
					if (i != -1) {
						ListView_SetItemState(GetDlgItem(MainWnd, IDC_LISTVIEW), i, st, LVIS_STATEIMAGEMASK)
						ListView_RedrawItems(GetDlgItem(MainWnd, IDC_LISTVIEW), i, i);
						UpdateWindow(GetDlgItem(MainWnd, IDC_LISTVIEW));
					}
				}
			}
			break;

		case ID_MENUITEM_ENCODE:
			// エンコード設定
			tpMailItem = (MAILITEM *)GetWindowLong(hWnd, GWL_USERDATA);
			if (tpMailItem != NULL) {
				DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_ENCODE), hWnd, SetEncodeProc, (LPARAM)tpMailItem);
			}
			break;

		case ID_MENUITEM_CLOSE:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		case ID_MENUITEM_UNDO:
			SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_UNDO, 0, 0);
			break;

		case ID_MENUITEM_CUT:
			SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_CUT , 0, 0);
			break;

		case ID_MENUITEM_COPY:
			SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_COPY , 0, 0);
			break;

		case ID_MENUITEM_PASTE:
			SendDlgItemMessage(hWnd, IDC_EDIT_BODY, WM_PASTE , 0, 0);
			break;

		case ID_MENUITEM_ALLSELECT:
			SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_SETSEL, 0, -1);
			break;

		case ID_MENUITEM_FILEOPEN:
			{
				TCHAR *buf;

				if (file_read_select(hWnd, &buf) == FALSE) {
					ErrorMessage(hWnd, STR_ERR_OPEN);
					break;
				}
				if (buf == NULL) {
					break;
				}
				SwitchCursor(FALSE);
				SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)buf);
				SwitchCursor(TRUE);

				mem_free(&buf);
			}
			break;

		case ID_MENUITEM_WORDBREAK:
			op.EditWordBreakFlag = SetWordBreak(hWnd);
			SendDlgItemMessage(hWnd, IDC_EDIT_BODY, EM_LIMITTEXT, (WPARAM)EditMaxLength, 0);
			break;

		case ID_MENUITEM_FONT:
			// フォント
			if (font_select(hWnd, &op.view_font) == TRUE) {
				if (hViewFont != NULL) {
					DeleteObject(hViewFont);
				}
				hViewFont = font_create(hWnd, &op.view_font);
				if (hViewWnd != NULL) {
					SendDlgItemMessage(hViewWnd, IDC_EDIT_BODY, WM_SETFONT, (WPARAM)hViewFont, MAKELPARAM(TRUE,0));
				}
				font_charset = op.view_font.charset;
				// 他のEditウィンドウのフォントを設定
				EnumWindows((WNDENUMPROC)enum_windows_proc, 0);
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
 * Edit_InitApplication - ウィンドウクラスの登録
 */
BOOL Edit_InitApplication(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = 0;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU_EDIT);
	wc.lpfnWndProc = (WNDPROC)EditProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_SENDMAIL));
	wc.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
	wc.lpszClassName = EDIT_WND_CLASS;

	return RegisterClass(&wc);
}

/*
 * Edit_MailToSet - メールアドレスから送信メールの作成
 */
int Edit_MailToSet(HINSTANCE hInstance, HWND hWnd, TCHAR *mail_addr, int rebox)
{
	static BOOL ExistFlag = FALSE;
	MAILITEM *tpMailItem;
	BOOL ret;

	if (ExistFlag == TRUE) {
		return EDIT_NONEDIT;
	}

	tpMailItem = (MAILITEM *)mem_calloc(sizeof(MAILITEM));
	if (tpMailItem == NULL) {
		ErrorMessage(hWnd, STR_ERR_MEMALLOC);
		return FALSE;
	}

	// URL(mailto:)をメールアイテムに設定
	if (URLToMailItem(mail_addr, tpMailItem) == FALSE) {
		item_free(&tpMailItem, 1);
		return FALSE;
	}
	ExistFlag = TRUE;
	ret = Edit_InitInstance(hInstance, hWnd, rebox, tpMailItem, EDIT_NEW, 0);
	item_free(&tpMailItem, 1);
	ExistFlag = FALSE;
	return ret;
}

/*
 * Edit_InitInstance - ウィンドウの作成
 */
int Edit_InitInstance(HINSTANCE hInstance, HWND hWnd, int rebox, MAILITEM *tpReMailItem, int OpenFlag, int ReplyFag)
{
	MAILITEM *tpMailItem;
	int key;

	key = GetKeyState(VK_SHIFT);

	// 編集する種類によって初期化
	switch (OpenFlag) {
	case EDIT_OPEN:
		// 既存の送信メールを開く
		if (tpReMailItem == NULL) {
			return EDIT_NONEDIT;
		}
		if (tpReMailItem->hEditWnd != NULL) {
			if (IsWindowVisible(tpReMailItem->hEditWnd) == TRUE) {
				ShowWindow(tpReMailItem->hEditWnd, SW_SHOW);
				if (IsIconic(tpReMailItem->hEditWnd) != 0) {
					ShowWindow(tpReMailItem->hEditWnd, SW_RESTORE);
				}
				_SetForegroundWindow(tpReMailItem->hEditWnd);
			}
			return EDIT_INSIDEEDIT;
		}
		tpMailItem = tpReMailItem;
		break;

	case EDIT_NEW:
		// 新規にメールを作成
		tpMailItem = (MAILITEM *)mem_calloc(sizeof(MAILITEM));
		if (tpMailItem == NULL) {
			ErrorMessage(hWnd, STR_ERR_MEMALLOC);
			return EDIT_NONEDIT;
		}
		if (tpReMailItem != NULL) {
			item_copy(tpReMailItem, tpMailItem);
		}
		tpMailItem->Download = TRUE;
		// 送信情報設定
		_SetForegroundWindow(hWnd);
		if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SETSEND), hWnd, SetSendProc, (LPARAM)tpMailItem) == FALSE) {
			item_free(&tpMailItem, 1);
			return EDIT_NONEDIT;
		}
		if (tpMailItem->Body == NULL) {
			SetReplyMessageBody(tpMailItem, NULL);
		}
		break;

	case EDIT_REPLY:
		// 返信
		tpMailItem = (MAILITEM *)mem_calloc(sizeof(MAILITEM));
		if (tpMailItem == NULL) {
			ErrorMessage(hWnd, STR_ERR_MEMALLOC);
			return EDIT_NONEDIT;
		}
		tpMailItem->Download = TRUE;
		if (tpReMailItem != NULL) {
			// 返信設定
			SetReplyMessage(tpMailItem, tpReMailItem, rebox, ReplyFag);
			if (tpReMailItem->Body != NULL) {
				tpMailItem->Status = 1;
			}
		}
		// 送信情報設定
		if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SETSEND), hWnd, SetSendProc, (LPARAM)tpMailItem) == FALSE) {
			item_free(&tpMailItem, 1);
			return EDIT_NONEDIT;
		}
		SetReplyMessageBody(tpMailItem, tpReMailItem);
		tpMailItem->Status = 0;
		break;
	}

	hEditWnd = CreateWindow(EDIT_WND_CLASS,
		STR_TITLE_MAILEDIT,
		WS_OVERLAPPEDWINDOW,
		op.EditRect.left,
		op.EditRect.top,
		op.EditRect.right,
		op.EditRect.bottom,
		NULL, NULL, hInstance, (LPVOID)tpMailItem);
	if (hEditWnd == NULL) {
		if (OpenFlag != EDIT_OPEN) {
			item_free(&tpMailItem, 1);
		}
		ErrorMessage(hWnd, STR_ERR_INIT);
		return EDIT_NONEDIT;
	}
	if ((op.DefEditApp == 1 && key >= 0) || (op.DefEditApp == 0 && key < 0)) {
		ShowWindow(hEditWnd, SW_HIDE);
		SetTimer(hEditWnd, ID_APP_TIMER, 1, NULL);
		return EDIT_OUTSIDEEDIT;
	} else {
		ShowWindow(hEditWnd, SW_SHOW);
		UpdateWindow(hEditWnd);
	}
	return EDIT_INSIDEEDIT;
}
/* End of source */
