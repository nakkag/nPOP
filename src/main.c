/*
 * nPOP
 *
 * main.c
 *
 * Copyright (C) 1996-2020 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include "General.h"
#include "Memory.h"
#include "String.h"
#include "Font.h"
#include "Charset.h"
#ifdef USE_NEDIT
#include "nEdit.h"
#endif
#include "dpi.h"

/* Define */
#define WM_TRAY_NOTIFY			(WM_APP + 100)		// タスクトレイ
#define WM_FINDMAILBOX			(WM_APP + 101)

#define ID_MENU					(WM_APP + 102)		// コントロールID
#define IDC_CB					2000
#define IDC_TB					2001

#define ID_MAILITEM_OPEN		(WM_APP + 300)		// メールOpen用ID

#define ID_RECV_TIMER			1					// タイマーID
#define ID_SMTP_TIMER			2
#define ID_SMTP_ONE_TIMER		3
#define ID_CHECK_TIMER			4
#define ID_EXEC_TIMER			5
#define ID_AUTOCHECK_TIMER		6
#define ID_TIMEOUT_TIMER		7
#define ID_NEWMAIL_TIMER		8

#define RECVTIME				1					// タイマーインターバル
#define SMTPTIME				100
#define CHECKTIME				100
#define AUTOCHECKTIME			60000
#define TIMEOUTTIME				60000

#define TRAY_ID					100					// タスクトレイID

#define CMD_RSET				"RSET"
#define CMD_QUIT				"QUIT"

/* Global Variables */
HINSTANCE hInst;							// Local copy of hInstance
TCHAR *AppDir;								// アプリケーションパス
TCHAR *DataDir;								// データ保存先のパス
TCHAR *g_Pass;								// 一時パスワード
int gPassSt;								// 一時パスワード保存フラグ
static TCHAR *CmdLine;						// コマンドライン
BOOL first_start;							// 初回起動フラグ
BOOL PPCFlag;								// PsPCフラグ
static int confirm_flag;					// 認証フラグ

HWND MainWnd;								// メインウィンドウのハンドル
HWND FocusWnd;								// フォーカスを持つウィンドウのハンドル
HFONT hListFont;							// ListViewのフォント
HFONT hViewFont;							// 表示のフォント
int font_charset;
static HICON TrayIcon_Main;					// タスクトレイアイコン (待機)
static HICON TrayIcon_Check;				// タスクトレイアイコン (チェック中)
static HICON TrayIcon_Mail;					// タスクトレイアイコン (新着あり)
BOOL NewMail_Flag;							// タスクトレイアイコン用新着フラグ
static HMENU hPOPUP;						// タスクトレイアイコン用のポップアップメニュー
static HANDLE hAccel, hViewAccel, hEditAccel;	// アクセラレータのハンドル
int MailMenuPos;							// メニュー位置

static WNDPROC ListViewWindowProcedure;		// サブクラス用プロシージャ(ListView)
int LvSortFlag;								// ListViewのソートフラグ
BOOL EndThreadSortFlag;						// 通信終了時の自動ソートフラグ(スレッド表示用)

MAILBOX *MailBox;							// メールボックス
MAILBOX *AddressBox;						// アドレス帳
int MailBoxCnt = 2;							// メールボックス数
int SelBox;									// 選択中のメールボックス
int RecvBox;								// 送受信中のメールボックス
static int CheckBox;						// チェック中のメールボックス

SOCKET g_soc = -1;							// ソケット
BOOL gSockFlag;								// 通信中フラグ
BOOL GetHostFlag;							// ホスト名解決中フラグ
int NewMailCnt;								// 新着メール数
BOOL ShowMsgFlag;							// 新着有りのメッセージ表示中
static BOOL ShowError;						// エラーメッセージ表示中
BOOL AutoCheckFlag;							// 自動チェック
BOOL PopBeforeSmtpFlag;						// POP before SMTP
BOOL KeyShowHeader;							// キーによる一時的なヘッダ表示フラグ
static BOOL AllCheck;						// 巡回中
BOOL ExecFlag;								// 実行中
static BOOL ExecCheckFlag;					// 実行後チェックのチェック判定
static int AutoCheckCnt;					// 自動チェック開始までの分数カウント
static int SmtpWait;						// POP before SMTP で認証後の待ち時間 (ミリ秒)
static MAILITEM *wkSendMailItem;			// 送信用メールアイテム

typedef BOOL (*PPROC)(HWND, SOCKET, char*, int, TCHAR*, MAILBOX*, BOOL);
static PPROC command_proc;					// 送受信プロシージャ
int command_status;							// 送受信コマンドステータス (POP_, SMTP_)

// 外部参照
extern OPTION op;

extern TCHAR *FindStr;						// 検索文字列
extern HWND hViewWnd;						// 表示ウィンドウ
extern HWND MsgWnd;							// メール到着メッセージウィンドウ

/* Local Function Prototypes */
static BOOL GetAppPath(HINSTANCE hinst);
static BOOL ConfirmPass(HWND hWnd, TCHAR *ps);
static BOOL TrayMessage(HWND hWnd, DWORD dwMessage, UINT uID, HICON hIcon, TCHAR *pszTip);
static void SetTrayIcon(HWND hWnd, HICON hIcon, TCHAR *buf);
static void FreeAllMailBox(void);
static void CloseViewWindow(int Flag);
static LRESULT CALLBACK SubClassListViewProc(HWND hWnd, UINT msg, WPARAM wParam,LPARAM lParam);
static void SetListViewSubClass(HWND hWnd);
static void DelListViewSubClass(HWND hWnd);
static LRESULT ListViewHeaderNotifyProc(HWND hWnd, LPARAM lParam);
static LRESULT TbNotifyProc(HWND hWnd,LPARAM lParam);
static LRESULT NotifyProc(HWND hWnd, WPARAM wParam, LPARAM lParam);
static int CreateComboBox(HWND hWnd, int Top);
static BOOL InitWindow(HWND hWnd);
static BOOL SetWindowSize(HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL SaveWindow(HWND hWnd);
static BOOL EndWindow(HWND hWnd);
static BOOL SendMail(HWND hWnd, MAILITEM *tpMailItem, int end_cmd);
static BOOL RecvMailList(HWND hWnd, int BoxIndex, BOOL SmtpFlag);
static BOOL MailMarkCheck(HWND hWnd, BOOL DelMsg, BOOL NoMsg);
static BOOL ExecItem(HWND hWnd, int BoxIndex);
static void OpenItem(HWND hWnd, BOOL MsgFlag, BOOL NoAppFlag);
static void ReMessageItem(HWND hWnd);
static void ItemToSaveBox(HWND hWnd);
static void ListDeleteItem(HWND hWnd);
static void SetDownloadMark(HWND hWnd, BOOL Flag);
static void SetDeleteMark(HWND hWnd);
static void UnMark(HWND hWnd);
static void SetMailStats(HWND hWnd, int St);
static void EndSocketFunc(HWND hWnd);
static BOOL CheckEndAutoExec(HWND hWnd, int SocBox, int cnt, BOOL AllFlag);
static void Init_NewMailFlag(void);
static void NewMail_Massage(HWND hWnd, int cnt);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL InitApplication(HINSTANCE hInstance);
static HWND InitInstance(HINSTANCE hInstance, int CmdShow);

/*
 * GetAppPath - ユーザディレクトリの作成
 */

static BOOL GetAppPath(HINSTANCE hinst)
{
	TCHAR *p, *r;

	AppDir = (TCHAR *)mem_calloc(sizeof(TCHAR) * BUF_SIZE);
	if (AppDir == NULL) {
		return FALSE;
	}

	// アプリケーションのパスを取得
	GetModuleFileName(hinst, AppDir, BUF_SIZE - 1);
	for (p = r = AppDir; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE && *(p + 1) != TEXT('\0')) {
			p++;
			continue;
		}
#endif	// UNICODE
		if (*p == TEXT('\\') || *p == TEXT('/')) {
			r = p;
		}
	}
	*r = TEXT('\0');

	lstrcat(AppDir, TEXT("\\"));
	return TRUE;
}

/*
 * CommandLine - コマンドラインを処理する
 */
static BOOL CommandLine(HWND hWnd, TCHAR *buf)
{
	TCHAR name[BUF_SIZE];
	TCHAR *p, *r;
	int i;

	if (buf == NULL || *buf == TEXT('\0')) {
		return FALSE;
	}
	for (p = buf; *p == TEXT(' '); p++);
	if (*p == TEXT('\0')) return FALSE;
	if (*p != TEXT('/')) return TRUE;
	p++;
	if (str_cmp_ni_t(p, TEXT("a:"), 2) != 0) {
		for (; *p != TEXT('\0') && *p != TEXT(' '); p++);
		for (; *p == TEXT(' '); p++);
		lstrcpy(buf, p);
		return ((*buf == TEXT('\0')) ? FALSE : TRUE);
	}
	p += 2;

	// メールボックスの取得
	if (*p == TEXT('\"')) {
		p++;
		for (r = p; *r != TEXT('\0') && *r != TEXT('\"'); r++);
	} else {
		for (r = p; *r != TEXT('\0') && *r != TEXT(' '); r++);
	}
	str_cpy_n_t(name, p, r - p + 1);
	i = mailbox_name_to_index(name);
	mailbox_select(hWnd, i);

	for (p = r; *p != TEXT('\0') && *p != TEXT(' '); p++);
	for (; *p == TEXT(' '); p++);
	lstrcpy(buf, p);
	return ((*buf == TEXT('\0')) ? FALSE : TRUE);
}

/*
 * ConfirmPass - パスワードの確認
 */
static BOOL ConfirmPass(HWND hWnd, TCHAR *ps)
{
	while (1) {
		// 起動パスワード
		gPassSt = 0;
		if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_INPUTPASS), NULL, InputPassProc,
			(LPARAM)STR_TITLE_SHOWPASSWORD) == FALSE) {
			return FALSE;
		}
		if (g_Pass == NULL || lstrcmp(ps, g_Pass) != 0) {
			ErrorMessage(NULL, STR_ERR_SOCK_BADPASSWORD);
			continue;
		}
		break;
	}
	confirm_flag = 2;
	return TRUE;
}

/*
 * TrayMessage - タスクトレイのアイコンの設定
 */
static BOOL TrayMessage(HWND hWnd, DWORD dwMessage, UINT uID, HICON hIcon, TCHAR *pszTip)
{
	NOTIFYICONDATA tnd;

	tnd.cbSize = sizeof(NOTIFYICONDATA);
	tnd.hWnd = hWnd;
	tnd.uID	= uID;
	tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	tnd.uCallbackMessage = WM_TRAY_NOTIFY;
	tnd.hIcon = hIcon;
	lstrcpy(tnd.szTip, (pszTip == NULL) ? TEXT("") : pszTip);
	return Shell_NotifyIcon(dwMessage, &tnd);
}

/*
 * SetTrayIcon - タスクトレイにアイコンを設定する
 */
static void SetTrayIcon(HWND hWnd, HICON hIcon, TCHAR *buf)
{
	if (op.ShowTrayIcon != 1 || hIcon == NULL) {
		return;
	}
	if (TrayMessage(hWnd, NIM_MODIFY, TRAY_ID, hIcon, buf) == FALSE) {
		// 変更できなかった場合は追加を行う
		TrayMessage(hWnd, NIM_ADD, TRAY_ID, hIcon, buf);
	}
}

/*
 * SwitchCursor - カーソルの状態の切り替え
 */
void SwitchCursor(const BOOL Flag)
{
	static HCURSOR hCursor = NULL;

	if (Flag == FALSE) {
		hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	} else {
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	}
}

/*
 * _SetForegroundWindow - ウィンドウをアクティブにする
 */
BOOL _SetForegroundWindow(const HWND hWnd)
{
#ifndef SPI_GETFOREGROUNDLOCKTIMEOUT
#define SPI_GETFOREGROUNDLOCKTIMEOUT		  0x2000
#endif
#ifndef SPI_SETFOREGROUNDLOCKTIMEOUT
#define SPI_SETFOREGROUNDLOCKTIMEOUT		  0x2001
#endif
	int nTargetID, nForegroundID;
	UINT nTimeout;
	BOOL ret;

	nForegroundID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	nTargetID = GetWindowThreadProcessId(hWnd, NULL);
	AttachThreadInput(nTargetID, nForegroundID, TRUE);

	SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &nTimeout, 0);
	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)0, 0);

	ret = SetForegroundWindow(hWnd);

	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)nTimeout, 0);

	AttachThreadInput(nTargetID, nForegroundID, FALSE);
	return ret;
}

/*
 * SetStatusTextT - ステータスの表示 (TCHAR)
 */
void SetStatusTextT(HWND hWnd, TCHAR *buf, int Part)
{
	SendDlgItemMessage(hWnd, IDC_STATUS, SB_SETTEXT, (WPARAM)Part, (LPARAM)buf);
}

/*
 * SetSocStatusTextT - ステータスの表示 (TCHAR)
 */
void SetSocStatusTextT(HWND hWnd, TCHAR *buf)
{
	TCHAR *st_buf;

	if (PPCFlag == FALSE && RecvBox >= MAILBOX_USER && (MailBox + RecvBox)->Name != NULL) {
		st_buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen((MailBox + RecvBox)->Name) + lstrlen(buf) + 4));
		if (st_buf == NULL) {
			return;
		}
		str_join_t(st_buf, TEXT("["), (MailBox + RecvBox)->Name, TEXT("] "), buf, (TCHAR *)-1);
		SendDlgItemMessage(hWnd, IDC_STATUS, SB_SETTEXT, (WPARAM)1, (LPARAM)st_buf);
		if (op.SocLog == 1) log_save(AppDir, LOG_FILE, st_buf);
		mem_free(&st_buf);
	} else {
		SendDlgItemMessage(hWnd, IDC_STATUS, SB_SETTEXT, (WPARAM)1, (LPARAM)buf);
		if (op.SocLog == 1) log_save(AppDir, LOG_FILE, buf);
	}
}

/*
 * SetSocStatusText - ステータスの表示
 */
#ifdef UNICODE
void SetSocStatusText(HWND hWnd, char *buf)
{
	TCHAR *wbuf;

	wbuf = alloc_char_to_tchar(buf);
	if (wbuf == NULL) {
		return;
	}
	SetSocStatusTextT(hWnd, wbuf);
	mem_free(&wbuf);
}
#endif

/*
 * SetItemCntStatusText - アイテム数の表示
 */
void SetItemCntStatusText(HWND hWnd, MAILBOX *tpViewMailBox)
{
	MAILBOX *tpMailBox;
	MAILITEM *tpMailItem;
	TCHAR wbuf[BUF_SIZE];
	int ItemCnt;
	int NewCnt = 0;
	int NoReadCnt = 0;
	int i;

	tpMailBox = (MailBox + SelBox);
	if (tpMailBox == NULL || (tpViewMailBox != NULL && tpViewMailBox != tpMailBox)) {
		return;
	}

	ItemCnt = ListView_GetItemCount(GetDlgItem(hWnd, IDC_LISTVIEW));

	if (SelBox == MAILBOX_SAVE || SelBox == MAILBOX_SEND) {
		wsprintf(wbuf, STR_STATUS_VIEWINFO, ItemCnt);
		SetStatusTextT(hWnd, wbuf, 0);
		if (g_soc == -1) {
			SetStatusTextT(hWnd, TEXT(""), 1);
		}
		return;
	}

	wsprintf(wbuf, STR_STATUS_MAILBOXINFO, ItemCnt, tpMailBox->MailCnt);
	SetStatusTextT(hWnd, wbuf, 0);

	if (g_soc != -1) {
		return;
	}
	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		tpMailItem = *(tpMailBox->tpMailItem + i);
		if (tpMailItem == NULL) {
			continue;
		}
		if (tpMailItem->New == TRUE) {
			NewCnt++;
		}
		if (tpMailItem->MailStatus == ICON_MAIL) {
			NoReadCnt++;
		}
	}
	wsprintf(wbuf, STR_STATUS_MAILINFO, NewCnt, NoReadCnt);
	SetStatusTextT(hWnd, wbuf, 1);
}

