/*
 * nPOP
 *
 * General.h
 *
 * Copyright (C) 1996-2020 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#pragma once

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <tchar.h>

#ifdef UNICODE
#include <stdlib.h>
#endif

#include <resource.h>
#include <Strtbl.h>
#include "font.h"
#include "ssl.h"

/* Define */
#define APP_NAME				TEXT("nPOP Ver 1.2.5")
#define WINDOW_TITLE			TEXT("nPOP")
#define KEY_NAME				TEXT("nPOP")

#define STR_MUTEX				TEXT("_nPOP_Mutex_")
#define ATTACH_FILE				TEXT("_attach_")
#define ATTACH_SEP				TEXT('|')

#define MAIN_WND_CLASS			TEXT("nPOPMainWndClass")
#define VIEW_WND_CLASS			TEXT("nPOPViewWndClass")
#define EDIT_WND_CLASS			TEXT("nPOPEditWndClass")

#define ADDRESS_FILE			TEXT("Address.lst")
#define SAVEBOX_FILE			TEXT("SaveBox.dat")
#define SENDBOX_FILE			TEXT("SendBox.dat")

#define VIEW_FILE				TEXT("$npop_view")

#define LOG_FILE				TEXT("nPOP.log")

#define BUF_SIZE				256					// バッファサイズ
#define MAXSIZE					32768
#define EDITMAXSIZE				30000

#define SICONSIZE				Scale(16)			// 小さいアイコンのサイズ
#define TB_ICONSIZE				16					// ツールバーのボタンサイズ

#define TABSTOPLEN				8					// TAB Stop

#define LV_COL_CNT				4					// ListViewのカラム数
#define AD_COL_CNT				2					// ListView Addressbook

#define MAILBOX_SAVE			0					// 固定メールボックス
#define MAILBOX_SEND			1
#define MAILBOX_USER			2

#define IDC_COMBO				400					// コントロールID
#define IDC_LISTVIEW			401
#define IDC_STATUS				402

#ifndef LVS_EX_INFOTIP
#define LVS_EX_INFOTIP			0x400
#endif

#define WM_SOCK_SELECT			(WM_APP + 1)		// ソケットイベント
#define WM_SOCK_RECV			(WM_APP + 2)		// ソケット受信イベント
#define WM_LV_EVENT				(WM_APP + 3)		// リストビューイベント
#define WM_STATUSTEXT			(WM_APP + 4)		// ステータスバーへ文字列設定
#define WM_SHOWLASTWINDOW		(WM_APP + 5)		// ウィンドウ表示
#define WM_SMTP_SENDMAIL		(WM_APP + 6)		// メール送信
#define WM_ENDCLOSE				(WM_APP + 7)		// ウィンドウの終了
#define WM_INITTRAYICON			(WM_APP + 8)		// タスクトレイアイコンの初期化

#define WM_SHOWDIALOG			(WM_APP + 9)		// メール到着メッセージ表示
#define WM_ENDDIALOG			(WM_APP + 10)		// メール到着メッセージ終了

#define WM_CHANGE_MARK			(WM_APP + 11)

#define EDIT_OPEN				0					// 送信メール編集のタイプ
#define EDIT_NEW				1
#define EDIT_REPLY				2

#define EDIT_NONEDIT			0					// 送信メール編集の戻り値
#define EDIT_INSIDEEDIT			1
#define EDIT_OUTSIDEEDIT		2

#define SELECT_MEM_ERROR		-2					// ソケット処理の戻り値
#define SELECT_SOC_ERROR		-1
#define SELECT_SOC_CLOSE		0
#define SELECT_SOC_SUCCEED		1
#define SELECT_SOC_NODATA		2

#define SORT_NO					100					// ソートフラグ
#define SORT_IOCN				101
#define SORT_THREAD				102

