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
#define WM_TRAY_NOTIFY			(WM_APP + 100)		// �^�X�N�g���C
#define WM_FINDMAILBOX			(WM_APP + 101)

#define ID_MENU					(WM_APP + 102)		// �R���g���[��ID
#define IDC_CB					2000
#define IDC_TB					2001

#define ID_MAILITEM_OPEN		(WM_APP + 300)		// ���[��Open�pID

#define ID_RECV_TIMER			1					// �^�C�}�[ID
#define ID_SMTP_TIMER			2
#define ID_SMTP_ONE_TIMER		3
#define ID_CHECK_TIMER			4
#define ID_EXEC_TIMER			5
#define ID_AUTOCHECK_TIMER		6
#define ID_TIMEOUT_TIMER		7
#define ID_NEWMAIL_TIMER		8

#define RECVTIME				1					// �^�C�}�[�C���^�[�o��
#define SMTPTIME				100
#define CHECKTIME				100
#define AUTOCHECKTIME			60000
#define TIMEOUTTIME				60000

#define TRAY_ID					100					// �^�X�N�g���CID

#define CMD_RSET				"RSET"
#define CMD_QUIT				"QUIT"

/* Global Variables */
HINSTANCE hInst;							// Local copy of hInstance
TCHAR *AppDir;								// �A�v���P�[�V�����p�X
TCHAR *DataDir;								// �f�[�^�ۑ���̃p�X
TCHAR *g_Pass;								// �ꎞ�p�X���[�h
int gPassSt;								// �ꎞ�p�X���[�h�ۑ��t���O
static TCHAR *CmdLine;						// �R�}���h���C��
BOOL first_start;							// ����N���t���O
BOOL PPCFlag;								// PsPC�t���O
static int confirm_flag;					// �F�؃t���O

HWND MainWnd;								// ���C���E�B���h�E�̃n���h��
HWND FocusWnd;								// �t�H�[�J�X�����E�B���h�E�̃n���h��
HFONT hListFont;							// ListView�̃t�H���g
HFONT hViewFont;							// �\���̃t�H���g
int font_charset;
static HICON TrayIcon_Main;					// �^�X�N�g���C�A�C�R�� (�ҋ@)
static HICON TrayIcon_Check;				// �^�X�N�g���C�A�C�R�� (�`�F�b�N��)
static HICON TrayIcon_Mail;					// �^�X�N�g���C�A�C�R�� (�V������)
BOOL NewMail_Flag;							// �^�X�N�g���C�A�C�R���p�V���t���O
static HMENU hPOPUP;						// �^�X�N�g���C�A�C�R���p�̃|�b�v�A�b�v���j���[
static HANDLE hAccel, hViewAccel, hEditAccel;	// �A�N�Z�����[�^�̃n���h��
int MailMenuPos;							// ���j���[�ʒu

static WNDPROC ListViewWindowProcedure;		// �T�u�N���X�p�v���V�[�W��(ListView)
int LvSortFlag;								// ListView�̃\�[�g�t���O
BOOL EndThreadSortFlag;						// �ʐM�I�����̎����\�[�g�t���O(�X���b�h�\���p)

MAILBOX *MailBox;							// ���[���{�b�N�X
MAILBOX *AddressBox;						// �A�h���X��
int MailBoxCnt = 2;							// ���[���{�b�N�X��
int SelBox;									// �I�𒆂̃��[���{�b�N�X
int RecvBox;								// ����M���̃��[���{�b�N�X
static int CheckBox;						// �`�F�b�N���̃��[���{�b�N�X

SOCKET g_soc = -1;							// �\�P�b�g
BOOL gSockFlag;								// �ʐM���t���O
BOOL GetHostFlag;							// �z�X�g���������t���O
int NewMailCnt;								// �V�����[����
BOOL ShowMsgFlag;							// �V���L��̃��b�Z�[�W�\����
static BOOL ShowError;						// �G���[���b�Z�[�W�\����
BOOL AutoCheckFlag;							// �����`�F�b�N
BOOL PopBeforeSmtpFlag;						// POP before SMTP
BOOL KeyShowHeader;							// �L�[�ɂ��ꎞ�I�ȃw�b�_�\���t���O
static BOOL AllCheck;						// ����
BOOL ExecFlag;								// ���s��
static BOOL ExecCheckFlag;					// ���s��`�F�b�N�̃`�F�b�N����
static int AutoCheckCnt;					// �����`�F�b�N�J�n�܂ł̕����J�E���g
static int SmtpWait;						// POP before SMTP �ŔF�،�̑҂����� (�~���b)
static MAILITEM *wkSendMailItem;			// ���M�p���[���A�C�e��

typedef BOOL (*PPROC)(HWND, SOCKET, char*, int, TCHAR*, MAILBOX*, BOOL);
static PPROC command_proc;					// ����M�v���V�[�W��
int command_status;							// ����M�R�}���h�X�e�[�^�X (POP_, SMTP_)

// �O���Q��
extern OPTION op;