/*
 * SetStatusRecvLen - 送受信バイト数の表示
 */
void SetStatusRecvLen(HWND hWnd, int len, TCHAR *msg)
{
	TCHAR wbuf[BUF_SIZE];

	wsprintf(wbuf, STR_STATUS_SOCKINFO, len, msg);
	SetStatusTextT(hWnd, wbuf, 1);
}

/*
 * ErrorMessage - エラーメッセージ
 */
void ErrorMessage(HWND hWnd, TCHAR *buf)
{
	SwitchCursor(TRUE);
	ShowError = TRUE;
	MessageBox(hWnd, buf, STR_TITLE_ERROR, MB_OK | MB_ICONERROR);
	ShowError = FALSE;
}

/*
 * SocketErrorMessage - ソケットのエラーメッセージ
 */
void SocketErrorMessage(HWND hWnd, TCHAR *buf, int BoxIndex)
{
	MAILBOX *tpMailBox;
	TCHAR *Title;
	TCHAR *p;

	tpMailBox = MailBox + BoxIndex;
	RecvBox = -1;
	SetMailMenu(hWnd);
	SwitchCursor(TRUE);

	if (hWnd != NULL) {
		// ステータスバーにエラーの情報を表示
		SetStatusTextT(hWnd, buf, 1);
		if (op.SocLog == 1) log_save(AppDir, LOG_FILE, buf);
	}
	if (op.SocIgnoreError == 1 && BoxIndex >= MAILBOX_USER) {
		// 受信エラーを無視する設定の場合
		return;
	}
	if (op.SendIgnoreError == 1 && BoxIndex == MAILBOX_SEND) {
		// 送信エラーを無視する設定の場合
		return;
	}
	if ((BoxIndex == MAILBOX_SEND || BoxIndex >= MAILBOX_USER) &&
		tpMailBox->Name != NULL && *tpMailBox->Name != TEXT('\0')) {
		// アカウント名付きのタイトルの作成
		p = Title = (TCHAR *)mem_alloc(
			sizeof(TCHAR) * (lstrlen(STR_TITLE_ERROR TEXT(" - ")) + lstrlen(tpMailBox->Name) + 1));
		if (Title == NULL) {
			p = tpMailBox->Name;
			Title = NULL;
		} else {
			str_join_t(Title, STR_TITLE_ERROR TEXT(" - "), tpMailBox->Name, (TCHAR *)-1);
		}
	} else {
		p = STR_TITLE_ERROR;
		Title = NULL;
	}
	ShowError = TRUE;
	// エラーメッセージの表示
	MessageBox(hWnd, buf, p, MB_OK | MB_ICONERROR);
	ShowError = FALSE;
	mem_free(&Title);
}

/*
 * ErrorSocketEnd - 通信の異常終了時の処理
 */
void ErrorSocketEnd(HWND hWnd, int BoxIndex)
{
	// ソケットを閉じる
	if (g_soc != -1 && GetHostFlag == FALSE) {
		socket_close(hWnd, g_soc);
	}
	g_soc = -1;
	KillTimer(hWnd, ID_TIMEOUT_TIMER);
	if (BoxIndex == MAILBOX_SEND) {
		smtp_set_error(hWnd);
	}

#ifndef WSAASYNC
	KillTimer(hWnd, ID_RECV_TIMER);
#endif
	if (AllCheck == FALSE) {
		EndSocketFunc(hWnd);
		return;
	}

	if (op.SocIgnoreError == 1 && BoxIndex >= MAILBOX_USER) {
		// 受信エラーを無視する設定の場合
		return;
	}
	if (op.SendIgnoreError == 1 && BoxIndex == MAILBOX_SEND) {
		// 送信エラーを無視する設定の場合
		return;
	}
	// 巡回の停止
	KillTimer(hWnd, ID_SMTP_TIMER);
	KillTimer(hWnd, ID_SMTP_ONE_TIMER);
	KillTimer(hWnd, ID_CHECK_TIMER);
	KillTimer(hWnd, ID_EXEC_TIMER);
	gSockFlag = FALSE;
	AllCheck = FALSE;
	ExecFlag = FALSE;
	KeyShowHeader = FALSE;
	EndSocketFunc(hWnd);
}

/*
 * ShowMenu - マウスの位置にメニューを表示する
 */
int ShowMenu(HWND hWnd, HMENU hMenu, int mpos, int PosFlag, BOOL ReturnFlag)
{
	HMENU hShowMenu;
	HWND hListView;
	RECT WndRect;
	RECT ItemRect;
	int i;
	int x = 0, y = 0;
	DWORD ret = 0;
	POINT apos;

	_SetForegroundWindow(hWnd);
	switch (PosFlag) {
	case 0:
		// マウス位置の取得
		GetCursorPos((LPPOINT)&apos);
		x = apos.x;
		y = apos.y;
		break;

	case 1:
		// アイテム位置の取得
		hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
		i = ListView_GetNextItem(hListView, -1, LVNI_FOCUSED);
		GetWindowRect(hListView, &WndRect);
		if (i == -1) {
			x = WndRect.left;
			y = WndRect.top;
		} else {
			ListView_EnsureVisible(hListView, i, TRUE);
			ListView_GetItemRect(hListView, i, &ItemRect, LVIR_ICON);
			if (ItemRect.left < 0) {
				ItemRect.left = 0;
			}
			if (ItemRect.top < 0) {
				ItemRect.top = 0;
			}
			if (ItemRect.top > (WndRect.bottom - WndRect.top)) {
				ItemRect.top = WndRect.bottom - WndRect.top;
			}
			x = WndRect.left + ItemRect.left + (SICONSIZE / 2);
			y = WndRect.top + ItemRect.top + (SICONSIZE / 2);
		}
		break;
	}
	hShowMenu = GetSubMenu(hMenu, mpos);
	ret = TrackPopupMenu(hShowMenu,
		TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_LEFTBUTTON | ((ReturnFlag == TRUE) ? TPM_RETURNCMD : 0),
		x, y, 0, hWnd, NULL);
	PostMessage(hWnd, WM_NULL, 0, 0);
	return ret;
}

/*
 * SetMailMenu - メニューの活性／非活性の切り替え
 */
void SetMailMenu(HWND hWnd)
{
	HMENU hMenu;
	HWND hToolBar;
	int SelFlag;
	int SocFlag;
	int RecvBoxFlag;
	int SaveBoxFlag;
	int SendBoxFlag;
	int MoveBoxFlag;

	hMenu = GetMenu(hWnd);
	hToolBar = GetDlgItem(hWnd, IDC_TB);
	if (hMenu == NULL) {
		return;
	}

	SelFlag = (ListView_GetSelectedCount(GetDlgItem(hWnd, IDC_LISTVIEW)) <= 0) ? 0 : 1;
	SocFlag = (g_soc != -1 || gSockFlag == TRUE) ? 0 : 1;
	RecvBoxFlag = (SelBox == RecvBox) ? 0 : 1;
	SaveBoxFlag = (SelBox == MAILBOX_SAVE) ? 0 : 1;
	SendBoxFlag = (SelBox == MAILBOX_SEND) ? 0 : 1;
	MoveBoxFlag = (MailBoxCnt <= 3) ? 0 : 1;

	EnableMenuItem(hMenu, ID_MENUITEM_SETMAILBOX, !(RecvBoxFlag & SaveBoxFlag & SendBoxFlag));
	EnableMenuItem(hMenu, ID_MENUITEM_DELETEMAILBOX, !(RecvBoxFlag & SaveBoxFlag & SendBoxFlag));
	EnableMenuItem(hMenu, ID_MENUITEM_LISTINIT, !(SaveBoxFlag & SendBoxFlag));

	EnableMenuItem(hMenu, ID_MENUITEM_MOVEUPMAILBOX, !(SocFlag & SaveBoxFlag & SendBoxFlag & MoveBoxFlag));
	EnableMenuItem(hMenu, ID_MENUITEM_MOVEDOWNMAILBOX, !(SocFlag & SaveBoxFlag & SendBoxFlag & MoveBoxFlag));

	EnableMenuItem(hMenu, ID_MENUITEM_OPEN, !SelFlag);
	EnableMenuItem(hMenu, ID_MENUITEM_REMESSEGE, !SelFlag);

	EnableMenuItem(hMenu, ID_MENUITEM_RECV, !(SocFlag & SaveBoxFlag & SendBoxFlag));
	EnableMenuItem(hMenu, ID_MENUITEM_ALLCHECK, !SocFlag);
	EnableMenuItem(hMenu, ID_MENUITEM_EXEC, !(SocFlag & SaveBoxFlag));
	EnableMenuItem(hMenu, ID_MENUITEM_ALLEXEC, !SocFlag);
	EnableMenuItem(hMenu, ID_MENUITEM_STOP, SocFlag | !SaveBoxFlag);
	SendMessage(hToolBar, TB_ENABLEBUTTON, ID_MENUITEM_RECV,
		(LPARAM)MAKELONG(SocFlag & SaveBoxFlag & SendBoxFlag, 0));
	SendMessage(hToolBar, TB_ENABLEBUTTON, ID_MENUITEM_ALLCHECK, (LPARAM)MAKELONG(SocFlag, 0));
	SendMessage(hToolBar, TB_ENABLEBUTTON, ID_MENUITEM_EXEC,
		(LPARAM)MAKELONG(SocFlag & SaveBoxFlag, 0));
	SendMessage(hToolBar, TB_ENABLEBUTTON, ID_MENUITEM_ALLEXEC, (LPARAM)MAKELONG(SocFlag, 0));
	SendMessage(hToolBar, TB_ENABLEBUTTON, ID_MENUITEM_STOP, (LPARAM)MAKELONG(!SocFlag, 0));

	EnableMenuItem(hMenu, ID_MENUITEM_DOWNMARK, !(SelFlag & SaveBoxFlag & !(!RecvBoxFlag && ExecFlag == TRUE)));
	EnableMenuItem(hMenu, ID_MENUITEM_DELMARK, !(SelFlag & SaveBoxFlag & SendBoxFlag & !(!RecvBoxFlag && ExecFlag == TRUE)));
	EnableMenuItem(hMenu, ID_MENUITEM_UNMARK, !(SelFlag & SaveBoxFlag & !(!RecvBoxFlag && ExecFlag == TRUE)));
	if (hViewWnd != NULL) {
		SendMessage(hViewWnd, WM_CHANGE_MARK, 0, 0);
	}

	EnableMenuItem(hMenu, ID_MENUITEM_READMAIL, !(SelFlag & SendBoxFlag));
	EnableMenuItem(hMenu, ID_MENUITEM_NOREADMAIL, !(SelFlag & SendBoxFlag));

	EnableMenuItem(hMenu, ID_MENUITE_SAVECOPY, !(SelFlag & SaveBoxFlag));
	EnableMenuItem(hMenu, ID_MENUITEM_DELETE, !SelFlag);
}

/*
 * FreeAllMailBox - ウィンドウの終了処理
 */
static void FreeAllMailBox(void)
{
	int i;

	// アドレス帳の解放
	mailbox_free(AddressBox);
	mem_free(&AddressBox);

	// すべてのメールボックスの解放
	for (i = 0; i < MailBoxCnt; i++) {
		mailbox_free((MailBox + i));
	}
	mem_free(&MailBox);
}

/*
 * CloseViewWindow - メール表示ウィンドウとメール編集ウィンドウを閉じる
 */
static void CloseViewWindow(int Flag)
{
	HWND fWnd;
	// メール表示ウィンドウを閉じる
	if (hViewWnd != NULL) {
		SendMessage(hViewWnd, WM_ENDCLOSE, 0, 0);
	}
	// メール編集ウィンドウを閉じる
	while ((fWnd = FindWindow(EDIT_WND_CLASS, NULL)) != NULL) {
		SendMessage(fWnd, WM_ENDCLOSE, Flag, 0);
	}
}

/*
 * SubClassListViewProc - サブクラス化したウィンドウプロシージャ
 *	IMEが入った状態でもSpaceKeyを有効にするため
 */
static LRESULT CALLBACK SubClassListViewProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int i;

	switch (msg) {
	case WM_FINDMAILBOX:
		SwitchCursor(FALSE);
		i = mailbox_next_unread(SelBox + 1, MailBoxCnt);
		if (i == -1) {
			i = mailbox_next_unread(MAILBOX_USER, SelBox);
		}
		if (i == -1) {
			SwitchCursor(TRUE);
			return 0;
		}
		// メールボックスの選択
		mailbox_select(GetParent(hWnd), i);
		SwitchCursor(TRUE);
		return 0;

	case WM_CHAR:
		if ((TCHAR)wParam == TEXT(' ') && GetKeyState(VK_CONTROL) >= 0 && GetKeyState(VK_SHIFT) >= 0) {
			if (ListView_GetItemCount(hWnd) == 0 && op.MoveAllMailBox == 1) {
				SendMessage(hWnd, WM_FINDMAILBOX, 0, 0);
				return 0;
			} else if (ListView_GetItemState(hWnd, ListView_GetNextItem(hWnd, -1, LVNI_FOCUSED), LVIS_SELECTED) == LVIS_SELECTED) {
				SendMessage(GetParent(hWnd), WM_COMMAND, ID_MAILITEM_OPEN, 0);
				return 0;
			}
		}
		break;

	case WM_IME_CHAR:
		// 2バイトのスペース
#ifdef UNICODE
		if (wParam == 0x3000 &&
#else
		if (wParam == 0x8140 &&
#endif
			GetKeyState(VK_CONTROL) >= 0 && GetKeyState(VK_SHIFT) >= 0) {
			if (ListView_GetItemCount(hWnd) == 0 && op.MoveAllMailBox == 1) {
				SendMessage(hWnd, WM_FINDMAILBOX, 0, 0);
				return 0;
			} else if (ListView_GetItemState(hWnd, ListView_GetNextItem(hWnd, -1, LVNI_FOCUSED), LVIS_SELECTED) == LVIS_SELECTED) {
				SendMessage(GetParent(hWnd), WM_COMMAND, ID_MAILITEM_OPEN, 0);
				return 0;
			}
		}
		break;
	}
	return CallWindowProc(ListViewWindowProcedure, hWnd, msg, wParam, lParam);
}

/*
 * SetListViewSubClass - ウィンドウのサブクラス化
 */
static void SetListViewSubClass(HWND hWnd)
{
	ListViewWindowProcedure = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (long)SubClassListViewProc);
}

/*
 * DelListViewSubClass - ウィンドウクラスを標準のものに戻す
 */
static void DelListViewSubClass(HWND hWnd)
{
	SetWindowLong(hWnd, GWL_WNDPROC, (long)ListViewWindowProcedure);
	ListViewWindowProcedure = NULL;
}

/*
 * ListViewHeaderNotifyProc - リストビューヘッダメッセージ
 */
static LRESULT ListViewHeaderNotifyProc(HWND hWnd, LPARAM lParam)
{
	HD_NOTIFY *phd = (HD_NOTIFY *)lParam;
	HWND hListView;

	switch (phd->hdr.code) {
	case HDN_ITEMCLICK:
		op.LvThreadView = 0;
		CheckMenuItem(GetMenu(hWnd), ID_MENUITEM_THREADVIEW, MF_UNCHECKED);
		// ソートの設定
		LvSortFlag = (ABS(LvSortFlag) == (phd->iItem + 1)) ? (LvSortFlag * -1) : (phd->iItem + 1);
		// ソート
		hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
		SwitchCursor(FALSE);
		ListView_SortItems(hListView, CompareFunc, LvSortFlag);
		SwitchCursor(TRUE);

		ListView_EnsureVisible(hListView,
			ListView_GetNextItem(hListView, -1, LVNI_FOCUSED), TRUE);

		if (SelBox == MAILBOX_SAVE || op.LvAutoSort == 2) {
			op.LvSortItem = LvSortFlag;
		}
		break;
	}
	return FALSE;
}