#define POP_ERR					-2					// POP3コマンドフラグ
#define POP_QUIT				-1
#define POP_START				0
#define POP_STARTTLS			1
#define POP_USER				2
#define POP_PASS				3
#define POP_LOGIN				4
#define POP_STAT				5
#define POP_LIST				6
#define POP_UIDL				7
#define POP_UIDL_ALL			8
#define POP_UIDL_CHECK			9
#define POP_TOP					10
#define POP_RETR				11
#define POP_DELE				12

#define SMTP_ERR				POP_ERR				// SMTPコマンドフラグ
#define SMTP_QUIT				POP_QUIT
#define SMTP_START				POP_START
#define SMTP_EHLO				1
#define SMTP_STARTTLS			2
#define SMTP_AUTH_CRAM_MD5		3
#define SMTP_AUTH_LOGIN			4
#define SMTP_AUTH_LOGIN_PASS	5
#define SMTP_HELO				6
#define SMTP_RSET				7
#define SMTP_MAILFROM			8
#define SMTP_RCPTTO				9
#define SMTP_DATA				10
#define SMTP_SENDBODY			11
#define SMTP_NEXTSEND			12
#define SMTP_SENDEND			13

#define ICON_NON				0					// アイコン状態
#define ICON_MAIL				1
#define ICON_READ				2
#define ICON_DOWN				3
#define ICON_DEL				4
#define ICON_SENDMAIL			5
#define ICON_SEND				6
#define ICON_ERROR				7
#define ICON_NEW				8

#define FILTER_UNRECV			1					// フィルタタイプ
#define FILTER_RECV				2
#define FILTER_DOWNLOADMARK		4
#define FILTER_DELETEMARK		8
#define FILTER_READICON			16
#define FILTER_SAVE				32

#define MP_ERROR_FILE			-2					// マルチパート処理の戻り値
#define MP_ERROR_ALLOC			-1
#define MP_NO_ATTACH			0
#define MP_ATTACH				1

#define HEAD_SUBJECT			"Subject:"
#define HEAD_FROM				"From:"
#define HEAD_TO					"To:"
#define HEAD_CC					"Cc:"
#define HEAD_BCC				"Bcc:"
#define HEAD_REPLYTO			"Reply-To:"
#define HEAD_DATE				"Date:"
#define HEAD_SIZE				"Content-Length:"
#define HEAD_MESSAGEID			"Message-Id:"
#define HEAD_INREPLYTO			"In-Reply-To:"
#define HEAD_REFERENCES			"References:"
#define HEAD_MIMEVERSION		"MIME-Version:"
#define HEAD_CONTENTTYPE		"Content-Type:"
#define HEAD_ENCODING			"Content-Transfer-Encoding:"
#define HEAD_DISPOSITION		"Content-Disposition:"
#define HEAD_X_MAILER			"X-Mailer:"
#define HEAD_X_UIDL				"X-UIDL:"
#define HEAD_X_NO				"X-No:"
#define HEAD_X_MARK				"X-Mark:"
#define HEAD_X_STATUS			"X-Status:"
#define HEAD_X_DOWNLOAD			"X-Download:"
#define HEAD_X_MAILBOX			"X-MailBox:"
#define HEAD_X_ATTACH			"X-File:"
#define HEAD_X_HEADCHARSET		"X-Header-Charset:"
#define HEAD_X_HEADENCODE		"X-Header-Encodeing:"
#define HEAD_X_BODYCHARSET		"X-Body-Charset:"
#define HEAD_X_BODYENCODE		"X-Body-Encodeing:"

#define HEAD_X_NO_OLD			"X-MailNo:"
#define HEAD_X_MARK_OLD			"X-MarkStatus:"
#define HEAD_X_STATUS_OLD		"X-MailStatus:"
#define HEAD_X_DOWNLOAD_OLD		"X-MailDownload:"
#define HEAD_X_ATTACH_OLD		"X-Attach:"

#define CHARSET_US_ASCII		"US-ASCII"
#define CHARSET_ISO_8859_1		"ISO-8859-1"
#define CHARSET_ISO_2022_JP		"ISO-2022-JP"
#define CHARSET_UTF_7			"UTF-7"
#define CHARSET_UTF_8			"UTF-8"