extern TCHAR *FindStr;						// ����������
extern HWND hViewWnd;						// �\���E�B���h�E
extern HWND MsgWnd;							// ���[���������b�Z�[�W�E�B���h�E

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
 * GetAppPath - ���[�U�f�B���N�g���̍쐬
 */

static BOOL GetAppPath(HINSTANCE hinst)
{
	TCHAR *p, *r;

	AppDir = (TCHAR *)mem_calloc(sizeof(TCHAR) * BUF_SIZE);
	if (AppDir == NULL) {
		return FALSE;
	}

	// �A�v���P�[�V�����̃p�X���擾
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
 * CommandLine - �R�}���h���C������������
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

	// ���[���{�b�N�X�̎擾
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
 * ConfirmPass - �p�X���[�h�̊m�F
 */
static BOOL ConfirmPass(HWND hWnd, TCHAR *ps)
{
	while (1) {
		// �N���p�X���[�h
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
 * TrayMessage - �^�X�N�g���C�̃A�C�R���̐ݒ�
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
 * SetTrayIcon - �^�X�N�g���C�ɃA�C�R����ݒ肷��
 */
static void SetTrayIcon(HWND hWnd, HICON hIcon, TCHAR *buf)
{
	if (op.ShowTrayIcon != 1 || hIcon == NULL) {
		return;
	}
	if (TrayMessage(hWnd, NIM_MODIFY, TRAY_ID, hIcon, buf) == FALSE) {
		// �ύX�ł��Ȃ������ꍇ�͒ǉ����s��
		TrayMessage(hWnd, NIM_ADD, TRAY_ID, hIcon, buf);
	}
}

/*
 * SwitchCursor - �J�[�\���̏�Ԃ̐؂�ւ�
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
 * _SetForegroundWindow - �E�B���h�E���A�N�e�B�u�ɂ���
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
 * SetStatusTextT - �X�e�[�^�X�̕\�� (TCHAR)
 */
void SetStatusTextT(HWND hWnd, TCHAR *buf, int Part)
{
	SendDlgItemMessage(hWnd, IDC_STATUS, SB_SETTEXT, (WPARAM)Part, (LPARAM)buf);
}

/*
 * SetSocStatusTextT - �X�e�[�^�X�̕\�� (TCHAR)
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
 * SetSocStatusText - �X�e�[�^�X�̕\��
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
 * SetItemCntStatusText - �A�C�e�����̕\��
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
 * SetStatusRecvLen - ����M�o�C�g���̕\��
 */
void SetStatusRecvLen(HWND hWnd, int len, TCHAR *msg)
{
	TCHAR wbuf[BUF_SIZE];

	wsprintf(wbuf, STR_STATUS_SOCKINFO, len, msg);
	SetStatusTextT(hWnd, wbuf, 1);
}

/*
 * ErrorMessage - �G���[���b�Z�[�W
 */
void ErrorMessage(HWND hWnd, TCHAR *buf)
{
	SwitchCursor(TRUE);
	ShowError = TRUE;
	MessageBox(hWnd, buf, STR_TITLE_ERROR, MB_OK | MB_ICONERROR);
	ShowError = FALSE;
}

/*
 * SocketErrorMessage - �\�P�b�g�̃G���[���b�Z�[�W
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
		// �X�e�[�^�X�o�[�ɃG���[�̏���\��
		SetStatusTextT(hWnd, buf, 1);
		if (op.SocLog == 1) log_save(AppDir, LOG_FILE, buf);
	}
	if (op.SocIgnoreError == 1 && BoxIndex >= MAILBOX_USER) {
		// ��M�G���[�𖳎�����ݒ�̏ꍇ
		return;
	}
	if (op.SendIgnoreError == 1 && BoxIndex == MAILBOX_SEND) {
		// ���M�G���[�𖳎�����ݒ�̏ꍇ
		return;
	}
	if ((BoxIndex == MAILBOX_SEND || BoxIndex >= MAILBOX_USER) &&
		tpMailBox->Name != NULL && *tpMailBox->Name != TEXT('\0')) {
		// �A�J�E���g���t���̃^�C�g���̍쐬
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
	// �G���[���b�Z�[�W�̕\��
	MessageBox(hWnd, buf, p, MB_OK | MB_ICONERROR);
	ShowError = FALSE;
	mem_free(&Title);
}

/*
 * ErrorSocketEnd - �ʐM�ُ̈�I�����̏���
 */
void ErrorSocketEnd(HWND hWnd, int BoxIndex)
{
	// �\�P�b�g�����
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
		// ��M�G���[�𖳎�����ݒ�̏ꍇ
		return;
	}
	if (op.SendIgnoreError == 1 && BoxIndex == MAILBOX_SEND) {
		// ���M�G���[�𖳎�����ݒ�̏ꍇ
		return;
	}
	// ����̒�~
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
 * ShowMenu - �}�E�X�̈ʒu�Ƀ��j���[��\������
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
		// �}�E�X�ʒu�̎擾
		GetCursorPos((LPPOINT)&apos);
		x = apos.x;
		y = apos.y;
		break;

	case 1:
		// �A�C�e���ʒu�̎擾
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
 * SetMailMenu - ���j���[�̊����^�񊈐��̐؂�ւ�
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
 * FreeAllMailBox - �E�B���h�E�̏I������
 */
static void FreeAllMailBox(void)
{
	int i;

	// �A�h���X���̉��
	mailbox_free(AddressBox);
	mem_free(&AddressBox);

	// ���ׂẴ��[���{�b�N�X�̉��
	for (i = 0; i < MailBoxCnt; i++) {
		mailbox_free((MailBox + i));
	}
	mem_free(&MailBox);
}

/*
 * CloseViewWindow - ���[���\���E�B���h�E�ƃ��[���ҏW�E�B���h�E�����
 */
static void CloseViewWindow(int Flag)
{
	HWND fWnd;
	// ���[���\���E�B���h�E�����
	if (hViewWnd != NULL) {
		SendMessage(hViewWnd, WM_ENDCLOSE, 0, 0);
	}
	// ���[���ҏW�E�B���h�E�����
	while ((fWnd = FindWindow(EDIT_WND_CLASS, NULL)) != NULL) {
		SendMessage(fWnd, WM_ENDCLOSE, Flag, 0);
	}
}

/*
 * SubClassListViewProc - �T�u�N���X�������E�B���h�E�v���V�[�W��
 *	IME����������Ԃł�SpaceKey��L���ɂ��邽��
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
		// ���[���{�b�N�X�̑I��
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
		// 2�o�C�g�̃X�y�[�X
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
 * SetListViewSubClass - �E�B���h�E�̃T�u�N���X��
 */
static void SetListViewSubClass(HWND hWnd)
{
	ListViewWindowProcedure = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (long)SubClassListViewProc);
}

/*
 * DelListViewSubClass - �E�B���h�E�N���X��W���̂��̂ɖ߂�
 */
static void DelListViewSubClass(HWND hWnd)
{
	SetWindowLong(hWnd, GWL_WNDPROC, (long)ListViewWindowProcedure);
	ListViewWindowProcedure = NULL;
}

/*
 * ListViewHeaderNotifyProc - ���X�g�r���[�w�b�_���b�Z�[�W
 */
static LRESULT ListViewHeaderNotifyProc(HWND hWnd, LPARAM lParam)
{
	HD_NOTIFY *phd = (HD_NOTIFY *)lParam;
	HWND hListView;

	switch (phd->hdr.code) {
	case HDN_ITEMCLICK:
		op.LvThreadView = 0;
		CheckMenuItem(GetMenu(hWnd), ID_MENUITEM_THREADVIEW, MF_UNCHECKED);
		// �\�[�g�̐ݒ�
		LvSortFlag = (ABS(LvSortFlag) == (phd->iItem + 1)) ? (LvSortFlag * -1) : (phd->iItem + 1);
		// �\�[�g
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
 * TbNotifyProc - �c�[���o�[�̒ʒm���b�Z�[�W (Win32)
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
 * NotifyProc - �R���g���[���̒ʒm���b�Z�[�W
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
 * CreateComboBox - �R���{�{�b�N�X�̍쐬
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
 * InitWindow - �E�B���h�E�̏�����
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

	// ListView�t�H���g
	if (op.lv_font.name != NULL && *op.lv_font.name != TEXT('\0')) {
		hListFont = font_create(hWnd, &op.lv_font);
	}
	// View�t�H���g
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

	// �R���{�{�b�N�X
	if ((j = CreateComboBox(hWnd, Height)) == -1) {
		return FALSE;
	}
	Height += j;

	// �X�e�[�^�X�o�[
	CreateWindowEx(0, STATUSCLASSNAME, TEXT(""),
		WS_VISIBLE | WS_CHILD | i,
		0, 0, 0, 0, hWnd, (HMENU)IDC_STATUS, hInst, NULL);
	GetWindowRect(GetDlgItem(hWnd, IDC_STATUS), &StatusRect);
	Width[0] = Scale(220);
	Width[1] = -1;
	SendDlgItemMessage(hWnd, IDC_STATUS, SB_SETPARTS,
		(WPARAM)(sizeof(Width) / sizeof(int)), (LPARAM)((LPINT)Width));

	// ���X�g�r���[
	if (CreateListView(hWnd, Height, StatusRect.bottom - StatusRect.top) == NULL) {
		return FALSE;
	}
	SetFocus(GetDlgItem(hWnd, IDC_LISTVIEW));
	if (hListFont != NULL) {
		// ���X�g�r���[�̃t�H���g��ݒ�
		SendMessage(GetDlgItem(hWnd, IDC_LISTVIEW), WM_SETFONT, (WPARAM)hListFont, MAKELPARAM(TRUE, 0));
	}
	// ���X�g�r���[���T�u�N���X������
	SetListViewSubClass(GetDlgItem(hWnd, IDC_LISTVIEW));
	return TRUE;
}

/*
 * SetWindowSize - �E�B���h�E�̃T�C�Y�ύX
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
 * SaveWindow - �E�B���h�E�̕ۑ�����
 */
static BOOL SaveWindow(HWND hWnd)
{
	int i;
	BOOL ret;

	SwitchCursor(FALSE);

	// �ʐM���̏ꍇ�͐ؒf���s��
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

	// �\���A�ҏW�E�B���h�E�����
	CloseViewWindow(1);

	// �A�h���X���A�ۑ����A���M����ۑ�
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

	// �A�J�E���g���̐ݒ�̕ۑ�
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
 * EndWindow - �E�B���h�E�̏I������
 */
static BOOL EndWindow(HWND hWnd)
{
	HIMAGELIST hImgList;
	TCHAR path[BUF_SIZE];

	SwitchCursor(FALSE);

	ListView_DeleteAllItems(GetDlgItem(hWnd, IDC_LISTVIEW));

	// ���b�Z�[�W�E�B���h�E�����
	if (MsgWnd != NULL) {
		SendMessage(MsgWnd, WM_ENDDIALOG, 0, 0);
	}

	// �O���A�v���p�t�@�C���̍폜
	str_join_t(path, DataDir, VIEW_FILE, TEXT("."), op.ViewFileSuffix, (TCHAR *)-1);
	DeleteFile(path);

	// �Y�t�t�@�C���̍폜
	if (op.AttachDelete != 0) {
		TCHAR file[BUF_SIZE];
		wsprintf(path, TEXT("%s%s\\"), DataDir, op.AttachPath);
		wsprintf(file, TEXT("%s*"), ATTACH_FILE);
		dir_delete(path, file);
		RemoveDirectory(path);
	}

	// ����������̉��
	mem_free(&FindStr);
	FindStr = NULL;

	// ���ׂẴ��[���{�b�N�X�̉��
	FreeAllMailBox();
	mem_free(&g_Pass);

	// �ݒ�̉��
	ini_free();

	// �^�X�N�g���C�̃A�C�R���̏���
	op.ShowTrayIcon = 0;
	TrayMessage(hWnd, NIM_DELETE, TRAY_ID, NULL, NULL);

	// �A�C�R���̔j��
	DestroyIcon(TrayIcon_Main);
	DestroyIcon(TrayIcon_Check);
	DestroyIcon(TrayIcon_Mail);

	// ���X�g�r���[�̃T�u�N���X���̉���
	DelListViewSubClass(GetDlgItem(hWnd, IDC_LISTVIEW));

	// �C���[�W���X�g�̔j��
	hImgList = ListView_SetImageList(GetDlgItem(hWnd, IDC_LISTVIEW), NULL, LVSIL_SMALL);
	ImageList_Destroy((void *)hImgList);
	hImgList = ListView_SetImageList(GetDlgItem(hWnd, IDC_LISTVIEW), NULL, LVSIL_STATE);
	ImageList_Destroy((void *)hImgList);

	// �E�B���h�E�̔j��
	DestroyWindow(GetDlgItem(hWnd, IDC_TB));
	DestroyWindow(GetDlgItem(hWnd, IDC_COMBO));
	DestroyWindow(GetDlgItem(hWnd, IDC_LISTVIEW));
	DestroyWindow(GetDlgItem(hWnd, IDC_STATUS));

	// �t�H���g�̔j��
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
 * SendMail - ���[���̑��M�J�n
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
		// �p�X���[�h���ݒ肳��Ă��Ȃ��ꍇ�̓p�X���[�h�̓��͂𑣂�
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
				// �ꎞ�p�X���[�h�̐ݒ�
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
 * RecvMailList - ���[���̈ꗗ�̎�M
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

	// �V���t���O�̏�����
	tpMailBox->NewMail = FALSE;

	// �p�X���[�h���ݒ肳��Ă��Ȃ��ꍇ�̓p�X���[�h�̓��͂𑣂�
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
			// �ꎞ�p�X���[�h�̐ݒ�
			tpMailBox->TmpPass = alloc_copy_t(g_Pass);
		}
	}

	if (op.SocLog == 1) log_init(AppDir, LOG_FILE, TEXT("recv"));

	RecvBox = BoxIndex;

	// �z�X�g������IP�A�h���X���擾 (�擾����IP�͕ۑ�����)
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

	// �ڑ��J�n
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
 * MailMarkCheck - �폜���[�����Ȃ����`�F�b�N����
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
 * ExecItem - �}�[�N�������̂����s
 */
static BOOL ExecItem(HWND hWnd, int BoxIndex)
{
	MAILBOX *tpMailBox;
	TCHAR ErrStr[BUF_SIZE];

	if (g_soc != -1) {
		return FALSE;
	}

	tpMailBox = MailBox + BoxIndex;

	// ���M���̏ꍇ�͑��M���s��
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
			// �ꎞ�p�X���[�h�̐ݒ�
			tpMailBox->TmpPass = alloc_copy_t(g_Pass);
		}
	}

	if (op.SocLog == 1) log_init(AppDir, LOG_FILE, TEXT("exec"));

	RecvBox = BoxIndex;

	// �z�X�g������IP�A�h���X���擾 (�擾����IP�͕ۑ�����)
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

	// �ڑ��J�n
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
 * OpenItem - �A�C�e�����J��
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
 * ReMessageItem - �ԐM�̍쐬
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
 * ItemToSaveBox - �ۑ����փA�C�e���̃R�s�[
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
			// ���M���ɃR�s�[���쐬����
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
			// ���ɑ��݂��Ă��Ȃ������ׂ�
			j = item_find_thread(tpMailBox, tpMailItem->MessageID, tpMailBox->MailItemCnt);
			if (j != -1) {
				// �R�s�[�����_�E�����[�h�ς݂��R�s�[�悪���_�E�����[�h�̂��̂̏ꍇ�͊m�F���s��Ȃ�
				if (tpMailItem->Download == TRUE
					|| (*(tpMailBox->tpMailItem + j))->Download == FALSE) {
					item_free((tpMailBox->tpMailItem + j), 1);
				} else {
					// �㏑���̊m�F
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
			// ���[���A�C�e���̃R�s�[
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
		// �ǉ����ꂽ�A�C�e����I������
		ListView_SetItemState(hListView, -1, 0, LVIS_SELECTED);
		ListView_SetItemState(hListView, SelPoint,
			LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		ListView_EnsureVisible(hListView, SelPoint, TRUE);
		SendDlgItemMessage(hWnd, IDC_LISTVIEW, WM_VSCROLL, SB_LINEDOWN, 0);
	}
	SwitchCursor(TRUE);
}

/*
 * ListDeleteItem - �ꗗ����A�C�e�����폜
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
	// ��������NULL�ɐݒ�
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
 * SetDownloadMark - �A�C�e���Ɏ�M�}�[�N��t��
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
 * SetDeleteMark - �A�C�e���ɍ폜�}�[�N��t��
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
 * UnMark - �A�C�e���̃}�[�N������
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
 * SetReadMail - �I�����[�����J���ς݁A���J���ɂ���
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
 * EndSocketFunc - �ʐM�I�����̏���
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
		// �\�[�g
		SwitchCursor(FALSE);
		hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
		if (op.LvThreadView == 1) {
			item_create_thread(MailBox + SelBox);
			ListView_SortItems(hListView, CompareFunc, SORT_THREAD + 1);
		} else if (op.LvAutoSort == 2) {
			ListView_SortItems(hListView, CompareFunc, op.LvSortItem);

			i = ListView_GetNextNoReadItem(hListView, -1, ListView_GetItemCount(hListView));
			if (i != -1) {
				// ���J�����[����I������
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
 * CheckEndAutoExec - �`�F�b�N��̎������s
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
		// ������s
		ShowError = TRUE;
		if (MailMarkCheck(hWnd, ((op.CheckEndExecNoDelMsg == 1) ? TRUE : FALSE), FALSE) == FALSE) {
			ShowError = FALSE;
			return FALSE;
		}
		ShowError = FALSE;

		AutoCheckFlag = FALSE;
		AllCheck = TRUE;
		ExecFlag = FALSE;		// FALSE �ɂ��邱�ƂŃ`�F�b�N�Ǝ��s�̃��[�v�������
		KeyShowHeader = FALSE;
		CheckBox = MAILBOX_USER - 1;

		gSockFlag = TRUE;
		SetTimer(hWnd, ID_EXEC_TIMER, CHECKTIME, NULL);
	} else {
		// ���s
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
		ExecFlag = FALSE;		// FALSE �ɂ��邱�ƂŃ`�F�b�N�Ǝ��s�̃��[�v�������
		KeyShowHeader = FALSE;
		ExecItem(hWnd, SocBox);
	}
	return TRUE;
}

/*
 * Init_NewMailFlag - �V�����[���t���O�̏�����
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
 * SetNoReadCntTitle - ���ǃ��[���{�b�N�X�̐����^�C�g���o�[�ɕ\��
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

	//���ǃA�J�E���g�����^�C�g���o�[�ɐݒ�
	if(NoReadMailBox == 0){
		SetWindowText(hWnd, WINDOW_TITLE);
	}else{
		wsprintf(wbuf, STR_TITLE_NOREADMAILBOX, WINDOW_TITLE, NoReadMailBox);
		SetWindowText(hWnd, wbuf);
	}
}

/*
 * NewMail_Massage - �V�����[���`�F�b�N���ʂ̃��b�Z�[�W
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
			// ���b�Z�[�W�{�b�N�X�̕\��
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

	// �R���{�{�b�N�X�ɐV����������� "*" ��t������
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

	// �V���̃��[���{�b�N�X�̃C���f�b�N�X���擾
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

	// ���b�Z�[�W�{�b�N�X�̕\��
	// ���b�Z�[�W��\�����Ȃ��ݒ肩���ݕ\������Ă��郁�[���{�b�N�X�̏ꍇ�̓��b�Z�[�W�{�b�N�X���o���Ȃ�
	if (ShowError == TRUE || op.ShowNewMailMessgae == 0 || ShowMsgFlag == TRUE ||
		(AutoCheckFlag == FALSE && hWnd == GetForegroundWindow() && i == SelBox)) {
		return;
	}
	SendMessage(MsgWnd,	WM_SHOWDIALOG, 0, 0);
}

/*
 * WndProc - ���C���E�B���h�E�v���V�[�W��
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
		// ���[���{�b�N�X�̏�����
		if (mailbox_init() == FALSE) {
			SwitchCursor(TRUE);
			ErrorMessage(NULL, STR_ERR_INIT);
			DestroyWindow(hWnd);
			break;
		}
		// INI�t�@�C���̓ǂݍ���
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
		// �E�B���h�E���̃R���g���[���̍쐬
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

		// �^�X�N�g���C�̐ݒ�
		TrayIcon_Main = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_NOCHECK),
			IMAGE_ICON, SICONSIZE, SICONSIZE, 0);
		TrayIcon_Check = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_CHECK),
			IMAGE_ICON, SICONSIZE, SICONSIZE, 0);
		TrayIcon_Mail = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_MAIN),
			IMAGE_ICON, SICONSIZE, SICONSIZE, 0);
		if (op.ShowTrayIcon == 1 && TrayIcon_Main != NULL) {
			TrayMessage(hWnd, NIM_ADD, TRAY_ID, TrayIcon_Main, WINDOW_TITLE);
		}

		// �����`�F�b�N�p�^�C�}�[�̋N��
		if (op.AutoCheck == 1) {
			SetTimer(hWnd, ID_AUTOCHECK_TIMER, AUTOCHECKTIME, NULL);
		}

		// ����N����
		if (first_start == TRUE) {
			ShowWindow(hWnd, SW_SHOW);
			SetMailBoxOption(hWnd);
			ini_save_setting(hWnd, FALSE);
			break;
		}
		
		// �N�����`�F�b�N�̊J�n
		if (op.StartCheck == 1) {
			SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_ALLCHECK, 0);
		}

		// �R�}���h���C���̃��[���A�h���X���烁�[���̕ҏW
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
		// ��M����
		case ID_RECV_TIMER:
			if (g_soc == -1) {
				KillTimer(hWnd, wParam);
				if (AllCheck == FALSE) {
					if (op.CheckEndExec == 1 &&
						CheckEndAutoExec(hWnd, RecvBox, NewMailCnt, FALSE) == TRUE) {
						// �`�F�b�N����s
						break;
					}
					if (ExecFlag == TRUE && op.CheckAfterUpdate == 1 && RecvBox != MAILBOX_SEND) {
						// ���s��`�F�b�N
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
			// �������G���[
			case SELECT_MEM_ERROR:
				ErrorSocketEnd(hWnd, RecvBox);
				SocketErrorMessage(hWnd, STR_ERR_MEMALLOC, RecvBox);
				break;

			// select�G���[
			case SELECT_SOC_ERROR:
				ErrorSocketEnd(hWnd, RecvBox);
				SocketErrorMessage(hWnd, STR_ERR_SOCK_SELECT, RecvBox);
				break;

			// �ؒf
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

			// ��M�f�[�^�L��
			case SELECT_SOC_SUCCEED:
				SetTimer(hWnd, ID_TIMEOUT_TIMER, TIMEOUTTIME * op.TimeoutInterval, NULL);
				break;
			}
			break;
#endif
		// 1�����M
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

			// ���[���̑��M
			SendMail(hWnd, (MAILITEM *)wkSendMailItem, SMTP_SENDEND);
			wkSendMailItem = NULL;
			break;

		// ���M
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
				// ���[���̑��M
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

		// ����`�F�b�N
		case ID_CHECK_TIMER:
			if (g_soc != -1) {
				break;
			}

			CheckBox++;
			if (CheckBox >= MailBoxCnt) {
				// ���ׂẴ��[���{�b�N�X�̃`�F�b�N����
				KillTimer(hWnd, wParam);
				gSockFlag = FALSE;
				if (op.CheckEndExec == 1 &&
					CheckEndAutoExec(hWnd, 0, NewMailCnt, TRUE) == TRUE) {
					// �`�F�b�N����s
					break;
				}
				EndSocketFunc(hWnd);
				NewMail_Massage(hWnd, NewMailCnt);
				AutoCheckFlag = FALSE;
				break;
			}
			// ���񂵂Ȃ��ݒ�̃��[���{�b�N�X
			if ((MailBox + CheckBox)->CyclicFlag == 1) {
				break;
			}
			// ���[����M�J�n
			RecvMailList(hWnd, CheckBox, FALSE);
			break;

		// ������s
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
				// ���M���[���̎��s (POP before SMTP)
				SendMail(hWnd, *((MailBox + MAILBOX_SEND)->tpMailItem +
					item_get_next_send_mark_mailbox((MailBox + MAILBOX_SEND), -1, CheckBox)), SMTP_NEXTSEND);
				break;
			}
			if (CheckBox >= MAILBOX_USER && (MailBox + CheckBox)->CyclicFlag == 0 &&
				op.CheckAfterUpdate == 1 && ExecCheckFlag == FALSE) {
				// ���s��`�F�b�N
				RecvMailList(hWnd, CheckBox, FALSE);
				ExecCheckFlag = TRUE;
				break;
			}
			ExecCheckFlag = FALSE;

			CheckBox++;
			if (CheckBox >= MailBoxCnt) {
				KillTimer(hWnd, wParam);
				if (item_get_next_send_mark((MailBox + MAILBOX_SEND), -1, NULL) != -1) {
					// ���M
					ExecItem(hWnd, MAILBOX_SEND);
					break;
				}
				// ������s�I��
				gSockFlag = FALSE;
				EndSocketFunc(hWnd);
				NewMail_Massage(hWnd, NewMailCnt);
				break;
			}
			// ���񂵂Ȃ��ݒ�̃��[���{�b�N�X
			if ((MailBox + CheckBox)->CyclicFlag == 1) {
				break;
			}
			// �}�[�N�����s
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

		// �����`�F�b�N
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

		// �^�C���A�E�g
		case ID_TIMEOUT_TIMER:
			if (g_soc == -1) {
				KillTimer(hWnd, wParam);
				break;
			}
			ErrorSocketEnd(hWnd, RecvBox);
			SocketErrorMessage(hWnd, STR_ERR_SOCK_TIMEOUT, RecvBox);
			break;

		// �N�������b�Z�[�W�쐬
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
		// �\���A�J�E���g�̐؂�ւ�
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

		// ��̃A�J�E���g�Ɉړ�
		case ID_KEY_ALTUP:
			if (SelBox == 0) {
				break;
			}
			mailbox_select(hWnd, SelBox - 1);
			break;

		// ���̃A�J�E���g�Ɉړ�
		case ID_KEY_ALTDOWN:
			if (SelBox + 1 >= MailBoxCnt) {
				break;
			}
			mailbox_select(hWnd, SelBox + 1);
			break;

		// �R���{�{�b�N�X�ƃ��X�g�r���[�̃t�H�[�J�X��؂�ւ���
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

		// �I���A�C�e���̈ʒu�Ƀ|�b�v�A�b�v���j���[��\��
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

		// �}�E�X�̈ʒu�Ƀ|�b�v�A�b�v���j���[��\��
		case ID_MENU:
			ShowMenu(hWnd, GetMenu(hWnd), MailMenuPos, 0, FALSE);
			break;

		// ====== �t�@�C�� =========
		// ���b�Z�[�W�̍쐬
		case ID_MENUITEM_NEWMAIL:
			if (op.ShowPass == 1 &&
				(IsWindowVisible(hWnd) == 0 || IsIconic(hWnd) != 0) &&
				op.Password != NULL && *op.Password != TEXT('\0') &&
				ConfirmPass(hWnd, op.Password) == FALSE) {
				break;
			}
			Edit_InitInstance(hInst, hWnd, -1, NULL, EDIT_NEW, 0);
			break;

		// �A�h���X��
		case ID_MENUITEM_ADDRESS:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_ADDRESS),
				hWnd, AddressListProc, 0);
			break;

		// �I�v�V����
		case ID_MENUITEM_OPTION:
			SetOption(hWnd);

			SwitchCursor(FALSE);
			// �����`�F�b�N�^�C�}�[�̐ݒ�
			if (op.AutoCheck == 1) {
				SetTimer(hWnd, ID_AUTOCHECK_TIMER, AUTOCHECKTIME, NULL);
			} else {
				KillTimer(hWnd, ID_AUTOCHECK_TIMER);
				AutoCheckCnt = 0;
			}
			// �^�X�N�g���C�̃A�C�R���̐ݒ�
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

		// �o�[�W�������
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

		// �I��
		case ID_MENUITE_QUIT:
			if (SaveWindow(hWnd) == FALSE) {
				break;
			}
			save_flag = TRUE;
			EndWindow(hWnd);
			break;

		// ====== �A�J�E���g =========
		// �A�J�E���g�̒ǉ�
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

		// �A�J�E���g�̐ݒ�
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

		// �A�J�E���g�̍폜
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

		// �A�J�E���g����Ɉړ�
		case ID_MENUITEM_MOVEUPMAILBOX:
			mailbox_move_up(hWnd);
			break;

		// �A�J�E���g�����Ɉړ�
		case ID_MENUITEM_MOVEDOWNMAILBOX:
			mailbox_move_down(hWnd);
			break;

		// �A�C�R�����Ƀ\�[�g
		case ID_MENUITEM_ICONSORT:
			op.LvThreadView = 0;
			CheckMenuItem(GetMenu(hWnd), ID_MENUITEM_THREADVIEW, MF_UNCHECKED);
			LvSortFlag = (ABS(LvSortFlag) == (SORT_IOCN + 1)) ? (LvSortFlag * -1) : (SORT_IOCN + 1);
			// �\�[�g
			SwitchCursor(FALSE);
			ListView_SortItems(GetDlgItem(hWnd, IDC_LISTVIEW), CompareFunc, LvSortFlag);
			SwitchCursor(TRUE);

			ListView_EnsureVisible(GetDlgItem(hWnd, IDC_LISTVIEW),
				ListView_GetNextItem(GetDlgItem(hWnd, IDC_LISTVIEW), -1, LVNI_FOCUSED), TRUE);

			if (SelBox == MAILBOX_SAVE || op.LvAutoSort == 2) {
				op.LvSortItem = LvSortFlag;
			}
			break;

		// �X���b�h�\��
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

		// �V���擾�ʒu�̏�����
		case ID_MENUITEM_LISTINIT:
			if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_INITMAILBOX),
				hWnd, InitMailBoxProc, (LPARAM)(MailBox + SelBox)) == FALSE) {
				break;
			}
			mem_free(&(MailBox + SelBox)->LastMessageId);
			(MailBox + SelBox)->LastMessageId = NULL;
			break;

		// ====== ����M =========
		// �V���`�F�b�N
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

			// ���[����M�J�n
			RecvMailList(hWnd, i, FALSE);
			break;

		// ����`�F�b�N
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

		// �}�[�N�������̂����s
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
			// �}�[�N�����s
			ExecItem(hWnd, i);
			break;

		// ������s
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

		// ���~
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

		// ====== ���[�� =========
		// �J��
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

		// �ԐM
		case ID_MENUITEM_REMESSEGE:
			ReMessageItem(hWnd);
			break;

		// ��M�p�}�[�N�̐؂�ւ�
		case ID_KEY_CTRLENTER:
			if (SelBox == MAILBOX_SAVE || (SelBox == RecvBox && ExecFlag == TRUE)) {
				break;
			}
			SetDownloadMark(hWnd, TRUE);
			break;

		// ��M�p�Ƀ}�[�N
		case ID_MENUITEM_DOWNMARK:
			if (SelBox == MAILBOX_SAVE || (SelBox == RecvBox && ExecFlag == TRUE)) {
				break;
			}
			SetDownloadMark(hWnd, FALSE);
			break;

		// �폜�p�Ƀ}�[�N
		case ID_MENUITEM_DELMARK:
			if (SelBox == MAILBOX_SAVE || SelBox == MAILBOX_SEND || (SelBox == RecvBox && ExecFlag == TRUE)) {
				break;
			}
			SetDeleteMark(hWnd);
			break;

		// �}�[�N����
		case ID_MENUITEM_UNMARK:
			if (SelBox == RecvBox && ExecFlag == TRUE) {
				break;
			}
			UnMark(hWnd);
			break;

		// �J���ς݂ɂ���
		case ID_MENUITEM_READMAIL:
			if (SelBox == MAILBOX_SEND) {
				break;
			}
			SetMailStats(hWnd, ICON_READ);
			break;

		// ���J���ɂ���
		case ID_MENUITEM_NOREADMAIL:
			if (SelBox == MAILBOX_SEND) {
				break;
			}
			SetMailStats(hWnd, ICON_MAIL);
			break;

		// �ۑ����փR�s�[
		case ID_MENUITE_SAVECOPY:
			ItemToSaveBox(hWnd);
			if (op.AutoSave == 1) {
				// �ۑ����̕ۑ�
				file_save_mailbox(SAVEBOX_FILE, MailBox + MAILBOX_SAVE, 2);
			}
			break;

		// �ꗗ����폜
		case ID_MENUITEM_DELETE:
			ListDeleteItem(hWnd);
			break;

		// ���ׂđI��
		case ID_MENUITEM_ALLSELECT:
			SetFocus(GetDlgItem(hWnd, IDC_LISTVIEW));
			ListView_SetItemState(GetDlgItem(hWnd, IDC_LISTVIEW), -1, LVIS_SELECTED, LVIS_SELECTED);
			break;

		// �A�J�E���g�̐؂�ւ�
		case ID_MENUITEM_CHANGEMAILBOX:
			SetFocus(GetDlgItem(hWnd, IDC_COMBO));
			SendDlgItemMessage(hWnd, IDC_COMBO, CB_SHOWDROPDOWN, TRUE, 0);
			break;

		// �ꗗ��ʂ̕\��
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

		// �E�B���h�E�̕\��
		case ID_MENUITEM_SHOWLASTWINDOW:
			SendMessage(hWnd, WM_SHOWLASTWINDOW, 0, 0);
			break;

		// ����
		case ID_MENUITEM_FIND:
			if (SelBox == MAILBOX_SEND) {
				break;
			}
			OpenItem(hWnd, TRUE, TRUE);
			if (hViewWnd != NULL) {
				View_FindMail(hViewWnd, TRUE);
			}
			break;

		// ��������
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

	// �^�X�N�g���C���b�Z�[�W
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
		/* �������ׂ��\�P�b�g������ */
		if (g_soc != (int)wParam) {
			break;
		}
		/* �\�P�b�g�C�x���g���̏������s�� */
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_CONNECT:					/* �T�[�o�ւ̐ڑ��������������������C�x���g */
			{
				TCHAR ErrStr[BUF_SIZE];
				*ErrStr = TEXT('\0');
				if (init_ssl(hWnd, g_soc, ErrStr) == -1) {
					ErrorSocketEnd(hWnd, RecvBox);
					SocketErrorMessage(hWnd, ErrStr, RecvBox);
				}
			}
			break;

		case FD_READ:						/* ��M�o�b�t�@�Ƀf�[�^�����鎖�������C�x���g */
			/* �f�[�^����M���Ē~�ς��� */
			if (recv_proc(hWnd, g_soc) == SELECT_MEM_ERROR) {
				// �������G���[
				ErrorSocketEnd(hWnd, RecvBox);
				SocketErrorMessage(hWnd, STR_ERR_MEMALLOC, RecvBox);
				break;
			}
			SetTimer(hWnd, ID_TIMEOUT_TIMER, TIMEOUTTIME * op.TimeoutInterval, NULL);
			break;

		case FD_WRITE:						/* ���M�\�Ȏ��������C�x���g */
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

		case FD_CLOSE:						/* �T�[�o�ւ̐ڑ����I���������������C�x���g */
			/* �ڑ����I������ */
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
						// �`�F�b�N����s
						break;
					}
					if (ExecFlag == TRUE && op.CheckAfterUpdate == 1 && RecvBox != MAILBOX_SEND) {
						// ���s��`�F�b�N
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

	// �\�P�b�g�̎�M���b�Z�[�W
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

	// ���[�����M
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

	// ���X�g�r���[�̃C�x���g
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

	// �X�e�[�^�X�o�[�ɕ������\��
	case WM_STATUSTEXT:
		SetStatusTextT(hWnd, (TCHAR *)lParam, 1);
		break;

	// �E�B���h�E�̕\��
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
 * InitApplication - �E�B���h�E�N���X�̓o�^
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
 * InitInstance - �E�B���h�E�̍쐬
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
 * MessageFunc - ���b�Z�[�W����
 */
BOOL MessageFunc(HWND hWnd, MSG *msg)
{
	HWND fWnd;

	fWnd = GetForegroundWindow();
	if (fWnd == NULL) {

	// ���C���E�B���h�E�̃A�N�Z�����[�^
	} else if (fWnd == hWnd &&
		TranslateAccelerator(fWnd, hAccel, msg) == TRUE) {
		return TRUE;

	// ���[���\���E�B���h�E�̃A�N�Z�����[�^
	} else if (fWnd == hViewWnd &&
		TranslateAccelerator(fWnd, hViewAccel, msg) == TRUE) {
		return TRUE;

	// ���[�������ʒm�E�B���h�E
	} else if (fWnd == MsgWnd &&
		IsDialogMessage(fWnd, msg) != 0) {
		return TRUE;

	// ���[���ҏW�E�B���h�E�̃A�N�Z�����[�^
	} else if (TranslateAccelerator(fWnd, hEditAccel, msg) == TRUE) {
		return TRUE;
	}
	TranslateMessage(msg);
	DispatchMessage(msg);
	return TRUE;
}

/*
 * WinMain - ���C��
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
	// �Q�d�N���N���h�~
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

	// �A�v���P�[�V�����̍�ƃp�X�̎擾
	if (GetAppPath(hInstance) == FALSE) {
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		ErrorMessage(NULL, STR_ERR_MEMALLOC);
		return 0;
	}

	{
		int TmpCmdShow;
		// �N���p�X���[�h�̃`�F�b�N
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

	// ������
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
	// �V����CommonControl�̏��������s��
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

	// �E�B���h�E�N���X�o�^
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
	// EDIT�o�^
	if (nedit_regist(hInstance) == FALSE) {
		return 0;
	}
#endif
	// ���C���E�B���h�E�̍쐬
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

	// ���\�[�X����|�b�v�A�b�v���j���[�����[�h
	hPOPUP = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU_POPUP));

	// ���\�[�X����A�N�Z�����[�^�����[�h
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
	hViewAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR_VIEW));
	hEditAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR_EDIT));

	// ���b�Z�[�W���[�v
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

	// �R�}���h���C���̍쐬
	cmdline = GetCommandLine();
    if (*cmdline == '"') {
		for (cmdline++; *cmdline != '\0' && *cmdline != '"'; cmdline++);
		if (*cmdline != '\0') cmdline++;
	} else {
		for (; *cmdline != '\0' && *cmdline != ' '; cmdline++);
	}
	for (; *cmdline == ' '; cmdline++);

	// �N�����̏��̎擾
	stinfo.cb = sizeof(STARTUPINFO);
	stinfo.dwFlags = STARTF_USESHOWWINDOW;
	GetStartupInfo(&stinfo);

	// WinMain�̌Ăяo��
	ret = WinMain(GetModuleHandle(NULL), NULL, cmdline, stinfo.wShowWindow);
	// �v���Z�X�̏I��
	ExitProcess(ret);
}
#endif
#endif
/* End of source */