/*
 * TbNotifyProc - ツールバーの通知メッセージ (Win32)
 */
static LRESULT TbNotifyProc(HWND hWnd, LPARAM lParam)
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
static LRESULT NotifyProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	NMHDR *CForm = (NMHDR *)lParam;

	if (CForm->hwndFrom == GetDlgItem(hWnd, IDC_LISTVIEW)) {
		return ListView_NotifyProc(hWnd, lParam);
	}
	if (CForm->hwndFrom == GetWindow(GetDlgItem(hWnd, IDC_LISTVIEW), GW_CHILD)) {
		return ListViewHeaderNotifyProc(hWnd, lParam);
	}
	if (CForm->code == TTN_NEEDTEXT) {
		return TbNotifyProc(hWnd, lParam);
	}
	return FALSE;
}

/*
 * CreateComboBox - コンボボックスの作成
 */
static int CreateComboBox(HWND hWnd, int Top)
{
	HWND hCombo;
	RECT rcClient, comboRect;
	int i;

	GetClientRect(hWnd, &rcClient);

	hCombo = CreateWindow(TEXT("COMBOBOX"), TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
		0, Top, rcClient.right, (rcClient.bottom < 200) ? 200 : rcClient.bottom,
		hWnd, (HMENU)IDC_COMBO, hInst, NULL);
	if (hCombo == NULL) {
		return -1;
	}

	SendDlgItemMessage(hWnd, IDC_COMBO, WM_SETFONT,
		(WPARAM)((hListFont != NULL) ? hListFont : GetStockObject(DEFAULT_GUI_FONT)),
		MAKELPARAM(TRUE,0));
	SendDlgItemMessage(hWnd, IDC_COMBO, CB_SETEXTENDEDUI, TRUE, 0);

	for (i = 0; i < MailBoxCnt; i++) {
		SendDlgItemMessage(hWnd, IDC_COMBO, CB_ADDSTRING, 0,
			(LPARAM)(((MailBox + i)->Name == NULL || *(MailBox + i)->Name == TEXT('\0'))
			? STR_MAILBOX_NONAME : (MailBox + i)->Name));
	}
	GetWindowRect(GetDlgItem(hWnd, IDC_COMBO), &comboRect);
	return (comboRect.bottom - comboRect.top);
}

/*
 * InitWindow - ウィンドウの初期化
 */
static BOOL InitWindow(HWND hWnd)
{
	HWND hToolBar;
	RECT StatusRect;
	HDC hdc;
	HFONT hFont;
	int Height = 0;
	int i, j;
	int Width[2];
	TBBUTTON tbButton[] = {
		{0,	ID_MENUITEM_RECV,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{1,	ID_MENUITEM_ALLCHECK,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{2,	ID_MENUITEM_EXEC,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{3,	ID_MENUITEM_ALLEXEC,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{4,	ID_MENUITEM_STOP,			TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
		{0,	0,							TBSTATE_ENABLED,	TBSTYLE_SEP,	0, 0, 0, -1},
		{5,	ID_MENUITEM_NEWMAIL,		TBSTATE_ENABLED,	TBSTYLE_BUTTON,	0, 0, 0, -1},
	};
	RECT ToolbarRect;

	MailMenuPos = 3;

	if (GetAwareness() != PROCESS_DPI_UNAWARE && GetScale() >= 300) {
		hToolBar = CreateToolbarEx(hWnd, WS_CHILD | TBSTYLE_TOOLTIPS, IDC_TB, 6, hInst, IDB_TOOLBAR_48,
			tbButton, sizeof(tbButton) / sizeof(TBBUTTON), 0, 0, 48, 48, sizeof(TBBUTTON));
	}
	else if (GetAwareness() != PROCESS_DPI_UNAWARE && GetScale() >= 150) {
		hToolBar = CreateToolbarEx(hWnd, WS_CHILD | TBSTYLE_TOOLTIPS, IDC_TB, 6, hInst, IDB_TOOLBAR_32,
			tbButton, sizeof(tbButton) / sizeof(TBBUTTON), 0, 0, 32, 32, sizeof(TBBUTTON));
	}
	else {
		hToolBar = CreateToolbarEx(hWnd, WS_CHILD | TBSTYLE_TOOLTIPS, IDC_TB, 6, hInst, IDB_TOOLBAR,
			tbButton, sizeof(tbButton) / sizeof(TBBUTTON), 0, 0, TB_ICONSIZE, TB_ICONSIZE, sizeof(TBBUTTON));
	}
	SetWindowLong(hToolBar, GWL_STYLE,
		GetWindowLong(hToolBar, GWL_STYLE) | TBSTYLE_FLAT);
	SendMessage(hToolBar, TB_SETINDENT, 5, 0);
	ShowWindow(hToolBar,SW_SHOW);

	GetWindowRect(hToolBar, &ToolbarRect);
	Height = ToolbarRect.bottom - ToolbarRect.top;

	i = SBS_SIZEGRIP | SBT_NOBORDERS;

	CheckMenuItem(GetMenu(hWnd), ID_MENUITEM_THREADVIEW, (op.LvThreadView == 1) ? MF_CHECKED : MF_UNCHECKED);

	// ListViewフォント
	if (op.lv_font.name != NULL && *op.lv_font.name != TEXT('\0')) {
		hListFont = font_create(hWnd, &op.lv_font);
	}
	// Viewフォント
	if (op.view_font.name != NULL && *op.view_font.name != TEXT('\0')) {
		hViewFont = font_create(hWnd, &op.view_font);
	}
	if (hViewFont == NULL) {
		hViewFont = font_copy(GetStockObject(DEFAULT_GUI_FONT));
	}
	hdc = GetDC(hWnd);
	if (hViewFont != NULL) {
		hFont = SelectObject(hdc, hViewFont);
	}
	font_charset = font_get_charset(hdc);
	if (hViewFont != NULL) {
		SelectObject(hdc, hFont);
	}
	ReleaseDC(hWnd, hdc);

	// コンボボックス
	if ((j = CreateComboBox(hWnd, Height)) == -1) {
		return FALSE;
	}
	Height += j;

	// ステータスバー
	CreateWindowEx(0, STATUSCLASSNAME, TEXT(""),
		WS_VISIBLE | WS_CHILD | i,
		0, 0, 0, 0, hWnd, (HMENU)IDC_STATUS, hInst, NULL);
	GetWindowRect(GetDlgItem(hWnd, IDC_STATUS), &StatusRect);
	Width[0] = Scale(220);
	Width[1] = -1;
	SendDlgItemMessage(hWnd, IDC_STATUS, SB_SETPARTS,
		(WPARAM)(sizeof(Width) / sizeof(int)), (LPARAM)((LPINT)Width));

	// リストビュー
	if (CreateListView(hWnd, Height, StatusRect.bottom - StatusRect.top) == NULL) {
		return FALSE;
	}
	SetFocus(GetDlgItem(hWnd, IDC_LISTVIEW));
	if (hListFont != NULL) {
		// リストビューのフォントを設定
		SendMessage(GetDlgItem(hWnd, IDC_LISTVIEW), WM_SETFONT, (WPARAM)hListFont, MAKELPARAM(TRUE, 0));
	}
	// リストビューをサブクラス化する
	SetListViewSubClass(GetDlgItem(hWnd, IDC_LISTVIEW));
	return TRUE;
}

/*
 * SetWindowSize - ウィンドウのサイズ変更
 */
static BOOL SetWindowSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	RECT rcClient, StatusRect, ToolbarRect, comboRect;
	int Height = 0;

	if (hWnd == NULL || IsIconic(hWnd) != 0) {
		return FALSE;
	}

	SendDlgItemMessage(hWnd, IDC_TB, WM_SIZE, wParam, lParam);
	SendDlgItemMessage(hWnd, IDC_STATUS, WM_SIZE, wParam, lParam);

	GetClientRect(hWnd, &rcClient);
	GetWindowRect(GetDlgItem(hWnd, IDC_TB), &ToolbarRect);
	GetWindowRect(GetDlgItem(hWnd, IDC_STATUS), &StatusRect);
	GetWindowRect(GetDlgItem(hWnd, IDC_COMBO), &comboRect);

	Height = ToolbarRect.bottom - ToolbarRect.top;
	MoveWindow(GetDlgItem(hWnd, IDC_COMBO), 0, Height,
		rcClient.right, comboRect.bottom - comboRect.top, TRUE);

	Height += comboRect.bottom - comboRect.top;
	MoveWindow(GetDlgItem(hWnd, IDC_LISTVIEW), 0, Height,
		rcClient.right, rcClient.bottom - Height - (StatusRect.bottom - StatusRect.top), TRUE);

	UpdateWindow(GetDlgItem(hWnd, IDC_LISTVIEW));
	return TRUE;
}

/*
 * SaveWindow - ウィンドウの保存処理
 */
static BOOL SaveWindow(HWND hWnd)
{
	int i;
	BOOL ret;

	SwitchCursor(FALSE);

	// 通信中の場合は切断を行う
	if (g_soc != -1 && GetHostFlag == FALSE) {
#ifndef WSAASYNC
		KillTimer(hWnd, ID_RECV_TIMER);
#endif
		KillTimer(hWnd, ID_SMTP_TIMER);
		KillTimer(hWnd, ID_SMTP_ONE_TIMER);
		KillTimer(hWnd, ID_CHECK_TIMER);
		KillTimer(hWnd, ID_EXEC_TIMER);
		KillTimer(hWnd, ID_TIMEOUT_TIMER);
		gSockFlag = FALSE;
		ExecFlag = FALSE;
		NewMailCnt = -1;
		command_status = POP_QUIT;
		send_buf(g_soc, CMD_RSET"\r\n");
		send_buf(g_soc, CMD_QUIT"\r\n");
		socket_close(hWnd, g_soc);
		g_soc = -1;
	}

	// 表示、編集ウィンドウを閉じる
	CloseViewWindow(1);

	// アドレス帳、保存箱、送信箱を保存
	ret = !file_save_address_book(ADDRESS_FILE, AddressBox);
	ret |= !file_save_mailbox(SAVEBOX_FILE, MailBox + MAILBOX_SAVE, 2);
	ret |= !file_save_mailbox(SENDBOX_FILE, MailBox + MAILBOX_SEND, 2);
	if (ret == TRUE) {
		SwitchCursor(TRUE);
		if (MessageBox(hWnd, STR_ERR_SAVEEND,
			STR_TITLE_ERROR, MB_ICONERROR | MB_YESNO) == IDNO) {
			return FALSE;
		}
	}

	// アカウント毎の設定の保存
	for (i = 0; i < LV_COL_CNT; i++) {
		op.LvColSize[i] = ListView_GetColumnWidth(GetDlgItem(hWnd, IDC_LISTVIEW), i);
	}
	if (ini_save_setting(hWnd, TRUE) == FALSE) {
		SwitchCursor(TRUE);
		if (MessageBox(hWnd, STR_ERR_SAVEEND,
			STR_TITLE_ERROR, MB_ICONERROR | MB_YESNO) == IDNO) {
			return FALSE;
		}
	}
	SwitchCursor(TRUE);
	return TRUE;
}

/*
 * EndWindow - ウィンドウの終了処理
 */
static BOOL EndWindow(HWND hWnd)
{
	HIMAGELIST hImgList;
	TCHAR path[BUF_SIZE];

	SwitchCursor(FALSE);

	ListView_DeleteAllItems(GetDlgItem(hWnd, IDC_LISTVIEW));

	// メッセージウィンドウを閉じる
	if (MsgWnd != NULL) {
		SendMessage(MsgWnd, WM_ENDDIALOG, 0, 0);
	}

	// 外部アプリ用ファイルの削除
	str_join_t(path, DataDir, VIEW_FILE, TEXT("."), op.ViewFileSuffix, (TCHAR *)-1);
	DeleteFile(path);

	// 添付ファイルの削除
	if (op.AttachDelete != 0) {
		TCHAR file[BUF_SIZE];
		wsprintf(path, TEXT("%s%s\\"), DataDir, op.AttachPath);
		wsprintf(file, TEXT("%s*"), ATTACH_FILE);
		dir_delete(path, file);
		RemoveDirectory(path);
	}

	// 検索文字列の解放
	mem_free(&FindStr);
	FindStr = NULL;

	// すべてのメールボックスの解放
	FreeAllMailBox();
	mem_free(&g_Pass);

	// 設定の解放
	ini_free();

	// タスクトレイのアイコンの除去
	op.ShowTrayIcon = 0;
	TrayMessage(hWnd, NIM_DELETE, TRAY_ID, NULL, NULL);

	// アイコンの破棄
	DestroyIcon(TrayIcon_Main);
	DestroyIcon(TrayIcon_Check);
	DestroyIcon(TrayIcon_Mail);

	// リストビューのサブクラス化の解除
	DelListViewSubClass(GetDlgItem(hWnd, IDC_LISTVIEW));

	// イメージリストの破棄
	hImgList = ListView_SetImageList(GetDlgItem(hWnd, IDC_LISTVIEW), NULL, LVSIL_SMALL);
	ImageList_Destroy((void *)hImgList);
	hImgList = ListView_SetImageList(GetDlgItem(hWnd, IDC_LISTVIEW), NULL, LVSIL_STATE);
	ImageList_Destroy((void *)hImgList);

	// ウィンドウの破棄
	DestroyWindow(GetDlgItem(hWnd, IDC_TB));
	DestroyWindow(GetDlgItem(hWnd, IDC_COMBO));
	DestroyWindow(GetDlgItem(hWnd, IDC_LISTVIEW));
	DestroyWindow(GetDlgItem(hWnd, IDC_STATUS));

	// フォントの破棄
	if (hListFont != NULL) {
		DeleteObject(hListFont);
	}
	if (hViewFont != NULL) {
		DeleteObject(hViewFont);
	}
	SwitchCursor(TRUE);
	DestroyWindow(hWnd);
	return TRUE;
}

/*
 * SendMail - メールの送信開始
 */
static BOOL SendMail(HWND hWnd, MAILITEM *tpMailItem, int end_cmd)
{
	MAILBOX *tpMailBox;
	TCHAR *pass;
	TCHAR ErrStr[BUF_SIZE];
	int BoxIndex;

	BoxIndex = mailbox_name_to_index(tpMailItem->MailBox);
	if (BoxIndex == -1) {
		ErrorSocketEnd(hWnd, MAILBOX_SEND);
		SocketErrorMessage(hWnd, STR_ERR_SELECTMAILBOX, MAILBOX_SEND);
		return FALSE;
	}
	tpMailBox = MailBox + BoxIndex;

	g_soc = 0;

	// SMTP Authentication
	if (tpMailBox->SmtpAuth == 1) {
		// パスワードが設定されていない場合はパスワードの入力を促す
		if (g_Pass != NULL) {
			mem_free(&g_Pass);
			g_Pass = NULL;
		}
		if (tpMailBox->AuthUserPass == 1) {
			pass = tpMailBox->SmtpPass;
			if (pass == NULL || *pass == TEXT('\0')) {
				pass = tpMailBox->SmtpTmpPass;
			}
		} else {
			pass = tpMailBox->Pass;
			if (pass == NULL || *pass == TEXT('\0')) {
				pass = tpMailBox->TmpPass;
			}
		}
		if (pass == NULL || *pass == TEXT('\0')) {
			gPassSt = 1;
			if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_INPUTPASS), hWnd, InputPassProc,
				(LPARAM)((tpMailBox->Name == NULL || *tpMailBox->Name == TEXT('\0'))
				? STR_MAILBOX_NONAME : tpMailBox->Name)) == FALSE) {
				g_soc = -1;
				ErrorSocketEnd(hWnd, MAILBOX_SEND);
				SetMailMenu(hWnd);
				return FALSE;
			}
			if (gPassSt == 1) {
				// 一時パスワードの設定
				if (tpMailBox->AuthUserPass == 1) {
					tpMailBox->SmtpTmpPass = alloc_copy_t(g_Pass);
				} else {
					tpMailBox->TmpPass = alloc_copy_t(g_Pass);
				}
			}
		}
	}

	if (op.SocLog == 1) log_init(AppDir, LOG_FILE, TEXT("send"));

	SetTimer(hWnd, ID_TIMEOUT_TIMER, TIMEOUTTIME * op.TimeoutInterval, NULL);

	SwitchCursor(FALSE);
	command_proc = smtp_proc;
	command_status = SMTP_START;
	ExecFlag = TRUE;

	*ErrStr = TEXT('\0');
	g_soc = smtp_send_mail(hWnd, tpMailBox, tpMailItem, end_cmd, ErrStr);
	if (g_soc == -1) {
		ErrorSocketEnd(hWnd, MAILBOX_SEND);
		SocketErrorMessage(hWnd, ErrStr, MAILBOX_SEND);
		return FALSE;
	}
	RecvBox = MAILBOX_SEND;

#ifndef WSAASYNC
	SetTimer(hWnd, ID_RECV_TIMER, RECVTIME, NULL);
#endif
	SetMailMenu(hWnd);
	SetTrayIcon(hWnd, TrayIcon_Check, WINDOW_TITLE);
	SwitchCursor(TRUE);
	return TRUE;
}

/*
 * RecvMailList - メールの一覧の受信
 */
static BOOL RecvMailList(HWND hWnd, int BoxIndex, BOOL SmtpFlag)
{
	MAILBOX *tpMailBox;
	TCHAR ErrStr[BUF_SIZE];

	if (g_soc != -1 || BoxIndex == MAILBOX_SAVE || BoxIndex == MAILBOX_SEND) {
		return FALSE;
	}
	g_soc = 0;

	tpMailBox = MailBox + BoxIndex;

	// 新着フラグの初期化
	tpMailBox->NewMail = FALSE;

	// パスワードが設定されていない場合はパスワードの入力を促す
	if (g_Pass != NULL) {
		mem_free(&g_Pass);
		g_Pass = NULL;
	}
	if ((tpMailBox->Pass == NULL || *tpMailBox->Pass == TEXT('\0')) &&
		(tpMailBox->TmpPass == NULL || *tpMailBox->TmpPass == TEXT('\0'))) {
		gPassSt = 1;
		if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_INPUTPASS), hWnd, InputPassProc,
			(LPARAM)((tpMailBox->Name == NULL || *tpMailBox->Name == TEXT('\0'))
			? STR_MAILBOX_NONAME : tpMailBox->Name)) == FALSE) {
			g_soc = -1;
			return FALSE;
		}
		if (gPassSt == 1) {
			// 一時パスワードの設定
			tpMailBox->TmpPass = alloc_copy_t(g_Pass);
		}
	}

	if (op.SocLog == 1) log_init(AppDir, LOG_FILE, TEXT("recv"));

	RecvBox = BoxIndex;

	// ホスト名からIPアドレスを取得 (取得したIPは保存する)
	SwitchCursor(FALSE);
	if (tpMailBox->PopIP == 0 || op.IPCache == 0) {
		GetHostFlag = TRUE;
		tpMailBox->PopIP = get_host_by_name(hWnd, tpMailBox->Server, ErrStr);
		GetHostFlag = FALSE;
		if (tpMailBox->PopIP == 0) {
			ErrorSocketEnd(hWnd, BoxIndex);
			SocketErrorMessage(hWnd, ErrStr, BoxIndex);
			g_soc = -1;
			return FALSE;
		}
	}

	SetTimer(hWnd, ID_TIMEOUT_TIMER, TIMEOUTTIME * op.TimeoutInterval, NULL);

	// 接続開始
	command_proc = pop3_list_proc;
	command_status = POP_START;
	PopBeforeSmtpFlag = SmtpFlag;
	g_soc = connect_server(hWnd,
		tpMailBox->PopIP, (unsigned short)tpMailBox->Port,
		tpMailBox->Server,
		tpMailBox->PopSSL,
		tpMailBox->PopSSLInfo.Type,
		tpMailBox->PopSSLInfo.Verify,
		ErrStr);
	if (g_soc == -1) {
		ErrorSocketEnd(hWnd, BoxIndex);
		SocketErrorMessage(hWnd, ErrStr, BoxIndex);
		return FALSE;
	}