#define ENCODE_7BIT				"7bit"
#define ENCODE_8BIT				"8bit"
#define ENCODE_BASE64			"base64"
#define ENCODE_Q_PRINT			"quoted-printable"

#define URL_HTTP				TEXT("http://")
#define URL_HTTPS				TEXT("https://")
#define URL_FTP					TEXT("ftp://")
#define URL_MAILTO				TEXT("mailto:")

#define ABS(n)					((n < 0) ? (n * -1) : n)				// 絶対値

/* Struct */
typedef struct _OPTION {
	int StertPass;
	int ShowPass;
	TCHAR *Password;

	TCHAR DataFileDir[BUF_SIZE];

	FONT_INFO view_font;
	FONT_INFO lv_font;

	RECT MainRect;
	RECT ViewRect;
	RECT EditRect;

	int ShowTrayIcon;
	int StartHide;
	int MinsizeHide;
	int CloseHide;
	int TrayIconToggle;
	int StartInit;
	int SocLog;

	int LvDefSelectPos;
	int LvAutoSort;
	int LvSortItem;
	int LvThreadView;
	int LvStyle;
	int LvStyleEx;
	int MoveAllMailBox;
	int RecvScroll;
	int SaveMsg;
	int AutoSave;
	int LvColSize[LV_COL_CNT];
	int AddColSize[AD_COL_CNT];

	int ListGetLine;
	int ListDownload;
	int ShowHeader;
	int ListSaveMode;
	int WordBreakFlag;
	int EditWordBreakFlag;
	int ViewShowDate;
	int MstchCase;
	int AllFind;
	int SubjectFind;

	int ESMTP;
	TCHAR *SendHelo;
	int SendMessageId;
	int SendDate;
	int SelectSendBox;
	int PopBeforeSmtpIsLoginOnly;
	int PopBeforeSmtpWait;

	// SSL
	TCHAR *CAFile;

	TCHAR *HeadCharset;
	int HeadEncoding;
	TCHAR *BodyCharset;
	int BodyEncoding;

	int AutoQuotation;
	TCHAR *QuotationChar;
	int WordBreakSize;
	int QuotationBreak;
	TCHAR *ReSubject;
	TCHAR *ReHeader;
	TCHAR *Bura;
	TCHAR *Oida;
	TCHAR *sBura;
	TCHAR *sOida;

	int IPCache;
	int EncodeType;
	TCHAR *TimeZone;
	TCHAR *DateFormat;
	TCHAR *TimeFormat;

	int ShowNewMailMessgae;
	int ShowNoMailMessage;
	int ActiveNewMailMessgae;

	int NewMailSound;
	TCHAR *NewMailSoundFile;
	int ExecEndSound;
	TCHAR *ExecEndSoundFile;

	int AutoCheck;
	int AutoCheckTime;
	int StartCheck;
	int CheckAfterUpdate;
	int SocIgnoreError;
	int SendIgnoreError;
	int CheckEndExec;
	int CheckEndExecNoDelMsg;
	int TimeoutInterval;

	int ViewClose;
	TCHAR *ViewApp;
	TCHAR *ViewAppCmdLine;
	TCHAR *ViewFileSuffix;
	TCHAR *ViewFileHeader;
	int ViewAppClose;
	int DefViewApp;
	TCHAR *EditApp;
	TCHAR *EditAppCmdLine;
	TCHAR *EditFileSuffix;
	int DefEditApp;
	TCHAR *AttachPath;
	int AttachWarning;
	int AttachDelete;

	TCHAR *URLApp;
} OPTION;

typedef struct _SSL_INFO {
	int Type;
	int Verify;
} SSL_INFO;