#ifndef WSAASYNC
	SetTimer(hWnd, ID_RECV_TIMER, RECVTIME, NULL);
#endif
	SetMailMenu(hWnd);
	SetTrayIcon(hWnd, TrayIcon_Check, WINDOW_TITLE);
	SwitchCursor(TRUE);
	return TRUE;
}

/*
 * MailMarkCheck - 削除メールがないかチェックする
 */
static BOOL MailMarkCheck(HWND hWnd, BOOL DelMsg, BOOL NoMsg)
{
	HWND hListView;
	int i;
	BOOL ret = FALSE;

	for (i = MAILBOX_USER; i < MailBoxCnt; i++) {
		if ((MailBox + i)->CyclicFlag == 1) {
			continue;
		}
		if (item_get_next_download_mark((MailBox + i), -1, NULL) == -1 &&
			item_get_next_delete_mark((MailBox + i), -1, NULL) == -1) {
			continue;
		}
		ret = TRUE;
		if (DelMsg == TRUE && item_get_next_delete_mark((MailBox + i), -1, NULL) != -1) {
			if (MessageBox(hWnd, STR_Q_DELSERVERMAIL,
				(MailBox + i)->Name, MB_ICONEXCLAMATION | MB_YESNO) == IDNO) {
				mailbox_select(hWnd, i);
				hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
				if ((i = ListView_GetNextDeleteItem(hListView, -1)) != -1) {
					ListView_SetItemState(hListView, -1, 0, LVIS_SELECTED);
					ListView_SetItemState(hListView, i,
						LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
					ListView_EnsureVisible(hListView, i, TRUE);
				}
				return FALSE;
			}
			break;
		}
	}
	if (item_get_next_send_mark((MailBox + MAILBOX_SEND), -1, NULL) != -1) {
		ret = TRUE;
	}
	if (NoMsg == TRUE && ret == FALSE) {
		MessageBox(hWnd,
			STR_MSG_NOMARK, STR_TITLE_ALLEXEC, MB_ICONEXCLAMATION | MB_OK);
	}
	return ret;
}

/*
 * ExecItem - マークしたものを実行
 */
static BOOL ExecItem(HWND hWnd, int BoxIndex)
{
	MAILBOX *tpMailBox;
	TCHAR ErrStr[BUF_SIZE];

	if (g_soc != -1) {
		return FALSE;
	}

	tpMailBox = MailBox + BoxIndex;

	// 送信箱の場合は送信を行う
	if (BoxIndex == MAILBOX_SEND) {
		if (item_get_next_send_mark(tpMailBox, -1, NULL) == -1) {
			return FALSE;
		}
		AllCheck = TRUE;
		gSockFlag = TRUE;
		KeyShowHeader = FALSE;
		SmtpWait = 0;
		CheckBox = MAILBOX_USER - 1;
		SetTimer(hWnd, ID_SMTP_TIMER, SMTPTIME, NULL);
		return TRUE;
	}

	if (item_get_next_download_mark(tpMailBox, -1, NULL) == -1 &&
		item_get_next_delete_mark(tpMailBox, -1, NULL) == -1) {
		return FALSE;
	}
	g_soc = 0;

	mem_free(&g_Pass);
	g_Pass = NULL;
	if ((tpMailBox->Pass == NULL || *tpMailBox->Pass == TEXT('\0')) &&
		(tpMailBox->TmpPass == NULL || *tpMailBox->TmpPass == TEXT('\0'))) {
		gPassSt = 1;
		if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_INPUTPASS), hWnd, InputPassProc,
			(LPARAM)((tpMailBox->Name == NULL || *tpMailBox->Name == TEXT('\0'))
			? STR_MAILBOX_NONAME : tpMailBox->Name)) == FALSE) {
			g_soc = -1;
			return FALSE;
		}
		if (gPassSt == 1) {
			// 一時パスワードの設定
			tpMailBox->TmpPass = alloc_copy_t(g_Pass);
		}
	}

	if (op.SocLog == 1) log_init(AppDir, LOG_FILE, TEXT("exec"));

	RecvBox = BoxIndex;

	// ホスト名からIPアドレスを取得 (取得したIPは保存する)
	SwitchCursor(FALSE);
	if (tpMailBox->PopIP == 0 || op.IPCache == 0) {
		GetHostFlag = TRUE;
		tpMailBox->PopIP = get_host_by_name(hWnd, tpMailBox->Server, ErrStr);
		GetHostFlag = FALSE;
		if (tpMailBox->PopIP == 0) {
			ErrorSocketEnd(hWnd, BoxIndex);
			SocketErrorMessage(hWnd, ErrStr, BoxIndex);
			g_soc = -1;
			return FALSE;
		}
	}

	SetTimer(hWnd, ID_TIMEOUT_TIMER, TIMEOUTTIME * op.TimeoutInterval, NULL);

	// 接続開始
	command_proc = pop3_exec_proc;
	command_status = POP_START;
	g_soc = connect_server(hWnd,
		tpMailBox->PopIP, (unsigned short)tpMailBox->Port,
		tpMailBox->Server,
		tpMailBox->PopSSL,
		tpMailBox->PopSSLInfo.Type,
		tpMailBox->PopSSLInfo.Verify,
		ErrStr);
	if (g_soc == -1) {
		ErrorSocketEnd(hWnd, BoxIndex);
		SocketErrorMessage(hWnd, ErrStr, BoxIndex);
		return FALSE;
	}
#ifndef WSAASYNC
	SetTimer(hWnd, ID_RECV_TIMER, RECVTIME, NULL);
#endif
	SetMailMenu(hWnd);
	SetTrayIcon(hWnd, TrayIcon_Check, WINDOW_TITLE);
	SwitchCursor(TRUE);
	return TRUE;
}

/*
 * OpenItem - アイテムを開く
 */
static void OpenItem(HWND hWnd, BOOL MsgFlag, BOOL NoAppFlag)
{
	MAILITEM *tpMailItem;
	HWND hListView;
	int i;

	hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
	if (ListView_GetSelectedCount(hListView) <= 0) {
		return;
	}
	i = ListView_GetNextItem(hListView, -1, LVNI_FOCUSED);
	if (i == -1) {
		return;
	}
	ListView_EnsureVisible(hListView, i, TRUE);
	tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
	if (tpMailItem == NULL) {
		return;
	}
	if (SelBox == MAILBOX_SEND) {
		Edit_InitInstance(hInst, hWnd, -1, tpMailItem, EDIT_OPEN, 0);
		return;
	}
	if (tpMailItem->Body == NULL && SelBox != MAILBOX_SAVE) {
		if (MsgFlag == TRUE) {
			MessageBox(hWnd, STR_MSG_NOBODY, STR_TITLE_OPEN, MB_ICONEXCLAMATION | MB_OK);
			return;
		}
		if (tpMailItem->Status == ICON_DOWN) {
			tpMailItem->Status = ICON_NON;
			ListView_SetItemState(hListView, i, LVIS_CUT, LVIS_CUT);
		} else {
			tpMailItem->Status = ICON_DOWN;
			ListView_SetItemState(hListView, i, 0, LVIS_CUT);
		}
		ListView_RedrawItems(hListView, i, i);
		UpdateWindow(hListView);
		return;
	}
	View_InitInstance(hInst,
		(LPVOID)ListView_GetlParam(hListView, i), NoAppFlag);
}

/*
 * ReMessageItem - 返信の作成
 */
static void ReMessageItem(HWND hWnd)
{
	HWND hListView;
	int i;

	hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
	i = ListView_GetNextItem(hListView, -1, LVNI_FOCUSED);
	if (i == -1) {
		return;
	}
	if (SelBox == MAILBOX_SEND) {
		DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SETSEND), hWnd, SetSendProc,
			(LPARAM)ListView_GetlParam(hListView, i));
		ListView_RedrawItems(hListView, i, i);
		UpdateWindow(hListView);
	} else {
		Edit_InitInstance(hInst, hWnd, SelBox,
			(MAILITEM*)ListView_GetlParam(hListView, i), EDIT_REPLY, 0);
	}
}

/*
 * ItemToSaveBox - 保存箱へアイテムのコピー
 */
static void ItemToSaveBox(HWND hWnd)
{
	MAILBOX *tpMailBox;
	MAILITEM *tpMailItem;
	MAILITEM *tpTmpMailItem;
	HWND hListView;
	TCHAR *buf;
	TCHAR msgbuf[BUF_SIZE];
	int i, j, SelPoint = -1;

	hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
	if ((i = ListView_GetSelectedCount(hListView)) <= 0) {
		return;
	}
	if (SelBox == MAILBOX_SEND) {
		tpMailBox = MailBox + MAILBOX_SEND;
	} else {
		if (op.SaveMsg == 1) {
			wsprintf(msgbuf, STR_Q_COPY, i);
			if (MessageBox(hWnd, msgbuf, STR_TITLE_COPY, MB_ICONQUESTION | MB_YESNO) == IDNO) {
				return;
			}
		}
		tpMailBox = MailBox + MAILBOX_SAVE;
	}

	SwitchCursor(FALSE);
	i = -1;
	while ((i = ListView_GetNextItem(hListView, i, LVNI_SELECTED)) != -1) {
		tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
		if (tpMailItem == NULL) {
			continue;
		}

		if (SelBox == MAILBOX_SEND) {
			// 送信箱にコピーを作成する
			if ((tpTmpMailItem = item_to_mailbox(tpMailBox, tpMailItem, NULL, TRUE)) == NULL) {
				SwitchCursor(TRUE);
				ErrorMessage(hWnd, STR_ERR_CREATECOPY);
				break;
			}
			j = ListView_InsertItemEx(hListView,
				(TCHAR *)LPSTR_TEXTCALLBACK, 0, I_IMAGECALLBACK, (long)tpTmpMailItem,
				ListView_GetItemCount(hListView));
			if (tpTmpMailItem->Multipart == TRUE) {
				ListView_SetItemState(hListView, j, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK)
				ListView_RedrawItems(hListView, j, j);
				UpdateWindow(hListView);
			}
			if (SelPoint == -1) {
				SelPoint = j;
			}
		} else {
			// 既に存在していないか調べる
			j = item_find_thread(tpMailBox, tpMailItem->MessageID, tpMailBox->MailItemCnt);
			if (j != -1) {
				// コピー元がダウンロード済みかコピー先が未ダウンロードのものの場合は確認を行わない
				if (tpMailItem->Download == TRUE
					|| (*(tpMailBox->tpMailItem + j))->Download == FALSE) {
					item_free((tpMailBox->tpMailItem + j), 1);
				} else {
					// 上書きの確認
					buf = (TCHAR *)mem_alloc(
						sizeof(TCHAR) * (lstrlen(tpMailItem->Subject) + lstrlen(STR_Q_COPY) + 1));
					if (buf != NULL) {
						wsprintf(buf, STR_Q_COPY, tpMailItem->Subject);
						SwitchCursor(TRUE);
						if (MessageBox(hWnd, buf, STR_TITLE_COPY, MB_ICONEXCLAMATION | MB_YESNO) == IDNO) {
							mem_free(&buf);
							continue;
						}
						mem_free(&buf);
					}
					item_free((tpMailBox->tpMailItem + j), 1);
				}
			}
			// メールアイテムのコピー
			if (item_to_mailbox(tpMailBox, tpMailItem, (MailBox + SelBox)->Name, FALSE) == NULL) {
				SwitchCursor(TRUE);
				ErrorMessage(hWnd, STR_ERR_SAVECOPY);
				break;
			}
		}
	}
	item_resize_mailbox(tpMailBox);
	SetItemCntStatusText(hWnd, NULL);
	if (SelPoint != -1) {
		// 追加されたアイテムを選択する
		ListView_SetItemState(hListView, -1, 0, LVIS_SELECTED);
		ListView_SetItemState(hListView, SelPoint,
			LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		ListView_EnsureVisible(hListView, SelPoint, TRUE);
		SendDlgItemMessage(hWnd, IDC_LISTVIEW, WM_VSCROLL, SB_LINEDOWN, 0);
	}
	SwitchCursor(TRUE);
}

/*
 * ListDeleteItem - 一覧からアイテムを削除
 */
static void ListDeleteItem(HWND hWnd)
{
	MAILITEM *tpMailItem;
	HWND hListView;
	TCHAR buf[BUF_SIZE];
	int i;

	hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
	if ((i = ListView_GetSelectedCount(hListView)) <= 0) {
		return;
	}
	wsprintf(buf, STR_Q_DELLISTMAIL, i, (SelBox >= MAILBOX_USER)
		? STR_Q_DELLISTMAIL_NOSERVER : TEXT(""));
	if (MessageBox(hWnd, buf, STR_TITLE_DELETE, MB_ICONEXCLAMATION | MB_YESNO) == IDNO) {
		return;
	}

	SwitchCursor(FALSE);
	ListView_SetRedraw(hListView, FALSE);
	while ((i = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) != -1) {
		tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
		if (tpMailItem != NULL) {
			tpMailItem->Status = -1;
		}
		ListView_DeleteItem(hListView, i);
	}
	// メモリはNULLに設定
	for (i = 0; i < (MailBox + SelBox)->MailItemCnt; i++) {
		if (*((MailBox + SelBox)->tpMailItem + i) == NULL ||
			(*((MailBox + SelBox)->tpMailItem + i))->Status != -1) {
			continue;
		}
		item_free(((MailBox + SelBox)->tpMailItem + i), 1);
	}
	item_resize_mailbox(MailBox + SelBox);

	ListView_SetRedraw(hListView, TRUE);
	SetItemCntStatusText(hWnd, NULL);
	SwitchCursor(TRUE);
}

/*
 * SetDownloadMark - アイテムに受信マークを付加
 */
static void SetDownloadMark(HWND hWnd, BOOL Flag)
{
#define DOWN_ICON	((SelBox == MAILBOX_SEND) ? ICON_SEND : ICON_DOWN)
	MAILITEM *tpMailItem;
	HWND hListView;
	int i;

	hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
	if (ListView_GetSelectedCount(hListView) <= 0) {
		return;
	}
	i = -1;
	while ((i = ListView_GetNextItem(hListView, i, LVNI_SELECTED)) != -1) {
		tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
		if (tpMailItem == NULL) {
			continue;
		}
		if (tpMailItem->Status == DOWN_ICON && Flag == TRUE) {
			tpMailItem->Status = tpMailItem->MailStatus;
			if (SelBox != MAILBOX_SEND && tpMailItem->Download == FALSE) {
				ListView_SetItemState(hListView, i, LVIS_CUT, LVIS_CUT);
			}
		} else {
			tpMailItem->Status = DOWN_ICON;
			ListView_SetItemState(hListView, i, 0, LVIS_CUT);
		}
		ListView_RedrawItems(hListView, i, i);
	}
	UpdateWindow(hListView);

	if (hViewWnd != NULL) {
		SendMessage(hViewWnd, WM_CHANGE_MARK, 0, 0);
	}
}

/*
 * SetDeleteMark - アイテムに削除マークを付加
 */
static void SetDeleteMark(HWND hWnd)
{
	MAILITEM *tpMailItem;
	HWND hListView;
	int i;

	hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
	if (ListView_GetSelectedCount(hListView) <= 0) {
		return;
	}
	i = -1;
	while ((i = ListView_GetNextItem(hListView, i, LVNI_SELECTED)) != -1) {
		tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
		if (tpMailItem == NULL) {
			continue;
		}
		tpMailItem->Status = ICON_DEL;
		ListView_SetItemState(hListView, i, 0, LVIS_CUT);
		ListView_RedrawItems(hListView, i, i);
	}
	UpdateWindow(hListView);

	if (hViewWnd != NULL) {
		SendMessage(hViewWnd, WM_CHANGE_MARK, 0, 0);
	}
}

/*
 * UnMark - アイテムのマークを解除
 */
static void UnMark(HWND hWnd)
{
	MAILITEM *tpMailItem;
	HWND hListView;
	int i;

	hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
	if (ListView_GetSelectedCount(hListView) <= 0) {
		return;
	}
	i = -1;
	while ((i = ListView_GetNextItem(hListView, i, LVNI_SELECTED)) != -1) {
		tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
		if (tpMailItem == NULL) {
			continue;
		}
		tpMailItem->Status = tpMailItem->MailStatus;
		if (SelBox != MAILBOX_SEND && tpMailItem->Download == FALSE) {
			ListView_SetItemState(hListView, i, LVIS_CUT, LVIS_CUT);
		}
		ListView_RedrawItems(hListView, i, i);
	}
	UpdateWindow(hListView);

	if (hViewWnd != NULL) {
		SendMessage(hViewWnd, WM_CHANGE_MARK, 0, 0);
	}
}

/*
 * SetReadMail - 選択メールを開封済み、未開封にする
 */
static void SetMailStats(HWND hWnd, int St)
{
	MAILITEM *tpMailItem;
	HWND hListView;
	int i;

	hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
	if (ListView_GetSelectedCount(hListView) <= 0) {
		return;
	}
	i = -1;
	while ((i = ListView_GetNextItem(hListView, i, LVNI_SELECTED)) != -1) {
		tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
		if (tpMailItem == NULL ||
			tpMailItem->MailStatus == ICON_NON || tpMailItem->MailStatus >= ICON_SENDMAIL) {
			continue;
		}
		tpMailItem->MailStatus = St;
		if (tpMailItem->Status != ICON_DOWN && tpMailItem->Status != ICON_DEL) {
			tpMailItem->Status = St;
		}
		ListView_RedrawItems(hListView, i, i);
	}
	UpdateWindow(hListView);
	SetItemCntStatusText(hWnd, NULL);
}

/*
 * EndSocketFunc - 通信終了時の処理
 */
static void EndSocketFunc(HWND hWnd)
{
	HWND hListView;
	int i;

	SetMailMenu(hWnd);

	if (NewMail_Flag == TRUE &&
		(IsWindowVisible(hWnd) == 0 ||
		IsIconic(hWnd) != 0 ||
		GetForegroundWindow() != hWnd)) {
		SetTrayIcon(hWnd, TrayIcon_Mail, WINDOW_TITLE);
	} else {
		NewMail_Flag = FALSE;
		SetTrayIcon(hWnd, TrayIcon_Main, WINDOW_TITLE);
	}

	if (EndThreadSortFlag == TRUE) {
		// ソート
		SwitchCursor(FALSE);
		hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
		if (op.LvThreadView == 1) {
			item_create_thread(MailBox + SelBox);
			ListView_SortItems(hListView, CompareFunc, SORT_THREAD + 1);
		} else if (op.LvAutoSort == 2) {
			ListView_SortItems(hListView, CompareFunc, op.LvSortItem);

			i = ListView_GetNextNoReadItem(hListView, -1, ListView_GetItemCount(hListView));
			if (i != -1) {
				// 未開封メールを選択する
				ListView_SetItemState(hListView, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
				ListView_SetItemState(hListView, i,
					LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			}

		}
		ListView_EnsureVisible(hListView,
			ListView_GetNextItem(hListView, -1, LVNI_FOCUSED), TRUE);
		SwitchCursor(TRUE);

		EndThreadSortFlag = FALSE;
	}

	if (ExecFlag == TRUE && op.ExecEndSound == 1) {
		if (op.ExecEndSoundFile == NULL || *op.ExecEndSoundFile == TEXT('\0') ||
			sndPlaySound(op.ExecEndSoundFile, SND_ASYNC | SND_NODEFAULT) == FALSE) {
			MessageBeep(MB_ICONASTERISK);
		}
	}
	ExecFlag = FALSE;
}

/*
 * CheckEndAutoExec - チェック後の自動実行
 */
static BOOL CheckEndAutoExec(HWND hWnd, int SocBox, int cnt, BOOL AllFlag)
{
	if (cnt == -1) {
		return FALSE;
	}
	if (g_soc != -1) {
		return FALSE;
	}

	if (AllFlag == TRUE) {
		// 巡回実行
		ShowError = TRUE;
		if (MailMarkCheck(hWnd, ((op.CheckEndExecNoDelMsg == 1) ? TRUE : FALSE), FALSE) == FALSE) {
			ShowError = FALSE;
			return FALSE;
		}
		ShowError = FALSE;

		AutoCheckFlag = FALSE;
		AllCheck = TRUE;
		ExecFlag = FALSE;		// FALSE にすることでチェックと実行のループを避ける
		KeyShowHeader = FALSE;
		CheckBox = MAILBOX_USER - 1;

		gSockFlag = TRUE;
		SetTimer(hWnd, ID_EXEC_TIMER, CHECKTIME, NULL);
	} else {
		// 実行
		if (item_get_next_download_mark((MailBox + SocBox), -1, NULL) == -1 &&
			item_get_next_delete_mark((MailBox + SocBox), -1, NULL) == -1 &&
			item_get_next_send_mark((MailBox + SocBox), -1, NULL) == -1) {
			return FALSE;
		}
		if (op.CheckEndExecNoDelMsg == 1 && item_get_next_delete_mark((MailBox + SocBox), -1, NULL) != -1) {
			ShowError = TRUE;
			if (MessageBox(hWnd, STR_Q_DELSERVERMAIL,
				STR_TITLE_EXEC, MB_ICONEXCLAMATION | MB_YESNO) == IDNO) {
				ShowError = FALSE;
				return FALSE;
			}
			ShowError = FALSE;
		}
		AutoCheckFlag = FALSE;
		AllCheck = FALSE;
		ExecFlag = FALSE;		// FALSE にすることでチェックと実行のループを避ける
		KeyShowHeader = FALSE;
		ExecItem(hWnd, SocBox);
	}
	return TRUE;
}

/*
 * Init_NewMailFlag - 新着メールフラグの初期化
 */
static void Init_NewMailFlag(void)
{
	int i;

	if (ShowMsgFlag == TRUE) {
		return;
	}

	for (i = MAILBOX_USER; i < MailBoxCnt; i++) {
		(MailBox + i)->NewMail = FALSE;
	}
}

/*
 * SetNoReadCntTitle - 未読メールボックスの数をタイトルバーに表示
 */
void SetNoReadCntTitle(HWND hWnd)
{
	TCHAR wbuf[BUF_SIZE];
	int i;
	int NoReadMailBox = 0;

	for(i = MAILBOX_USER; i < MailBoxCnt; i++){
		if((MailBox + i)->NoRead == TRUE){
			NoReadMailBox++;
		}
	}

	//未読アカウント数をタイトルバーに設定
	if(NoReadMailBox == 0){
		SetWindowText(hWnd, WINDOW_TITLE);
	}else{
		wsprintf(wbuf, STR_TITLE_NOREADMAILBOX, WINDOW_TITLE, NoReadMailBox);
		SetWindowText(hWnd, wbuf);
	}
}

/*
 * NewMail_Massage - 新着メールチェック結果のメッセージ
 */
static void NewMail_Massage(HWND hWnd, int cnt)
{
	TCHAR *p;
	int i, j;

	if (cnt == -1) {
		return;
	}
	if (cnt == 0) {
		if (ShowError == FALSE && op.ShowNoMailMessage == 1 && ShowMsgFlag == FALSE &&
			AutoCheckFlag == FALSE) {
			// メッセージボックスの表示
			MessageBox(hWnd, STR_MSG_NONEWMAIL, WINDOW_TITLE,
				MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND);
		}
		return;
	}

	if (IsWindowVisible(hWnd) == 0 ||
		IsIconic(hWnd) != 0 ||
		GetForegroundWindow() != hWnd) {
		SetTrayIcon(hWnd, TrayIcon_Mail, WINDOW_TITLE);
		NewMail_Flag = TRUE;
	}

	// コンボボックスに新着ありを示す "*" を付加する
	j = SendDlgItemMessage(hWnd, IDC_COMBO, CB_GETCURSEL, 0, 0);
	for (i = MAILBOX_USER; i < MailBoxCnt; i++) {
		if (SelBox == i || (MailBox + i)->NewMail == FALSE ||
			((op.ListGetLine > 0 || op.ShowHeader == 1 || op.ListDownload == 1) &&
			mailbox_unread_check(i, FALSE) == FALSE)) {
			continue;
		}
		(MailBox + i)->NoRead = TRUE;
		SendDlgItemMessage(hWnd, IDC_COMBO, CB_DELETESTRING, i, 0);
		if ((MailBox + i)->Name == NULL || *(MailBox + i)->Name == TEXT('\0')) {
			SendDlgItemMessage(hWnd, IDC_COMBO, CB_INSERTSTRING, i,
				(LPARAM)STR_MAILBOX_NONAME\
				TEXT(" *"));
		} else {
			p = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen((MailBox + i)->Name) + 3));
			if (p != NULL) {
				str_join_t(p, (MailBox + i)->Name, TEXT(" *"), (TCHAR *)-1);
				SendDlgItemMessage(hWnd, IDC_COMBO, CB_INSERTSTRING, i, (LPARAM)p);
				mem_free(&p);
			}
		}
	}
	SendDlgItemMessage(hWnd, IDC_COMBO, CB_SETCURSEL, j, 0);

	SetNoReadCntTitle(hWnd);

	// 新着のメールボックスのインデックスを取得
	for (i = MAILBOX_USER; i < MailBoxCnt; i++) {
		if ((MailBox + i)->NewMail == TRUE) {
			break;
		}
	}

	if (op.NewMailSound == 1) {
		if (op.NewMailSoundFile == NULL || *op.NewMailSoundFile == TEXT('\0') ||
			sndPlaySound(op.NewMailSoundFile, SND_ASYNC | SND_NODEFAULT) == FALSE) {
			MessageBeep(MB_ICONASTERISK);
		}
	}

	// メッセージボックスの表示
	// メッセージを表示しない設定か現在表示されているメールボックスの場合はメッセージボックスを出さない
	if (ShowError == TRUE || op.ShowNewMailMessgae == 0 || ShowMsgFlag == TRUE ||
		(AutoCheckFlag == FALSE && hWnd == GetForegroundWindow() && i == SelBox)) {
		return;
	}
	SendMessage(MsgWnd,	WM_SHOWDIALOG, 0, 0);
}

/*
 * WndProc - メインウィンドウプロシージャ
 */
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static BOOL save_flag = FALSE;
	int i;

	switch (msg) {
	case WM_CREATE:
		MainWnd = hWnd;
		SwitchCursor(FALSE);

		save_flag = TRUE;
		// メールボックスの初期化
		if (mailbox_init() == FALSE) {
			SwitchCursor(TRUE);
			ErrorMessage(NULL, STR_ERR_INIT);
			DestroyWindow(hWnd);
			break;
		}
		// INIファイルの読み込み
		if (ini_read_setting(hWnd) == FALSE) {
			SwitchCursor(TRUE);
			ErrorMessage(NULL, STR_ERR_INIT);
			DestroyWindow(hWnd);
			break;
		}
		if (mailbox_read() == FALSE) {
			SwitchCursor(TRUE);
			ErrorMessage(NULL, STR_ERR_INIT);
			DestroyWindow(hWnd);
			break;
		}
		SetWindowPos(hWnd, 0, op.MainRect.left, op.MainRect.top, op.MainRect.right, op.MainRect.bottom,
			SWP_NOZORDER | SWP_HIDEWINDOW);
		// ウィンドウ内のコントロールの作成
		if (InitWindow(hWnd) == FALSE) {
			SwitchCursor(TRUE);
			ErrorMessage(NULL, STR_ERR_INIT);
			DestroyWindow(hWnd);
			break;
		}
		save_flag = FALSE;
		mailbox_select(hWnd, MAILBOX_USER);

		if (op.SocLog == 1) log_clear(AppDir, LOG_FILE);
		SwitchCursor(TRUE);

		// タスクトレイの設定
		TrayIcon_Main = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_NOCHECK),
			IMAGE_ICON, SICONSIZE, SICONSIZE, 0);
		TrayIcon_Check = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_CHECK),
			IMAGE_ICON, SICONSIZE, SICONSIZE, 0);
		TrayIcon_Mail = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_MAIN),
			IMAGE_ICON, SICONSIZE, SICONSIZE, 0);
		if (op.ShowTrayIcon == 1 && TrayIcon_Main != NULL) {
			TrayMessage(hWnd, NIM_ADD, TRAY_ID, TrayIcon_Main, WINDOW_TITLE);
		}

		// 自動チェック用タイマーの起動
		if (op.AutoCheck == 1) {
			SetTimer(hWnd, ID_AUTOCHECK_TIMER, AUTOCHECKTIME, NULL);
		}

		// 初回起動時
		if (first_start == TRUE) {
			ShowWindow(hWnd, SW_SHOW);
			SetMailBoxOption(hWnd);
			ini_save_setting(hWnd, FALSE);
			break;
		}
		
		// 起動時チェックの開始
		if (op.StartCheck == 1) {
			SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_ALLCHECK, 0);
		}

		// コマンドラインのメールアドレスからメールの編集
		if (CmdLine != NULL) {
			SetTimer(hWnd, ID_NEWMAIL_TIMER, 1, NULL);
		}
		break;

	case WM_COPYDATA:
		if (op.ShowPass == 1 &&
			(IsWindowVisible(hWnd) == 0 || IsIconic(hWnd) != 0) &&
			op.Password != NULL && *op.Password != TEXT('\0') &&
			ConfirmPass(hWnd, op.Password) == FALSE) {
			break;
		}
		if (CommandLine(hWnd, ((PCOPYDATASTRUCT)lParam)->lpData) == TRUE) {
			Edit_MailToSet(hInst, hWnd, ((PCOPYDATASTRUCT)lParam)->lpData, -1);
		}
		break;

	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED) {
			confirm_flag = 1;
		} 
		if (wParam == SIZE_MINIMIZED && op.ShowTrayIcon == 1 && op.MinsizeHide == 1) {
			ShowWindow(hWnd, SW_HIDE);
			return 0;
		}
		if (op.ShowPass == 1 &&
			(wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) &&
			op.Password != NULL && *op.Password != TEXT('\0') && confirm_flag == 1) {
			ShowWindow(hWnd, SW_MINIMIZE);
			return 0;
		}
		SetWindowSize(hWnd, wParam, lParam);
		if (wParam != SIZE_MINIMIZED) {
			confirm_flag = 0;
		} 
		break;

	case WM_EXITSIZEMOVE:
		if (IsWindowVisible(hWnd) != 0 &&
			IsIconic(hWnd) == 0 && IsZoomed(hWnd) == 0) {
			GetWindowRect(hWnd, (LPRECT)&op.MainRect);
			op.MainRect.right -= op.MainRect.left;
			op.MainRect.bottom -= op.MainRect.top;
		}
		break;

	case WM_QUERYENDSESSION:
		if (SaveWindow(hWnd) == FALSE) {
			return FALSE;
		}
		save_flag = TRUE;
		return TRUE;

	case WM_ENDSESSION:
		EndWindow(hWnd);
		return 0;

	case WM_CLOSE:
		if (op.ShowTrayIcon == 1 && op.CloseHide == 1) {
			ShowWindow(hWnd, SW_HIDE);
			FocusWnd = hWnd;
			break;
		}
		if (SaveWindow(hWnd) == FALSE) {
			break;
		}
		save_flag = TRUE;
		EndWindow(hWnd);
		break;

	case WM_DESTROY:
		if (save_flag == FALSE) {
			SaveWindow(hWnd);
		}
		save_flag = TRUE;
		PostQuitMessage(0);
		break;

	case WM_NOTIFY:
		return NotifyProc(hWnd, wParam, lParam);

	case WM_SETFOCUS:
		SetFocus(GetDlgItem(hWnd, IDC_LISTVIEW));
	case WM_INITTRAYICON:
		if (g_soc == -1) {
			SetTrayIcon(hWnd, TrayIcon_Main, WINDOW_TITLE);
		}
		NewMail_Flag = FALSE;
		break;

	case WM_TIMER:
		switch (wParam) {
#ifndef WSAASYNC
		// 受信処理
		case ID_RECV_TIMER:
			if (g_soc == -1) {
				KillTimer(hWnd, wParam);
				if (AllCheck == FALSE) {
					if (op.CheckEndExec == 1 &&
						CheckEndAutoExec(hWnd, RecvBox, NewMailCnt, FALSE) == TRUE) {
						// チェック後実行
						break;
					}
					if (ExecFlag == TRUE && op.CheckAfterUpdate == 1 && RecvBox != MAILBOX_SEND) {
						// 実行後チェック
						ExecFlag = FALSE;
						RecvMailList(hWnd, RecvBox, FALSE);
						break;
					}
					RecvBox = -1;
					EndSocketFunc(hWnd);
					NewMail_Massage(hWnd, NewMailCnt);
				} else {
					RecvBox = -1;
					SetMailMenu(hWnd);
				}
				break;
			}
			switch (recv_select(hWnd, g_soc)) {
			// メモリエラー
			case SELECT_MEM_ERROR:
				ErrorSocketEnd(hWnd, RecvBox);
				SocketErrorMessage(hWnd, STR_ERR_MEMALLOC, RecvBox);
				break;

			// selectエラー
			case SELECT_SOC_ERROR:
				ErrorSocketEnd(hWnd, RecvBox);
				SocketErrorMessage(hWnd, STR_ERR_SOCK_SELECT, RecvBox);
				break;

			// 切断
			case SELECT_SOC_CLOSE:
				if (command_status != POP_QUIT) {
					ErrorSocketEnd(hWnd, RecvBox);
					SocketErrorMessage(hWnd, STR_ERR_SOCK_DISCONNECT, RecvBox);
				} else {
					socket_close(hWnd, g_soc);
					g_soc = -1;
					KillTimer(hWnd, ID_TIMEOUT_TIMER);
					SetItemCntStatusText(hWnd, NULL);
				}
				break;

			// 受信データ有り
			case SELECT_SOC_SUCCEED:
				SetTimer(hWnd, ID_TIMEOUT_TIMER, TIMEOUTTIME * op.TimeoutInterval, NULL);
				break;
			}
			break;
#endif
		// 1件送信
		case ID_SMTP_ONE_TIMER:
			if (g_soc != -1) {
				break;
			}
			if (SmtpWait > 0) {
				SmtpWait--;
				break;
			}
			KillTimer(hWnd, wParam);

			AutoCheckFlag = FALSE;
			AllCheck = FALSE;
			ExecFlag = TRUE;
			KeyShowHeader = FALSE;
			gSockFlag = FALSE;

			// メールの送信
			SendMail(hWnd, (MAILITEM *)wkSendMailItem, SMTP_SENDEND);
			wkSendMailItem = NULL;
			break;

		// 送信
		case ID_SMTP_TIMER:
			if (g_soc != -1) {
				break;
			}
			if (SmtpWait > 0) {
				SmtpWait--;
				break;
			}
			if (CheckBox >= MAILBOX_USER &&
				item_get_next_send_mark_mailbox((MailBox + MAILBOX_SEND), -1, CheckBox) != -1) {
				// メールの送信
				SendMail(hWnd, *((MailBox + MAILBOX_SEND)->tpMailItem +
					item_get_next_send_mark_mailbox((MailBox + MAILBOX_SEND), -1, CheckBox)), SMTP_NEXTSEND);
				break;
			}

			CheckBox++;
			if (CheckBox >= MailBoxCnt) {
				KillTimer(hWnd, wParam);
				gSockFlag = FALSE;
				EndSocketFunc(hWnd);
				NewMail_Massage(hWnd, NewMailCnt);
				break;
			}
			if ((MailBox + CheckBox)->PopBeforeSmtp != 0 &&
				item_get_next_send_mark_mailbox((MailBox + MAILBOX_SEND), -1, CheckBox) != -1) {
				// POP before SMTP
				if (op.PopBeforeSmtpIsLoginOnly == 0 && NewMailCnt == -1) NewMailCnt = 0;
				RecvMailList(hWnd, CheckBox, (op.PopBeforeSmtpIsLoginOnly == 1) ? TRUE : FALSE);
				SmtpWait = op.PopBeforeSmtpWait / SMTPTIME;
			}
			break;

		// 巡回チェック
		case ID_CHECK_TIMER:
			if (g_soc != -1) {
				break;
			}

			CheckBox++;
			if (CheckBox >= MailBoxCnt) {
				// すべてのメールボックスのチェック完了
				KillTimer(hWnd, wParam);
				gSockFlag = FALSE;
				if (op.CheckEndExec == 1 &&
					CheckEndAutoExec(hWnd, 0, NewMailCnt, TRUE) == TRUE) {
					// チェック後実行
					break;
				}
				EndSocketFunc(hWnd);
				NewMail_Massage(hWnd, NewMailCnt);
				AutoCheckFlag = FALSE;
				break;
			}
			// 巡回しない設定のメールボックス
			if ((MailBox + CheckBox)->CyclicFlag == 1) {
				break;
			}
			// メール受信開始
			RecvMailList(hWnd, CheckBox, FALSE);
			break;

		// 巡回実行
		case ID_EXEC_TIMER:
			if (g_soc != -1) {
				break;
			}
			if (SmtpWait > 0) {
				SmtpWait--;
				break;
			}
			if (CheckBox >= MAILBOX_USER && (MailBox + CheckBox)->CyclicFlag == 0 &&
				item_get_next_send_mark_mailbox((MailBox + MAILBOX_SEND), -1, CheckBox) != -1) {
				// 送信メールの実行 (POP before SMTP)
				SendMail(hWnd, *((MailBox + MAILBOX_SEND)->tpMailItem +
					item_get_next_send_mark_mailbox((MailBox + MAILBOX_SEND), -1, CheckBox)), SMTP_NEXTSEND);
				break;
			}
			if (CheckBox >= MAILBOX_USER && (MailBox + CheckBox)->CyclicFlag == 0 &&
				op.CheckAfterUpdate == 1 && ExecCheckFlag == FALSE) {
				// 実行後チェック
				RecvMailList(hWnd, CheckBox, FALSE);
				ExecCheckFlag = TRUE;
				break;
			}
			ExecCheckFlag = FALSE;

			CheckBox++;
			if (CheckBox >= MailBoxCnt) {
				KillTimer(hWnd, wParam);
				if (item_get_next_send_mark((MailBox + MAILBOX_SEND), -1, NULL) != -1) {
					// 送信
					ExecItem(hWnd, MAILBOX_SEND);
					break;
				}
				// 巡回実行終了
				gSockFlag = FALSE;
				EndSocketFunc(hWnd);
				NewMail_Massage(hWnd, NewMailCnt);
				break;
			}
			// 巡回しない設定のメールボックス
			if ((MailBox + CheckBox)->CyclicFlag == 1) {
				break;
			}
			// マークを実行
			i = ExecItem(hWnd, CheckBox);
			if ((MailBox + CheckBox)->PopBeforeSmtp != 0 &&
				item_get_next_send_mark_mailbox((MailBox + MAILBOX_SEND), -1, CheckBox) != -1) {
				if (i == FALSE) {
					// POP before SMTP
					if (op.PopBeforeSmtpIsLoginOnly == 0 && NewMailCnt == -1) NewMailCnt = 0;
					RecvMailList(hWnd, CheckBox, (op.PopBeforeSmtpIsLoginOnly == 1) ? TRUE : FALSE);
				}
				SmtpWait = op.PopBeforeSmtpWait / SMTPTIME;
			}
			break;

		// 自動チェック
		case ID_AUTOCHECK_TIMER:
			if (op.AutoCheck == 0) {
				KillTimer(hWnd, wParam);
				AutoCheckCnt = 0;
				break;
			}
			AutoCheckCnt++;
			if (AutoCheckCnt < op.AutoCheckTime) {
				break;
			}
			AutoCheckCnt = 0;

			if (g_soc != -1 || ShowError == TRUE) {
				break;
			}
			AutoCheckFlag = TRUE;
			AllCheck = TRUE;
			KeyShowHeader = FALSE;
			NewMailCnt = 0;
			CheckBox = MAILBOX_USER - 1;
			Init_NewMailFlag();

			gSockFlag = TRUE;
			ExecFlag = FALSE;
			SetTimer(hWnd, ID_CHECK_TIMER, CHECKTIME, NULL);
			break;

		// タイムアウト
		case ID_TIMEOUT_TIMER:
			if (g_soc == -1) {
				KillTimer(hWnd, wParam);
				break;
			}
			ErrorSocketEnd(hWnd, RecvBox);
			SocketErrorMessage(hWnd, STR_ERR_SOCK_TIMEOUT, RecvBox);
			break;

		// 起動時メッセージ作成
		case ID_NEWMAIL_TIMER:
			KillTimer(hWnd, wParam);
			if (CmdLine != NULL) {
				if (CommandLine(hWnd, CmdLine) == TRUE && Edit_MailToSet(hInst, hWnd, CmdLine, -1) == EDIT_INSIDEEDIT) {
				}
				mem_free(&CmdLine);
				break;
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {
		// 表示アカウントの切り替え
		case IDC_COMBO:
			if (HIWORD(wParam) == CBN_CLOSEUP) {
				if (SelBox == SendDlgItemMessage(hWnd, IDC_COMBO, CB_GETCURSEL, 0, 0)) {
					break;
				}
				SetFocus(GetDlgItem(hWnd, IDC_LISTVIEW));
				mailbox_select(hWnd, SendDlgItemMessage(hWnd, IDC_COMBO, CB_GETCURSEL, 0, 0));
				SwitchCursor(TRUE);
			}
			break;

		// 上のアカウントに移動
		case ID_KEY_ALTUP:
			if (SelBox == 0) {
				break;
			}
			mailbox_select(hWnd, SelBox - 1);
			break;

		// 下のアカウントに移動
		case ID_KEY_ALTDOWN:
			if (SelBox + 1 >= MailBoxCnt) {
				break;
			}
			mailbox_select(hWnd, SelBox + 1);
			break;

		// コンボボックスとリストビューのフォーカスを切り替える
		case ID_KEY_TAB:
			if (GetFocus() == GetDlgItem(hWnd, IDC_LISTVIEW)) {
				SetFocus(GetDlgItem(hWnd, IDC_COMBO));
			} else {
				SetFocus(GetDlgItem(hWnd, IDC_LISTVIEW));
				if (SelBox != SendDlgItemMessage(hWnd, IDC_COMBO, CB_GETCURSEL, 0, 0)) {
					mailbox_select(hWnd, SendDlgItemMessage(hWnd, IDC_COMBO, CB_GETCURSEL, 0, 0));
				}
			}
			break;

		// 選択アイテムの位置にポップアップメニューを表示
		case ID_KEY_SHOWMENU:
		case ID_KEY_ESC:
			if (IsWindowVisible(hWnd) == 0 ||
				IsIconic(hWnd) != 0 ||
				GetForegroundWindow() != hWnd) {
				break;
			}
			if (GetFocus() == GetDlgItem(hWnd, IDC_COMBO)) {
				if (SendDlgItemMessage(hWnd, IDC_COMBO, CB_GETDROPPEDSTATE, 0, 0) == FALSE) {
					SetFocus(GetDlgItem(hWnd, IDC_LISTVIEW));
				} else {
					SendDlgItemMessage(hWnd, IDC_COMBO, CB_SHOWDROPDOWN, FALSE, 0);
				}
				break;
			}
			SetFocus(GetDlgItem(hWnd, IDC_LISTVIEW));
			ShowMenu(hWnd, GetMenu(hWnd), MailMenuPos, 1, FALSE);
			break;

		// マウスの位置にポップアップメニューを表示
		case ID_MENU:
			ShowMenu(hWnd, GetMenu(hWnd), MailMenuPos, 0, FALSE);
			break;

		// ====== ファイル =========
		// メッセージの作成
		case ID_MENUITEM_NEWMAIL:
			if (op.ShowPass == 1 &&
				(IsWindowVisible(hWnd) == 0 || IsIconic(hWnd) != 0) &&
				op.Password != NULL && *op.Password != TEXT('\0') &&
				ConfirmPass(hWnd, op.Password) == FALSE) {
				break;
			}
			Edit_InitInstance(hInst, hWnd, -1, NULL, EDIT_NEW, 0);
			break;

		// アドレス帳
		case ID_MENUITEM_ADDRESS:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_ADDRESS),
				hWnd, AddressListProc, 0);
			break;

		// オプション
		case ID_MENUITEM_OPTION:
			SetOption(hWnd);

			SwitchCursor(FALSE);
			// 自動チェックタイマーの設定
			if (op.AutoCheck == 1) {
				SetTimer(hWnd, ID_AUTOCHECK_TIMER, AUTOCHECKTIME, NULL);
			} else {
				KillTimer(hWnd, ID_AUTOCHECK_TIMER);
				AutoCheckCnt = 0;
			}
			// タスクトレイのアイコンの設定
			if (op.ShowTrayIcon == 1) {
				SetTrayIcon(hWnd, TrayIcon_Main, WINDOW_TITLE);
			} else {
				TrayMessage(hWnd, NIM_DELETE, TRAY_ID, NULL, NULL);
			}
			if (op.AutoSave == 1) {
				ini_save_setting(hWnd, FALSE);
			}
			SwitchCursor(TRUE);
			break;

		// バージョン情報
		case ID_MENUITEM_ABOUT:
			MessageBox(hWnd,
				APP_NAME
#ifdef UNICODE
				TEXT(" (UNICODE)")
#endif
				TEXT("\nCopyright (C) 1996-2023 by Ohno Tomoaki. All rights reserved.\n\n")
				TEXT("WEB SITE: https://nakka.com/\nE-MAIL: nakka@nakka.com"),
				TEXT("About"), MB_OK | MB_ICONINFORMATION);
			break;

		// 終了
		case ID_MENUITE_QUIT:
			if (SaveWindow(hWnd) == FALSE) {
				break;
			}
			save_flag = TRUE;
			EndWindow(hWnd);
			break;

		// ====== アカウント =========
		// アカウントの追加
		case ID_MENUITEM_ADDMAILBOX:
			mailbox_select(hWnd, mailbox_create(hWnd, TRUE));
			if (SetMailBoxOption(hWnd) == FALSE) {
				mailbox_select(hWnd, mailbox_delete(hWnd, SelBox));
				break;
			}
			if (op.AutoSave == 1) {
				SwitchCursor(FALSE);
				ini_save_setting(hWnd, FALSE);
				SwitchCursor(TRUE);
			}
			break;

		// アカウントの設定
		case ID_MENUITEM_SETMAILBOX:
			if (SetMailBoxOption(hWnd) == FALSE) {
				break;
			}
			if (op.AutoSave == 1) {
				SwitchCursor(FALSE);
				ini_save_setting(hWnd, FALSE);
				SwitchCursor(TRUE);
			}
			break;

		// アカウントの削除
		case ID_MENUITEM_DELETEMAILBOX:
			if (SelBox == MAILBOX_SAVE || SelBox == MAILBOX_SEND || SelBox == RecvBox) {
				break;
			}
			if (MessageBox(hWnd, STR_Q_DELMAILBOX, STR_TITLE_DELETE, MB_ICONEXCLAMATION | MB_YESNO) == IDNO) {
				break;
			}
			if (g_soc != -1 && SelBox < RecvBox) {
				RecvBox--;
			}
			mailbox_select(hWnd, mailbox_delete(hWnd, SelBox));
			break;

		// アカウントを上に移動
		case ID_MENUITEM_MOVEUPMAILBOX:
			mailbox_move_up(hWnd);
			break;

		// アカウントを下に移動
		case ID_MENUITEM_MOVEDOWNMAILBOX:
			mailbox_move_down(hWnd);
			break;

		// アイコン順にソート
		case ID_MENUITEM_ICONSORT:
			op.LvThreadView = 0;
			CheckMenuItem(GetMenu(hWnd), ID_MENUITEM_THREADVIEW, MF_UNCHECKED);
			LvSortFlag = (ABS(LvSortFlag) == (SORT_IOCN + 1)) ? (LvSortFlag * -1) : (SORT_IOCN + 1);
			// ソート
			SwitchCursor(FALSE);
			ListView_SortItems(GetDlgItem(hWnd, IDC_LISTVIEW), CompareFunc, LvSortFlag);
			SwitchCursor(TRUE);

			ListView_EnsureVisible(GetDlgItem(hWnd, IDC_LISTVIEW),
				ListView_GetNextItem(GetDlgItem(hWnd, IDC_LISTVIEW), -1, LVNI_FOCUSED), TRUE);

			if (SelBox == MAILBOX_SAVE || op.LvAutoSort == 2) {
				op.LvSortItem = LvSortFlag;
			}
			break;

		// スレッド表示
		case ID_MENUITEM_THREADVIEW:
			SwitchCursor(FALSE);
			if (op.LvThreadView == 1) {
				op.LvThreadView = 0;
				ListView_SortItems(GetDlgItem(hWnd, IDC_LISTVIEW), CompareFunc, LvSortFlag);
			} else {
				op.LvThreadView = 1;
				item_create_thread(MailBox + SelBox);
				ListView_SortItems(GetDlgItem(hWnd, IDC_LISTVIEW), CompareFunc, SORT_THREAD + 1);
			}
			SwitchCursor(TRUE);
			CheckMenuItem(GetMenu(hWnd), ID_MENUITEM_THREADVIEW, (op.LvThreadView == 1) ? MF_CHECKED : MF_UNCHECKED);
			ListView_EnsureVisible(GetDlgItem(hWnd, IDC_LISTVIEW),
				ListView_GetNextItem(GetDlgItem(hWnd, IDC_LISTVIEW), -1, LVNI_FOCUSED), TRUE);
			break;

		// 新着取得位置の初期化
		case ID_MENUITEM_LISTINIT:
			if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_INITMAILBOX),
				hWnd, InitMailBoxProc, (LPARAM)(MailBox + SelBox)) == FALSE) {
				break;
			}
			mem_free(&(MailBox + SelBox)->LastMessageId);
			(MailBox + SelBox)->LastMessageId = NULL;
			break;

		// ====== 送受信 =========
		// 新着チェック
		case ID_MENUITEM_RECV:
			if (g_soc != -1) {
				break;
			}
			i = SelBox;
			AutoCheckFlag = FALSE;
			AllCheck = FALSE;
			ExecFlag = FALSE;
			KeyShowHeader = FALSE;
			NewMailCnt = 0;
			Init_NewMailFlag();

			// メール受信開始
			RecvMailList(hWnd, i, FALSE);
			break;

		// 巡回チェック
		case ID_MENUITEM_ALLCHECK:
			if (g_soc != -1 || ShowError == TRUE) {
				break;
			}
			AutoCheckFlag = FALSE;
			AllCheck = TRUE;
			ExecFlag = FALSE;
			KeyShowHeader = FALSE;
			NewMailCnt = 0;
			CheckBox = MAILBOX_USER - 1;
			Init_NewMailFlag();

			gSockFlag = TRUE;
			SetTimer(hWnd, ID_CHECK_TIMER, CHECKTIME, NULL);
			SendMessage(hWnd, WM_TIMER, ID_CHECK_TIMER, 0);
			break;

		// マークしたものを実行
		case ID_MENUITEM_EXEC:
			if (g_soc != -1) {
				break;
			}
			KeyShowHeader = (GetKeyState(VK_SHIFT) < 0) ? TRUE : FALSE;

			if (item_get_next_download_mark((MailBox + SelBox), -1, NULL) == -1 &&
				item_get_next_delete_mark((MailBox + SelBox), -1, NULL) == -1 &&
				item_get_next_send_mark((MailBox + SelBox), -1, NULL) == -1) {
				MessageBox(hWnd,
					STR_MSG_NOMARK, STR_TITLE_EXEC, MB_ICONEXCLAMATION | MB_OK);
				break;
			}
			if (item_get_next_delete_mark((MailBox + SelBox), -1, NULL) != -1) {
				if (MessageBox(hWnd, STR_Q_DELSERVERMAIL,
					STR_TITLE_EXEC, MB_ICONEXCLAMATION | MB_YESNO) == IDNO) {
					ShowError = FALSE;
					break;
				}
			}
			i = SelBox;
			AutoCheckFlag = FALSE;
			AllCheck = FALSE;
			ExecFlag = TRUE;
			NewMailCnt = -1;
			if (op.CheckAfterUpdate == 1) {
				NewMailCnt = 0;
				Init_NewMailFlag();
			}
			// マークを実行
			ExecItem(hWnd, i);
			break;

		// 巡回実行
		case ID_MENUITEM_ALLEXEC:
			if (g_soc != -1) {
				break;
			}
			KeyShowHeader = (GetKeyState(VK_SHIFT) < 0) ? TRUE : FALSE;

			if (MailMarkCheck(hWnd, TRUE, TRUE) == FALSE) {
				break;
			}

			AutoCheckFlag = FALSE;
			AllCheck = TRUE;
			ExecFlag = TRUE;
			CheckBox = MAILBOX_USER - 1;
			NewMailCnt = -1;
			if (op.CheckAfterUpdate == 1) {
				NewMailCnt = 0;
				Init_NewMailFlag();
			}

			gSockFlag = TRUE;
			SetTimer(hWnd, ID_EXEC_TIMER, CHECKTIME, NULL);
			SendMessage(hWnd, WM_TIMER, ID_EXEC_TIMER, 0);
			break;

		// 中止
		case ID_MENUITEM_STOP:
			KillTimer(hWnd, ID_SMTP_TIMER);
			KillTimer(hWnd, ID_SMTP_ONE_TIMER);
			KillTimer(hWnd, ID_CHECK_TIMER);
			KillTimer(hWnd, ID_EXEC_TIMER);
			KillTimer(hWnd, ID_TIMEOUT_TIMER);
			gSockFlag = FALSE;
			AllCheck = FALSE;
			AutoCheckFlag = FALSE;
			ExecFlag = FALSE;
			KeyShowHeader = FALSE;
			NewMailCnt = -1;
			if (g_soc == -1 || GetHostFlag == TRUE) {
				g_soc = -1;
				RecvBox = -1;
				EndSocketFunc(hWnd);
				break;
			}
			if (command_status == POP_QUIT || command_status == POP_START) {
				socket_close(hWnd, g_soc);
				g_soc = -1;
				RecvBox = -1;
				SetItemCntStatusText(hWnd, NULL);
				EndSocketFunc(hWnd);
				break;
			}
			command_status = POP_QUIT;
			SetSocStatusTextT(hWnd, TEXT(CMD_RSET));
			send_buf(g_soc, CMD_RSET"\r\n");
			SetSocStatusTextT(hWnd, TEXT(CMD_QUIT));
			send_buf(g_soc, CMD_QUIT"\r\n");
			break;

		// ====== メール =========
		// 開く
		case ID_KEY_ENTER:
			if (GetFocus() == GetDlgItem(hWnd, IDC_COMBO)) {
				SendDlgItemMessage(hWnd, IDC_COMBO, CB_SHOWDROPDOWN,
					!SendDlgItemMessage(hWnd, IDC_COMBO, CB_GETDROPPEDSTATE, 0, 0), 0);
				break;
			}
		case ID_MAILITEM_OPEN:
			OpenItem(hWnd, FALSE, FALSE);
			break;

		case ID_MENUITEM_OPEN:
			OpenItem(hWnd, TRUE, FALSE);
			break;

		// 返信
		case ID_MENUITEM_REMESSEGE:
			ReMessageItem(hWnd);
			break;

		// 受信用マークの切り替え
		case ID_KEY_CTRLENTER:
			if (SelBox == MAILBOX_SAVE || (SelBox == RecvBox && ExecFlag == TRUE)) {
				break;
			}
			SetDownloadMark(hWnd, TRUE);
			break;

		// 受信用にマーク
		case ID_MENUITEM_DOWNMARK:
			if (SelBox == MAILBOX_SAVE || (SelBox == RecvBox && ExecFlag == TRUE)) {
				break;
			}
			SetDownloadMark(hWnd, FALSE);
			break;

		// 削除用にマーク
		case ID_MENUITEM_DELMARK:
			if (SelBox == MAILBOX_SAVE || SelBox == MAILBOX_SEND || (SelBox == RecvBox && ExecFlag == TRUE)) {
				break;
			}
			SetDeleteMark(hWnd);
			break;

		// マーク解除
		case ID_MENUITEM_UNMARK:
			if (SelBox == RecvBox && ExecFlag == TRUE) {
				break;
			}
			UnMark(hWnd);
			break;

		// 開封済みにする
		case ID_MENUITEM_READMAIL:
			if (SelBox == MAILBOX_SEND) {
				break;
			}
			SetMailStats(hWnd, ICON_READ);
			break;

		// 未開封にする
		case ID_MENUITEM_NOREADMAIL:
			if (SelBox == MAILBOX_SEND) {
				break;
			}
			SetMailStats(hWnd, ICON_MAIL);
			break;

		// 保存箱へコピー
		case ID_MENUITE_SAVECOPY:
			ItemToSaveBox(hWnd);
			if (op.AutoSave == 1) {
				// 保存箱の保存
				file_save_mailbox(SAVEBOX_FILE, MailBox + MAILBOX_SAVE, 2);
			}
			break;

		// 一覧から削除
		case ID_MENUITEM_DELETE:
			ListDeleteItem(hWnd);
			break;

		// すべて選択
		case ID_MENUITEM_ALLSELECT:
			SetFocus(GetDlgItem(hWnd, IDC_LISTVIEW));
			ListView_SetItemState(GetDlgItem(hWnd, IDC_LISTVIEW), -1, LVIS_SELECTED, LVIS_SELECTED);
			break;

		// アカウントの切り替え
		case ID_MENUITEM_CHANGEMAILBOX:
			SetFocus(GetDlgItem(hWnd, IDC_COMBO));
			SendDlgItemMessage(hWnd, IDC_COMBO, CB_SHOWDROPDOWN, TRUE, 0);
			break;

		// 一覧画面の表示
		case ID_MENUITEM_RESTORE:
			if (op.ShowPass == 1 &&
				(IsWindowVisible(hWnd) == 0 || IsIconic(hWnd) != 0) &&
				op.Password != NULL && *op.Password != TEXT('\0') &&
				ConfirmPass(hWnd, op.Password) == FALSE) {
				break;
			}
			ShowWindow(hWnd, SW_SHOW);
			if (IsIconic(hWnd) != 0) {
				ShowWindow(hWnd, SW_RESTORE);
			}
			_SetForegroundWindow(hWnd);
			break;

		// ウィンドウの表示
		case ID_MENUITEM_SHOWLASTWINDOW:
			SendMessage(hWnd, WM_SHOWLASTWINDOW, 0, 0);
			break;

		// 検索
		case ID_MENUITEM_FIND:
			if (SelBox == MAILBOX_SEND) {
				break;
			}
			OpenItem(hWnd, TRUE, TRUE);
			if (hViewWnd != NULL) {
				View_FindMail(hViewWnd, TRUE);
			}
			break;

		// 次を検索
		case ID_MENUITEM_NEXTFIND:
			if (SelBox == MAILBOX_SEND) {
				break;
			}
			OpenItem(hWnd, TRUE, TRUE);
			if (hViewWnd != NULL) {
				View_FindMail(hViewWnd, FALSE);
			}
			break;
		}
		break;

	case WM_SYSCOMMAND:
		if (op.ShowPass == 1 &&
			(wParam == SC_RESTORE || wParam == SC_MAXIMIZE) &&
			(IsWindowVisible(hWnd) == 0 || IsIconic(hWnd) != 0) &&
			op.Password != NULL && *op.Password != TEXT('\0') &&
			ConfirmPass(hWnd, op.Password) == FALSE) {
			return 0;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);

	// タスクトレイメッセージ
	case WM_TRAY_NOTIFY:
		switch (lParam) {
		case WM_LBUTTONDOWN:
			if (op.TrayIconToggle == 1 && IsIconic(hWnd) == 0 && IsWindowVisible(hWnd) != 0) {
				ShowWindow(hWnd, SW_MINIMIZE);
			} else {
				SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_RESTORE, 0);
			}
			break;

		case WM_RBUTTONUP:
			SetMenuDefaultItem(GetSubMenu(hPOPUP, 0), ID_MENUITEM_RESTORE, 0);
			EnableMenuItem(GetSubMenu(hPOPUP, 0), ID_MENUITEM_ALLCHECK, !(g_soc == -1));
			EnableMenuItem(GetSubMenu(hPOPUP, 0), ID_MENUITEM_STOP, (g_soc == -1));
			SendMessage(hWnd, WM_NULL, 0, 0);
			ShowMenu(hWnd, hPOPUP, 0, 0, FALSE);
			break;
		}
		break;