typedef struct _MAILBOX {
	TCHAR *Name;

	// POP
	TCHAR *Server;
	int Port;
	TCHAR *User;
	TCHAR *Pass;
	TCHAR *TmpPass;
	int APOP;
	int PopSSL;
	SSL_INFO PopSSLInfo;
	int NoRETR;
	int NoUIDL;
	unsigned long PopIP;

	int MailCnt;
	unsigned int MailSize;

	char *LastMessageId;
	int LastNo;

	// SMTP
	TCHAR *SmtpServer;
	int SmtpPort;
	TCHAR *UserName;
	TCHAR *MailAddress;
	TCHAR *Signature;
	TCHAR *ReplyTo;
	int MyAddr2Bcc;
	TCHAR *BccAddr;
	int PopBeforeSmtp;
	int SmtpAuth;
	int SmtpAuthType;
	int AuthUserPass;
	TCHAR *SmtpUser;
	TCHAR *SmtpPass;
	TCHAR *SmtpTmpPass;
	int SmtpSSL;
	SSL_INFO SmtpSSLInfo;
	unsigned long SmtpIP;

	// Check
	BOOL NewMail;
	BOOL NoRead;
	int CyclicFlag;

	// Filter
	int FilterEnable;
	struct _FILTER **tpFilter;
	int FilterCnt;

	// MailItem
	struct _MAILITEM **tpMailItem;
	int MailItemCnt;
	int AllocCnt;
} MAILBOX;

typedef struct _MAILITEM {
	int No;
	int Status;
	int MailStatus;
	int PrevNo;
	int NextNo;
	int Indent;
	BOOL New;
	BOOL Download;
	BOOL Multipart;
	HWND hEditWnd;
	HANDLE hProcess;

	TCHAR *From;
	TCHAR *To;
	TCHAR *Cc;
	TCHAR *Bcc;
	TCHAR *Date;
	TCHAR *Size;
	TCHAR *Subject;
	TCHAR *ReplyTo;
	TCHAR *ContentType;
	TCHAR *Encoding;
	TCHAR *MessageID;
	TCHAR *UIDL;
	TCHAR *InReplyTo;
	TCHAR *References;
	char *Body;

	TCHAR *MailBox;
	TCHAR *Attach;
	TCHAR *HeadCharset;
	int HeadEncoding;
	TCHAR *BodyCharset;
	int BodyEncoding;
} MAILITEM;

typedef struct _FILTER {
	int Enable;
	int Action;

	TCHAR *Header1;
	TCHAR *Content1;

	TCHAR *Header2;
	TCHAR *Content2;
} FILTER;

typedef struct _ATTACHINFO {
	TCHAR *from;
	TCHAR *fname;
	TCHAR *mime;
	int size;
} ATTACHINFO;

/* Function Prototypes */
// WinSock
unsigned long get_host_by_name(HWND hWnd, TCHAR *server, TCHAR *ErrStr);
SOCKET connect_server(HWND hWnd, unsigned long ip_addr, unsigned short port, TCHAR* server, const int use_ssl, const int _ssl_type, const int _ssl_verify, TCHAR* ErrStr);
int recv_proc(HWND hWnd, SOCKET soc);
#ifndef WSAASYNC
int recv_select(HWND hWnd, SOCKET soc);
#endif	//WSAASYNC
int send_data(SOCKET soc, char *wbuf, int len);
int send_buf(SOCKET soc, char *buf);
#ifdef UNICODE
int send_buf_t(SOCKET soc, TCHAR *wbuf);
#else	//UNICODE
#define send_buf_t	send_buf
#endif	//UNICODE
void socket_close(HWND hWnd, SOCKET soc);
int init_ssl(const HWND hWnd, const SOCKET soc, TCHAR* ErrStr);