#ifdef WSAASYNC
	case WM_SOCK_SELECT:
		if (g_soc != -1 && WSAGETSELECTERROR(lParam) != 0) {
			ErrorSocketEnd(hWnd, RecvBox);
			switch (WSAGETSELECTEVENT(lParam)) {
			case FD_CONNECT:
				SocketErrorMessage(hWnd, STR_ERR_SOCK_CONNECT, RecvBox);
				break;

			case FD_READ:
			case FD_WRITE:
				SocketErrorMessage(hWnd, STR_ERR_SOCK_SENDRECV, RecvBox);
				break;

			default:
				SocketErrorMessage(hWnd, STR_ERR_SOCK_DISCONNECT, RecvBox);
				break;
			}
			break;
		}
		/* 処理すべきソケットか判定 */
		if (g_soc != (int)wParam) {
			break;
		}
		/* ソケットイベント毎の処理を行う */
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_CONNECT:					/* サーバへの接続が完了した事を示すイベント */
			{
				TCHAR ErrStr[BUF_SIZE];
				*ErrStr = TEXT('\0');
				if (init_ssl(hWnd, g_soc, ErrStr) == -1) {
					ErrorSocketEnd(hWnd, RecvBox);
					SocketErrorMessage(hWnd, ErrStr, RecvBox);
				}
			}
			break;

		case FD_READ:						/* 受信バッファにデータがある事を示すイベント */
			/* データを受信して蓄積する */
			if (recv_proc(hWnd, g_soc) == SELECT_MEM_ERROR) {
				// メモリエラー
				ErrorSocketEnd(hWnd, RecvBox);
				SocketErrorMessage(hWnd, STR_ERR_MEMALLOC, RecvBox);
				break;
			}
			SetTimer(hWnd, ID_TIMEOUT_TIMER, TIMEOUTTIME * op.TimeoutInterval, NULL);
			break;

		case FD_WRITE:						/* 送信可能な事を示すイベント */
			{
				TCHAR ErrStr[BUF_SIZE];
				*ErrStr = TEXT('\0');
				if (smtp_send_proc(hWnd, g_soc, ErrStr, (MailBox + RecvBox)) == FALSE) {
					ErrorSocketEnd(hWnd, RecvBox);
					SocketErrorMessage(hWnd, ErrStr, RecvBox);
					break;
				}
				SetTimer(hWnd, ID_TIMEOUT_TIMER, TIMEOUTTIME * op.TimeoutInterval, NULL);
			}
			break;

		case FD_CLOSE:						/* サーバへの接続が終了した事を示すイベント */
			/* 接続を終了する */
			if (command_status != POP_QUIT) {
				ErrorSocketEnd(hWnd, RecvBox);
				SocketErrorMessage(hWnd, STR_ERR_SOCK_DISCONNECT, RecvBox);
			} else {
				socket_close(hWnd, g_soc);
				g_soc = -1;
				KillTimer(hWnd, ID_TIMEOUT_TIMER);
				SetItemCntStatusText(hWnd, NULL);
				if (AllCheck == FALSE) {
					if (op.CheckEndExec == 1 &&
						CheckEndAutoExec(hWnd, RecvBox, NewMailCnt, FALSE) == TRUE) {
						// チェック後実行
						break;
					}
					if (ExecFlag == TRUE && op.CheckAfterUpdate == 1 && RecvBox != MAILBOX_SEND) {
						// 実行後チェック
						ExecFlag = FALSE;
						RecvMailList(hWnd, RecvBox, FALSE);
						break;
					}
					RecvBox = -1;
					EndSocketFunc(hWnd);
					NewMail_Massage(hWnd, NewMailCnt);
				} else {
					RecvBox = -1;
					SetMailMenu(hWnd);
				}
			}
			break;
		}
		break;
#endif

	// ソケットの受信メッセージ
	case WM_SOCK_RECV:
		{
			TCHAR ErrStr[BUF_SIZE];

			*ErrStr = TEXT('\0');
			if (command_proc(hWnd, g_soc, (char *)lParam, wParam, ErrStr,
				(MailBox + RecvBox), RecvBox == SelBox) == FALSE) {
				ErrorSocketEnd(hWnd, RecvBox);
				if (*ErrStr != TEXT('\0')) {
					SocketErrorMessage(hWnd, ErrStr, RecvBox);
				} else {
					RecvBox = -1;
					SetMailMenu(hWnd);
				}
				return FALSE;
			}
		}
		return TRUE;

	// メール送信
	case WM_SMTP_SENDMAIL:
		if (g_soc != -1 || lParam == 0) {
			break;
		}
		wkSendMailItem = (MAILITEM *)lParam;
		NewMailCnt = -1;
		SmtpWait = 0;
		CheckBox = mailbox_name_to_index(wkSendMailItem->MailBox);
		if (CheckBox != -1 && (MailBox + CheckBox)->PopBeforeSmtp != 0) {
			// POP before SMTP
			AutoCheckFlag = FALSE;
			AllCheck = TRUE;
			gSockFlag = TRUE;
			if (op.PopBeforeSmtpIsLoginOnly == 0) NewMailCnt = 0;
			RecvMailList(hWnd, CheckBox, (op.PopBeforeSmtpIsLoginOnly == 1) ? TRUE : FALSE);
			SmtpWait = op.PopBeforeSmtpWait / SMTPTIME;
		}
		SetTimer(hWnd, ID_SMTP_ONE_TIMER, SMTPTIME, NULL);
		break;

	// リストビューのイベント
	case WM_LV_EVENT:
		switch (wParam) {
		case LVN_ITEMCHANGED:
			SetMailMenu(hWnd);
			break;

		case NM_CLICK:
			break;

		case NM_DBLCLK:
			SendMessage(hWnd, WM_COMMAND, ID_MAILITEM_OPEN, 0);
			break;

		case NM_RCLICK:
			SendMessage(hWnd, WM_COMMAND, ID_MENU, 0);
			break;
		}
		break;

	// ステータスバーに文字列を表示
	case WM_STATUSTEXT:
		SetStatusTextT(hWnd, (TCHAR *)lParam, 1);
		break;

	// ウィンドウの表示
	case WM_SHOWLASTWINDOW:
		SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_RESTORE, 0);
		if (op.StartCheck == 1) {
			SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_ALLCHECK, 0);
		}
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/*
 * InitApplication - ウィンドウクラスの登録
 */