// Pop3
BOOL pop3_list_proc(HWND hWnd, SOCKET soc, char *buf, int len, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
BOOL pop3_exec_proc(HWND hWnd, SOCKET soc, char *buf, int len, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
void pop3_free(void);

// Smtp
void HMAC_MD5(unsigned char *input, int len, unsigned char *key, int keylen, unsigned char *digest);
#ifdef WSAASYNC
BOOL smtp_send_proc(HWND hWnd, SOCKET soc, TCHAR *ErrStr, MAILBOX *tpMailBox);
#endif
BOOL smtp_proc(HWND hWnd, SOCKET soc, char *buf, int len, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
SOCKET smtp_send_mail(HWND hWnd, MAILBOX *tpMailBox, MAILITEM *tpMailItem, int EndMailFlag, TCHAR *ErrStr);
void smtp_set_error(HWND hWnd);
void smtp_free(void);

// File
BOOL log_init(TCHAR *fpath, TCHAR *fname, TCHAR *buf);
BOOL log_save(TCHAR *fpath, TCHAR *fname, TCHAR *buf);
BOOL log_clear(TCHAR *fpath, TCHAR *fname);
BOOL dir_check(TCHAR *path);
BOOL dir_delete(TCHAR *Path, TCHAR *file);
void filename_conv(TCHAR *buf);
BOOL filename_select(HWND hWnd, TCHAR *ret, TCHAR *DefExt, TCHAR *filter, BOOL OpenSave);
long file_get_size(TCHAR *FileName);
char *file_read(TCHAR *path, long FileSize);
BOOL file_read_select(HWND hWnd, TCHAR **buf);
BOOL file_read_mailbox(TCHAR *FileName, MAILBOX *tpMailBox);
int file_read_address_book(TCHAR *FileName, MAILBOX *tpMailBox);
BOOL file_write(HANDLE hFile, char *buf, int len);
BOOL file_write_ascii(HANDLE hFile, TCHAR *buf, int len);
BOOL file_write_utf8(HANDLE hFile, TCHAR* buf);
BOOL file_save(HWND hWnd, TCHAR *FileName, TCHAR *Ext, char *buf, int len);
BOOL file_save_exec(HWND hWnd, TCHAR *FileName, char *buf, int len);
BOOL file_save_mailbox(TCHAR *FileName, MAILBOX *tpMailBox, int SaveFlag);
BOOL file_save_address_book(TCHAR *FileName, MAILBOX *tpMailBox);

// Ini
BOOL ini_start_auth_check(void);
BOOL ini_read_setting(HWND hWnd);
BOOL ini_save_setting(HWND hWnd, BOOL SaveMailFlag);
void ini_free(void);

// Item
BOOL item_is_mailbox(MAILBOX *tpMailBox, MAILITEM *tpMailItem);
BOOL item_set_count(MAILBOX *tpMailBox, int i);
BOOL item_add(MAILBOX *tpMailBox, MAILITEM *tpNewMailItem);
void item_copy(MAILITEM *tpFromMailItem, MAILITEM *tpToMailItem);
MAILITEM *item_to_mailbox(MAILBOX *tpMailBox, MAILITEM *tpNewMailItem, TCHAR *MailBoxName, BOOL SendClear);
BOOL item_resize_mailbox(MAILBOX *tpMailBox);
void item_free(MAILITEM **tpMailItem, int cnt);
char *item_get_message_id(char *buf);
int item_get_number_to_index(MAILBOX *tpMailBox, int No);
int item_get_next_download_mark(MAILBOX *tpMailBox, int Index, int *No);
int item_get_next_delete_mark(MAILBOX *tpMailBox, int Index, int *No);
int item_get_next_send_mark(MAILBOX *tpMailBox, int Index, int *MailBoxIndex);
int item_get_next_send_mark_mailbox(MAILBOX *tpMailBox, int Index, int MailBoxIndex);
BOOL item_mail_to_item(MAILITEM *tpMailItem, char *buf, int Size, BOOL download);
MAILITEM *item_header_to_item(MAILBOX *tpMailBox, char *buf, int Size);
MAILITEM *item_string_to_item(MAILBOX *tpMailBox, char *buf);
MAILITEM *item_string_to_item_utf8(MAILBOX* tpMailBox, char* buf);
int item_to_string_size(MAILITEM *tpMailItem, BOOL BodyFlag);
char *item_to_string(char *buf, MAILITEM *tpMailItem, BOOL BodyFlag);
int item_find_thread(MAILBOX *tpMailBox, TCHAR *p, int Index);
void item_create_thread(MAILBOX *tpMailBox);

// MailBox
BOOL mailbox_init(void);
int mailbox_create(HWND hWnd, BOOL ShowFlag);
int mailbox_delete(HWND hWnd, int DelIndex);
BOOL mailbox_read(void);
void mailbox_move_up(HWND hWnd);
void mailbox_move_down(HWND hWnd);
BOOL mailbox_unread_check(int index, BOOL NewFlag);
int mailbox_next_unread(int index, int endindex);
void mailbox_select(HWND hWnd, int Sel);
int mailbox_name_to_index(TCHAR *Name);
void filer_free(MAILBOX *tpMailBox);
void mailbox_free(MAILBOX *tpMailBox);

// util
TCHAR *GetHeaderStringPointT(TCHAR *buf, TCHAR *str);
#ifdef UNICODE
char *GetHeaderStringPoint(char *buf, char *str);
#else
#define GetHeaderStringPoint GetHeaderStringPointT
#endif
int GetHeaderStringSizeT(TCHAR *buf, BOOL CrLfFlag);
#ifdef UNICODE
int GetHeaderStringSize(char *buf, BOOL CrLfFlag);
#else
#define GetHeaderStringSize GetHeaderStringSizeT
#endif
BOOL GetHeaderStringT(TCHAR *buf, TCHAR *ret, BOOL CrLfFlag);
#ifdef UNICODE
BOOL GetHeaderString(char *buf, char *ret, BOOL CrLfFlag);
#else
#define GetHeaderString GetHeaderStringT
#endif

TCHAR *GetBodyPointaT(TCHAR *buf);
#ifdef UNICODE
char *GetBodyPointa(char *buf);
#else
#define GetBodyPointa GetBodyPointaT
#endif

void TrimMessageId(char *buf);
int GetReferencesSize(char *p, BOOL Flag);
BOOL ConvReferences(char *p, char *r, BOOL Flag);

void DateAdd(SYSTEMTIME *sTime, char *tz);
int DateConv(char *buf, char *ret);
int SortDateConv(char *buf, char *ret);
void GetTimeString(TCHAR *buf);
void EncodePassword(TCHAR *Key, TCHAR *Word, TCHAR *ret, int retsize, BOOL decode);
void EncodeCtrlChar(TCHAR *buf, TCHAR *ret);
void DecodeCtrlChar(TCHAR *buf, TCHAR *ret);
TCHAR *CreateMessageId(long id, TCHAR *MailAddress);
int CreateHeaderStringSize(TCHAR *buf, MAILITEM *tpMailItem);
TCHAR *CreateHeaderString(TCHAR *buf, TCHAR *ret, MAILITEM *tpMailItem);
int GetReplyBodySize(TCHAR *body, TCHAR *ReStr);
TCHAR *SetReplyBody(TCHAR *body, TCHAR *ret, TCHAR *ReStr);
int SetDotSize(TCHAR *buf);
void SetDot(TCHAR *buf, TCHAR *ret);
void DelDot(TCHAR *buf, TCHAR *ret);
int WordBreakStringSize(TCHAR *buf, TCHAR *str, int BreakCnt, BOOL BreakFlag);
void WordBreakString(TCHAR *buf, TCHAR *ret, TCHAR *str, int BreakCnt, BOOL BreakFlag);
BOOL URLToMailItem(TCHAR *buf, MAILITEM *tpMailItem);
TCHAR *GetMailAddress(TCHAR *buf, TCHAR *ret, BOOL quote);
TCHAR *GetMailString(TCHAR *buf, TCHAR *ret);
void SetUserName(TCHAR *buf, TCHAR *ret);
int SetCcAddressSize(TCHAR *To);
TCHAR *SetCcAddress(TCHAR *Type, TCHAR *To, TCHAR *r);
TCHAR *GetFileNameString(TCHAR *p);
int SetAttachListSize(TCHAR *buf);
TCHAR *SetAttachList(TCHAR *buf, TCHAR *ret);
TCHAR *GetMIME2Extension(TCHAR *MIMEStr, TCHAR *Filename);
TCHAR *CreateCommandLine(TCHAR *buf, TCHAR *filename, BOOL spFlag);

// View
void SetWordBreakMenu(HWND hWnd, HMENU hEditMenu, int Flag);
int SetWordBreak(HWND hWnd);
void View_FindMail(HWND hWnd, BOOL FindSet);
BOOL View_InitApplication(HINSTANCE hInstance);
BOOL View_InitInstance(HINSTANCE hInstance, LPVOID lpParam, BOOL NoAppFlag);

// Edit
BOOL CALLBACK enum_windows_proc(const HWND hWnd, const LPARAM lParam);
BOOL Edit_InitApplication(HINSTANCE hInstance);
int Edit_MailToSet(HINSTANCE hInstance, HWND hWnd, TCHAR *mail_addr, int rebox);
int Edit_InitInstance(HINSTANCE hInstance, HWND hWnd, int rebox,
					   MAILITEM *tpReMailItem, int OpenFlag, int ReplyFag);

// Option
void AllocGetText(HWND hEdit, TCHAR **buf);
BOOL SetMailBoxOption(HWND hWnd);
BOOL CALLBACK SetEncodeProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SetOption(HWND hWnd);
BOOL CALLBACK InputPassProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK InitMailBoxProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SetAttachProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CheckDependence(HWND hWnd, int Ctl);
BOOL CALLBACK SetSendProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MailPropProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AddressListProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SetFindProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK NewMailMessageProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AttachNoticeProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ListView
void ListView_AddColumn(HWND hListView, int fmt, int cx, TCHAR *buf, int iSubItem);
HWND CreateListView(HWND hWnd, int Top, int bottom);
void ListView_SetRedraw(HWND hListView, BOOL DrawFlag);
int ListView_InsertItemEx(HWND hListView, TCHAR *buf, int len, int Img, long lp, int iItem);
void ListView_MoveItem(HWND hListView, int SelectItem, int Move, int ColCnt);
TCHAR *ListView_GetSelStringList(HWND hListView);
long ListView_GetlParam(HWND hListView, int i);
int ListView_GetMemToItem(HWND hListView, MAILITEM *tpMemMailItem);
int ListView_GetNextDeleteItem(HWND hListView, int Index);
int ListView_GetNextMailItem(HWND hListView, int Index);
int ListView_GetPrevMailItem(HWND hListView, int Index);
int ListView_GetNextNoReadItem(HWND hListView, int Index, int endindex);
int ListView_GetNewItem(HWND hListView, MAILBOX *tpMailBox);
BOOL ListView_ShowItem(HWND hListView, MAILBOX *tpMailBox);
int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
LRESULT ListView_NotifyProc(HWND hWnd, LPARAM lParam);

// main
void SwitchCursor(const BOOL Flag);
BOOL _SetForegroundWindow(const HWND hWnd);
void SetStatusTextT(HWND hWnd, TCHAR *buf, int Part);
void SetSocStatusTextT(HWND hWnd, TCHAR *buf);
#ifdef UNICODE
void SetSocStatusText(HWND hWnd, char *buf);
#else
#define SetSocStatusText(hWnd, buf)	SetSocStatusTextT(hWnd, buf)
#endif
void SetItemCntStatusText(HWND hWnd, MAILBOX *tpViewMailBox);
void SetStatusRecvLen(HWND hWnd, int len, TCHAR *msg);
void ErrorMessage(HWND hWnd, TCHAR *buf);
void SocketErrorMessage(HWND hWnd, TCHAR *buf, int BoxIndex);
void ErrorSocketEnd(HWND hWnd, int BoxIndex);
int ShowMenu(HWND hWnd, HMENU hMenu, int mpos, int PosFlag, BOOL ReturnFlag);
void SetMailMenu(HWND hWnd);
void SetNoReadCntTitle(HWND hWnd);
BOOL MessageFunc(HWND hWnd, MSG *msg);
/* End of source */