static BOOL InitApplication(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = 0;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU_WINDOW);
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN));
	wc.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
	wc.lpszClassName = MAIN_WND_CLASS;

	return RegisterClass(&wc);
}

/*
 * InitInstance - ウィンドウの作成
 */
static HWND InitInstance(HINSTANCE hInstance, int CmdShow)
{
	HWND hwndMain =  NULL;

	hInst = hInstance;

	hwndMain = CreateWindow(MAIN_WND_CLASS,
		WINDOW_TITLE,
		WS_OVERLAPPEDWINDOW,
		0, 0,
		240,
		320,
		NULL, NULL, hInstance, NULL);
	if (!hwndMain) {
		return NULL;
	}

	if (op.ShowTrayIcon == 1 && op.StartHide == 1) {
		return hwndMain;
	}
	if ((CmdShow == SW_MINIMIZE || CmdShow == SW_SHOWMINIMIZED) && op.ShowTrayIcon == 1 && op.MinsizeHide == 1) {
		return hwndMain;
	}

	ShowWindow(hwndMain, CmdShow);
	UpdateWindow(hwndMain);
	return hwndMain;
}

/*
 * MessageFunc - メッセージ処理
 */
BOOL MessageFunc(HWND hWnd, MSG *msg)
{
	HWND fWnd;

	fWnd = GetForegroundWindow();
	if (fWnd == NULL) {

	// メインウィンドウのアクセラレータ
	} else if (fWnd == hWnd &&
		TranslateAccelerator(fWnd, hAccel, msg) == TRUE) {
		return TRUE;

	// メール表示ウィンドウのアクセラレータ
	} else if (fWnd == hViewWnd &&
		TranslateAccelerator(fWnd, hViewAccel, msg) == TRUE) {
		return TRUE;

	// メール到着通知ウィンドウ
	} else if (fWnd == MsgWnd &&
		IsDialogMessage(fWnd, msg) != 0) {
		return TRUE;

	// メール編集ウィンドウのアクセラレータ
	} else if (TranslateAccelerator(fWnd, hEditAccel, msg) == TRUE) {
		return TRUE;
	}
	TranslateMessage(msg);
	DispatchMessage(msg);
	return TRUE;
}

/*
 * WinMain - メイン
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int CmdShow)
{
	MSG msg;
	HWND hWnd;
	WSADATA WsaData;
	HANDLE hMutex = NULL;
	INITCOMMONCONTROLSEX iccex;
	HINSTANCE hLib;
	FARPROC _InitCommonControlsEx;

	hInst = hInstance;

#ifndef _DEBUG
	// ２重起動起動防止
	hMutex = CreateMutex(NULL, TRUE, STR_MUTEX);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		hWnd = FindWindow(MAIN_WND_CLASS, NULL);
		if (hWnd != NULL) {
			if (lpCmdLine != NULL && *lpCmdLine != TEXT('\0')) {
				COPYDATASTRUCT cpdata;

				CmdLine = alloc_char_to_tchar(lpCmdLine);
				cpdata.lpData = CmdLine;
				cpdata.cbData = sizeof(TCHAR) * (tstrlen(lpCmdLine) + 1);
				SendMessage(hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cpdata);
				mem_free(&CmdLine);
			} else {
				SendMessage(hWnd, WM_SHOWLASTWINDOW, 0, 0);
			}
		}
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		return 0;
	}
#endif	// _DEBUG

	// アプリケーションの作業パスの取得
	if (GetAppPath(hInstance) == FALSE) {
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		ErrorMessage(NULL, STR_ERR_MEMALLOC);
		return 0;
	}

	{
		int TmpCmdShow;
		// 起動パスワードのチェック
		TmpCmdShow = CmdShow;
		if (ini_start_auth_check() == FALSE) {
			mem_free(&AppDir);
			mem_free(&g_Pass);
			if (hMutex != NULL) {
				CloseHandle(hMutex);
			}
			return 0;
		}
		mem_free(&g_Pass);
		g_Pass = NULL;
		CmdShow = TmpCmdShow;
	}

	// 初期化
	if (WSAStartup(0x0101, &WsaData) != 0) {
		mem_free(&AppDir);
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		ErrorMessage(NULL, STR_ERR_INIT);
		return 0;
	}
	charset_init();
	InitCommonControls();
	// 新しいCommonControlの初期化を行う
	hLib = LoadLibrary(TEXT("comctl32.dll"));
	if (hLib != NULL) {
		_InitCommonControlsEx = GetProcAddress((HMODULE)hLib, "InitCommonControlsEx");
		if (_InitCommonControlsEx != NULL) {
			iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
			iccex.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
			_InitCommonControlsEx(&iccex);
		}
		FreeLibrary(hLib);
	}
	InitDpi();

	// ウィンドウクラス登録
	if (!InitApplication(hInstance) || !View_InitApplication(hInstance)
		|| !Edit_InitApplication(hInstance)) {
		mem_free(&AppDir);
		WSACleanup();
		charset_uninit();
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		ErrorMessage(NULL, STR_ERR_INIT);
		return 0;
	}

	if (lpCmdLine != NULL && *lpCmdLine != TEXT('\0')) {
		CmdLine = alloc_char_to_tchar(lpCmdLine);
	}

	CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_MSG), NULL, NewMailMessageProc, 0);

#ifdef USE_NEDIT
	// EDIT登録
	if (nedit_regist(hInstance) == FALSE) {
		return 0;
	}
#endif
	// メインウィンドウの作成
	if ((hWnd = InitInstance(hInstance, CmdShow)) == NULL) {
		FreeAllMailBox();
		mem_free(&CmdLine);
		mem_free(&AppDir);
		WSACleanup();
		charset_uninit();
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		ErrorMessage(NULL, STR_ERR_INIT);
		return 0;
	}

	// リソースからポップアップメニューをロード
	hPOPUP = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU_POPUP));

	// リソースからアクセラレータをロード
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
	hViewAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR_VIEW));
	hEditAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR_EDIT));

	// メッセージループ
	while (GetMessage(&msg, NULL, 0, 0) == TRUE) {
		MessageFunc(hWnd, &msg);
	}

	mem_free(&AppDir);
	DestroyMenu(hPOPUP);
	UnregisterClass(MAIN_WND_CLASS, hInstance);
	UnregisterClass(VIEW_WND_CLASS, hInstance);
	UnregisterClass(EDIT_WND_CLASS, hInstance);
	WSACleanup();
	charset_uninit();
	if (hMutex != NULL) {
		CloseHandle(hMutex);
	}
#ifdef _DEBUG
	mem_debug();
#endif
	return 0;
}

#ifndef UNICODE
#ifndef _DEBUG
void __cdecl WinMainCRTStartup(void)
{
    STARTUPINFO stinfo;
	char *cmdline;
	int ret;

	// コマンドラインの作成
	cmdline = GetCommandLine();
    if (*cmdline == '"') {
		for (cmdline++; *cmdline != '\0' && *cmdline != '"'; cmdline++);
		if (*cmdline != '\0') cmdline++;
	} else {
		for (; *cmdline != '\0' && *cmdline != ' '; cmdline++);
	}
	for (; *cmdline == ' '; cmdline++);

	// 起動時の情報の取得
	stinfo.cb = sizeof(STARTUPINFO);
	stinfo.dwFlags = STARTF_USESHOWWINDOW;
	GetStartupInfo(&stinfo);

	// WinMainの呼び出し
	ret = WinMain(GetModuleHandle(NULL), NULL, cmdline, stinfo.wShowWindow);
	// プロセスの終了
	ExitProcess(ret);
}
#endif
#endif
/* End of source */
