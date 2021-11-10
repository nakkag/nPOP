/*
 * nPOP
 *
 * Pop3.c (RFC 1939)
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include "General.h"
#include "Memory.h"
#include "String.h"
#include "global.h"
#include "md5.h"

/* Define */
#define OK_LEN					3			// "+OK" のバイト数

#define MAIL_BUF_SIZE			32768		// メール受信用バッファの初期サイズ
#define DOWNLOAD_SIZE			65535		// RETR未使用時の全文受信サイズ

#define HEAD_LINE				30			// ヘッダー長
#define LINE_LEN				80

#define REDRAWCNT				100			// ステータスバー再設定数

#define CMD_USER				"USER"
#define CMD_PASS				"PASS"
#define CMD_APOP				"APOP"
#define CMD_STLS				"STLS"
#define CMD_STAT				"STAT"
#define CMD_LIST				"LIST"
#define CMD_UIDL				"UIDL"
#define CMD_TOP					"TOP"
#define CMD_RETR				"RETR"
#define CMD_DELE				"DELE"
#define CMD_RSET				"RSET"
#define CMD_QUIT				"QUIT"

/* Global Variables */
static char *mail_buf = NULL;				// メール受信用バッファ
static int mail_buf_size;					// メール受信用バッファの実サイズ
static int mail_buf_len;					// メール受信用バッファ内の文字列長
static int mail_size = -1;					// メールサイズ
static int list_get_no;						// 受信メール位置
static int download_get_no;					// ダウンロードメール位置
static int delete_get_no;					// 削除メール位置
static BOOL init_recv;						// 新着取得位置初期化フラグ
static BOOL mail_received;					// １件目受信フラグ
static BOOL receiving_uidl;					// UIDLレスポンス受信中
static BOOL receiving_data;					// メールデータ受信中
static BOOL disable_uidl;					// UIDLサポートフラグ
static BOOL disable_top;					// TOPサポートフラグ (op.ListDownload が 1 or TOPが未サポート)

typedef struct _UIDL_INFO {
	int no;
	TCHAR *uidl;
} UIDL_INFO;

static UIDL_INFO *ui;						// UIDLリスト
static int ui_size;							// UIDLリスト数
static int ui_pt;							// UIDL追加位置
static MAILITEM *uidl_item;					// UIDLをセットするメールアイテム

// 外部参照
extern OPTION op;

extern int command_status;
extern int NewMailCnt;
extern TCHAR *g_Pass;
extern HWND hViewWnd;						// 表示ウィンドウ

extern BOOL ShowMsgFlag;
extern BOOL NewMail_Flag;
extern BOOL EndThreadSortFlag;

extern int PopBeforeSmtpFlag;
extern SSL* ssl;

/* Local Function Prototypes */
static BOOL mail_buf_init(int size);
static BOOL mail_buf_set(char *buf, int len);
static void mail_buf_free(void);
static BOOL uidl_init(int size);
static BOOL uidl_set(char *buf, int len);
static TCHAR *uidl_get(int get_no);
static int uidl_check(TCHAR *buf);
static void uidl_free(void);
static void init_mailbox(HWND hWnd, MAILBOX *tpMailBox, BOOL ShowFlag);
static char *skip_response(char *p);
static BOOL check_response(char *buf);
static BOOL check_message_id(char *buf, MAILITEM *tpMailItem, TCHAR *ErrStr, MAILBOX *tpMailBox);
static int check_last_mail(HWND hWnd, SOCKET soc, BOOL chek_flag, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
static BOOL send_command(HWND hWnd, SOCKET soc, TCHAR *Command, int Cnt, TCHAR *ErrStr);
static int send_command_top(HWND hWnd, SOCKET soc, int Cnt, TCHAR *ErrStr, int len, int ret);
static TCHAR *create_apop(char *buf, TCHAR *ErrStr, MAILBOX *tpMailBox, TCHAR *sPass);
static int login_proc(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox);
static int list_proc_stat(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
static int list_proc_uidl_all(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
static int list_proc_uidl_check(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
static int list_proc_uidl_set(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
static int list_proc_list(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
static int list_proc_top(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
static int exec_send_check_command(HWND hWnd, SOCKET soc, int get_no, TCHAR *ErrStr, MAILBOX *tpMailBox);
static int exec_proc_init(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
static int exec_proc_retr(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
static int exec_proc_uidl(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
static int exec_proc_top(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);
static int exec_proc_dele(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag);

/*
 * mail_buf_init - メールバッファを初期化
 */
static BOOL mail_buf_init(int size)
{
	if (mail_buf == NULL || mail_buf_size < size) {
		mem_free(&mail_buf);
		mail_buf_size = size + 3;
		mail_buf = (char *)mem_alloc(mail_buf_size);
	}
	mail_buf_len = 0;
	return ((mail_buf == NULL) ? FALSE : TRUE);
}

/*
 * mail_buf_set - メールバッファに文字列を追加
 */
static BOOL mail_buf_set(char *buf, int len)
{
	char *tmp;

	if (mail_buf_size < (mail_buf_len + len + 2 + 1)) {
		// 再確保
		mail_buf_size += (MAIL_BUF_SIZE < len + 2 + 1) ? (len + 2 + 1) : MAIL_BUF_SIZE;
		tmp = (char *)mem_alloc(mail_buf_size);
		if (tmp == NULL) {
			mail_buf_size = 0;
			return FALSE;
		}
		CopyMemory(tmp, mail_buf, mail_buf_len);
		CopyMemory(tmp + mail_buf_len, buf, len);
		*(tmp + mail_buf_len + len + 0) = '\r';
		*(tmp + mail_buf_len + len + 1) = '\n';
		*(tmp + mail_buf_len + len + 2) = '\0';
		mail_buf_len += (len + 2);
		// 旧メモリの解放
		mem_free(&mail_buf);
		mail_buf = tmp;
	} else {
		// 追加
		CopyMemory(mail_buf + mail_buf_len, buf, len);
		*(mail_buf + mail_buf_len + len + 0) = '\r';
		*(mail_buf + mail_buf_len + len + 1) = '\n';
		*(mail_buf + mail_buf_len + len + 2) = '\0';
		mail_buf_len += (len + 2);
	}
	return TRUE;
}

/*
 * mail_buf_free - メールバッファを解放
 */
static void mail_buf_free(void)
{
	if (mail_buf != NULL) {
		mem_free(&mail_buf);
		mail_buf = NULL;
	}
}

/*
 * uidl_init - UIDLリストを初期化
 */
static BOOL uidl_init(int size)
{
	uidl_free();
	ui_size = size;
	ui_pt = 0;
	ui = (UIDL_INFO *)mem_calloc(sizeof(UIDL_INFO) * ui_size);
	return ((ui == NULL) ? FALSE : TRUE);
}

/*
 * uidl_set - UIDLリストに文字列を追加
 */
static BOOL uidl_set(char *buf, int len)
{
	char *p;

	if (ui_pt >= ui_size) {
		return FALSE;
	}

	ui[ui_pt].no = a2i(buf);
	for (p = buf; *p != ' ' && *p != '\0'; p++);		// 番号
	for (; *p == ' '; p++);								// 空白

	ui[ui_pt].uidl = alloc_char_to_tchar(p);
	if (ui[ui_pt].uidl == NULL) {
		return FALSE;
	}
	ui_pt++;
	return TRUE;
}

/*
 * uidl_get - UIDL取得
 */
static TCHAR *uidl_get(int get_no)
{
	int i;

	for (i = 0; i < ui_size; i++) {
		if (ui[i].no == get_no) {
			return ui[i].uidl;
		}
	}
	return NULL;
}

/*
 * uidl_check - UIDLがUIDLの一覧に存在するかチェック
 */
static int uidl_check(TCHAR *buf)
{
	int i;

	if (buf == NULL) {
		return 0;
	}
	for (i = 0; i < ui_size; i++) {
		if (lstrcmp(ui[i].uidl, buf) == 0) {
			return ui[i].no;
		}
	}
	return 0;
}

/*
 * uidl_free - UIDLリストを解放
 */
static void uidl_free(void)
{
	int i;

	if (ui == NULL) return;

	for (i = 0; i < ui_size; i++) {
		mem_free(&ui[i].uidl);
	}
	mem_free(&ui);
	ui = NULL;
	ui_size = 0;
}

/*
 * init_mailbox - メールを最初から受信に設定
 */
static void init_mailbox(HWND hWnd, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	if (ShowFlag == TRUE) {
		ListView_DeleteAllItems(GetDlgItem(hWnd, IDC_LISTVIEW));
	}
	item_free(tpMailBox->tpMailItem, tpMailBox->MailItemCnt);
	mem_free((void **)&tpMailBox->tpMailItem);
	tpMailBox->tpMailItem = NULL;
	tpMailBox->AllocCnt = tpMailBox->MailItemCnt = 0;
	tpMailBox->LastNo = 0;
}

/*
 * skip_response - レスポンスのスキップ
 */
static char *skip_response(char *p)
{
	for (; *p != ' ' && *p != '\0'; p++);	// +OK
	for (; *p == ' '; p++);					// 空白
	for (; *p != ' ' && *p != '\0'; p++);	// 番号
	for (; *p == ' '; p++);					// 空白
	return p;
}

/*
 * check_response - レスポンスのチェック
 */
static BOOL check_response(char *buf)
{
	return ((tstrlen(buf) < OK_LEN || *buf != '+' ||
		!(*(buf + 1) == 'O' || *(buf + 1) == 'o') ||
		!(*(buf + 2) == 'K' || *(buf + 2) == 'k')) ? FALSE : TRUE);
}

/*
 * check_message_id - メッセージIDのチェック
 */
static BOOL check_message_id(char *buf, MAILITEM *tpMailItem, TCHAR *ErrStr, MAILBOX *tpMailBox)
{
	char *content;
#ifdef UNICODE
	char *p;
#endif

	// Message-Id
	content = item_get_message_id(buf);
	if (content == NULL) {
		lstrcpy(ErrStr, STR_ERR_SOCK_NOMESSAGEID);
		return FALSE;
	}

#ifdef UNICODE
	p = NULL;
	if (tpMailItem->MessageID != NULL) {
		p = alloc_tchar_to_char(tpMailItem->MessageID);
		if (p == NULL) {
			mem_free(&content);
			lstrcpy(ErrStr, STR_ERR_MEMALLOC);
			return FALSE;
		}
	}
	if (tpMailItem->MessageID == NULL || tstrcmp(p, content) != 0) {
		mem_free(&content);
		mem_free(&p);
		lstrcpy(ErrStr, STR_ERR_SOCK_MAILSYNC);
		return FALSE;
	}
	mem_free(&p);
#else
	if (tpMailItem->MessageID == NULL || tstrcmp(tpMailItem->MessageID, content) != 0) {
		mem_free(&content);
		lstrcpy(ErrStr, STR_ERR_SOCK_MAILSYNC);
		return FALSE;
	}
#endif
	mem_free(&content);
	return TRUE;
}

/*
 * check_last_mail - 最後に受信したメールのチェック
 */
static int check_last_mail(HWND hWnd, SOCKET soc, BOOL chek_flag, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	if (chek_flag != FALSE) {
		// 前回最後に取得したメールではない
		if (disable_uidl == FALSE) {
			// UIDLで全メールの同期を取る
			receiving_uidl = FALSE;
			SetSocStatusTextT(hWnd, TEXT(CMD_UIDL)TEXT("\r\n"));
			if (send_buf(soc, CMD_UIDL"\r\n") == -1) {
				lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
				return FALSE;
			}
			return POP_UIDL_ALL;
		}
		// 1件目から取得
		init_mailbox(hWnd, tpMailBox, ShowFlag);
		list_get_no = 1;
	} else {
		// 前回最後に取得した位置から変化なしなのでそのまま取得開始
		list_get_no++;
		if (list_get_no > tpMailBox->MailCnt) {
			return POP_QUIT;
		}
	}
	if (send_command(hWnd, soc, TEXT(CMD_LIST), list_get_no, ErrStr) == FALSE) {
		return POP_ERR;
	}
	return POP_LIST;
}

/*
 * send_command - コマンドと引数(数値)を送信する
 */
static BOOL send_command(HWND hWnd, SOCKET soc, TCHAR *Command, int Cnt, TCHAR *ErrStr)
{
	TCHAR wBuf[BUF_SIZE];

	wsprintf(wBuf, TEXT("%s %d\r\n"), Command, Cnt);
	SetSocStatusTextT(hWnd, wBuf);

	if (send_buf_t(soc, wBuf) == -1) {
		lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
		return FALSE;
	}
	return TRUE;
}

/*
 * send_command_top - TOPコマンドを送信する
 */
static int send_command_top(HWND hWnd, SOCKET soc, int Cnt, TCHAR *ErrStr, int len, int ret)
{
	TCHAR wBuf[BUF_SIZE];

	wsprintf(wBuf, TEXT(CMD_TOP)TEXT(" %d %d\r\n"), Cnt, len);
	SetSocStatusTextT(hWnd, wBuf);

	if (send_buf_t(soc, wBuf) == -1) {
		lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
		return POP_ERR;
	}
	return ret;
}

/*
 * create_apop - APOPの文字列を生成する
 */
static TCHAR *create_apop(char *buf, TCHAR *ErrStr, MAILBOX *tpMailBox, TCHAR *sPass)
{
	MD5_CTX context;
	TCHAR *wbuf;
	unsigned char digest[16];
	char *hidx = NULL;
	char *tidx = NULL;
	char *pass;
	char *p;
	int len;
	int i;

	// < から > までを抽出
	for (hidx = NULL, p = buf; *p != '\0';p++) {
		if (*p == '<') {
			hidx = p;
		}
	}
	if (hidx != NULL) {
		for (tidx = NULL, p = hidx; *p != '\0';p++) {
			if (*p == '>') {
				tidx = p;
			}
		}
	}
	if (hidx == NULL || tidx == NULL) {
		lstrcpy(ErrStr, STR_ERR_SOCK_NOAPOP);
		return NULL;
	}

	pass = alloc_tchar_to_char(sPass);
	if (pass == NULL) {
		lstrcpy(ErrStr, STR_ERR_MEMALLOC);
		return NULL;
	}

	p = (char *)mem_alloc((tidx - hidx + 2) + tstrlen(pass) + 1);
	if (p == NULL) {
		mem_free(&pass);
		lstrcpy(ErrStr, STR_ERR_MEMALLOC);
		return NULL;
	}

	str_cpy_n(p, hidx, tidx - hidx + 2);
	tstrcat(p, pass);
	mem_free(&pass);

	// digest 値を取る
	MD5Init(&context);
	MD5Update(&context, p, tstrlen(p));
	MD5Final(digest, &context);

	mem_free(&p);

	wbuf = (TCHAR *)mem_alloc(
		sizeof(TCHAR) * (lstrlen(TEXT(CMD_APOP)) + 1 + lstrlen(tpMailBox->User) + 1 + (16 * 2) + 2 + 1));
	if (wbuf == NULL) {
		lstrcpy(ErrStr, STR_ERR_MEMALLOC);
		return NULL;
	}

	len = wsprintf(wbuf, TEXT(CMD_APOP)TEXT(" %s "), tpMailBox->User);
	for (i = 0; i < 16; i++, len += 2) {
		wsprintf(wbuf + len, TEXT("%02x"), digest[i]);
	}
	lstrcat(wbuf, TEXT("\r\n"));
	return wbuf;
}

/*
 * login_proc - ログインの処理を行う
 */
static int login_proc(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox)
{
	TCHAR *wbuf;
	static TCHAR *pass;
	int ret = POP_ERR;
	BOOL PopSTARTTLS = FALSE;

	if (op.SocLog == 1) SetSocStatusText(hWnd, buf);

	switch (command_status) {
	case POP_STARTTLS:
		// レスポンスの解析
		if (check_response(buf) == FALSE) {
			lstrcpy(ErrStr, STR_ERR_SOCK_RESPONSE);
			str_cat_n(ErrStr, buf, BUF_SIZE - 1);
			return POP_ERR;
		}
		ssl = ssl_init();
		if (init_ssl(hWnd, soc, ErrStr) == -1) {
			return POP_ERR;
		}
		PopSTARTTLS = TRUE;

	case POP_START:
		// フラグを初期化
		disable_top = FALSE;
		disable_uidl = (tpMailBox->NoUIDL == 0) ? FALSE : TRUE;
		// レスポンスの解析
		if (check_response(buf) == FALSE) {
			lstrcpy(ErrStr, STR_ERR_SOCK_RESPONSE);
			str_cat_n(ErrStr, buf, BUF_SIZE - 1);
			return POP_ERR;
		}
		if (tpMailBox->User == NULL || *tpMailBox->User == TEXT('\0')) {
			lstrcpy(ErrStr, STR_ERR_SOCK_NOUSERID);
			return POP_ERR;
		}

		// STARTTLS
		if (PopSTARTTLS == FALSE && tpMailBox->PopSSL == 1 && tpMailBox->PopSSLInfo.Type == 4) {
			SetSocStatusTextT(hWnd, TEXT(CMD_STLS));
			if (send_buf(soc, CMD_STLS"\r\n") == -1) {
				lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
				return POP_ERR;
			}
			return POP_STARTTLS;
		}

		pass = tpMailBox->Pass;
		if (pass == NULL || *pass == TEXT('\0')) {
			pass = tpMailBox->TmpPass;
		}
		if (pass == NULL || *pass == TEXT('\0')) {
			pass = g_Pass;
		}
		if (pass == NULL || *pass == TEXT('\0')) {
			lstrcpy(ErrStr, STR_ERR_SOCK_NOPASSWORD);
			return POP_ERR;
		}
		// APOPによる認証
		if (tpMailBox->APOP == 1) {
			wbuf = create_apop(buf, ErrStr, tpMailBox, pass);
			if (wbuf == NULL) {
				return POP_ERR;
			}

			SetSocStatusTextT(hWnd, TEXT(CMD_APOP)TEXT(" ****"));

			if (send_buf_t(soc, wbuf) == -1) {
				mem_free(&wbuf);
				lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
				return POP_ERR;
			}
			mem_free(&wbuf);
			ret = POP_PASS;
			break;
		}
		// USER の送信
		wbuf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(TEXT(CMD_USER)) + 1 + lstrlen(tpMailBox->User) + 2 + 1));
		if (wbuf == NULL) {
			lstrcpy(ErrStr, STR_ERR_MEMALLOC);
			return POP_ERR;
		}
		str_join_t(wbuf, TEXT(CMD_USER), TEXT(" "), tpMailBox->User, TEXT("\r\n"), (TCHAR *)-1);
		SetSocStatusTextT(hWnd, wbuf);
		if (send_buf_t(soc, wbuf) == -1) {
			mem_free(&wbuf);
			lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
			return POP_ERR;
		}
		mem_free(&wbuf);

		ret = POP_USER;
		break;

	case POP_USER:
		// レスポンスの解析
		if (check_response(buf) == FALSE) {
			lstrcpy(ErrStr, STR_ERR_SOCK_ACCOUNT);
			str_cat_n(ErrStr, buf, BUF_SIZE - 1);
			return POP_ERR;
		}
		// PASS の送信
		wbuf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(TEXT(CMD_PASS)) + 1 + lstrlen(pass) + 2 + 1));
		if (wbuf == NULL) {
			lstrcpy(ErrStr, STR_ERR_MEMALLOC);
			return POP_ERR;
		}
		str_join_t(wbuf, TEXT(CMD_PASS), TEXT(" "), pass, TEXT("\r\n"), (TCHAR *)-1);
		SetSocStatusTextT(hWnd, TEXT(CMD_PASS)TEXT(" ****"));
		if (send_buf_t(soc, wbuf) == -1) {
			mem_free(&wbuf);
			lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
			return POP_ERR;
		}
		mem_free(&wbuf);

		ret = POP_PASS;
		break;

	case POP_PASS:
		// レスポンスの解析
		if (check_response(buf) == FALSE) {
			lstrcpy(ErrStr, STR_ERR_SOCK_BADPASSWORD);
			str_cat_n(ErrStr, buf, BUF_SIZE - 1);
			return POP_ERR;
		}
		ret = POP_LOGIN;
		break;
	}
	return ret;
}

/*
 * list_proc_stat - STATのレスポンスの解析
 */
static int list_proc_stat(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	MAILITEM *tpMailItem = NULL;
	char *p, *r, *t;
	int get_no;
	int ret;

	if (op.SocLog == 1) SetSocStatusText(hWnd, buf);
	// 前コマンド結果に'.'が付いている場合はスキップする
	if (*buf == '.' && *(buf + 1) == '\0') {
		return POP_STAT;
	}
	// レスポンスの解析
	if (check_response(buf) == FALSE) {
		lstrcpy(ErrStr, STR_ERR_SOCK_STAT);
		str_cat_n(ErrStr, buf, BUF_SIZE - 1);
		return POP_ERR;
	}

	// メール数の取得
	p = buf;
	t = NULL;
	for (; *p != ' ' && *p != '\0'; p++);
	for (; *p == ' '; p++);
	for (r = p; *r != '\0'; r++) {
		if (*r == ' ') {
			t = r + 1;
			*r = '\0';
			break;
		}
	}
	tpMailBox->MailCnt = a2i(p);
	if (t != NULL) {
		tpMailBox->MailSize = a2i(t);
	}
	if (tpMailBox->MailCnt == 0 || tpMailBox->LastNo == -1) {
		// メールボックスの初期化
		init_mailbox(hWnd, tpMailBox, ShowFlag);
		if (tpMailBox->MailCnt == 0) {
			SetItemCntStatusText(hWnd, tpMailBox);
			return POP_QUIT;
		}
	}
	SetItemCntStatusText(hWnd, tpMailBox);

	init_recv = FALSE;
	mail_received = FALSE;
	disable_top = (op.ListDownload != 0) ? TRUE : FALSE;

	// UIDLの初期化
	if (uidl_init(tpMailBox->MailCnt) == FALSE) {
		lstrcpy(ErrStr, STR_ERR_MEMALLOC);
		return POP_ERR;
	}
	if (tpMailBox->LastNo == 0 || tpMailBox->LastMessageId == NULL) {
		// 指定番目から取得
		if (disable_uidl == FALSE) {
			receiving_uidl = FALSE;
			SetSocStatusTextT(hWnd, TEXT(CMD_UIDL)TEXT("\r\n"));
			if (send_buf(soc, CMD_UIDL"\r\n") == -1) {
				lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
				return POP_ERR;
			}
			ret = POP_UIDL_ALL;
		} else {
			list_get_no = (tpMailBox->LastNo == 0) ? 1 : tpMailBox->LastNo;
			init_recv = TRUE;
			if (send_command(hWnd, soc, TEXT(CMD_LIST), list_get_no, ErrStr) == FALSE) {
				return POP_ERR;
			}
			ret = POP_LIST;
		}
	} else if (tpMailBox->LastNo > tpMailBox->MailCnt) {
		// 前回最後に取得したメール数より少ない
		if (disable_uidl == FALSE) {
			receiving_uidl = FALSE;
			SetSocStatusTextT(hWnd, TEXT(CMD_UIDL)TEXT("\r\n"));
			if (send_buf(soc, CMD_UIDL"\r\n") == -1) {
				lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
				return POP_ERR;
			}
			ret = POP_UIDL_ALL;
		} else {
			// 1件目から取得
			init_mailbox(hWnd, tpMailBox, ShowFlag);
			list_get_no = 1;
			if (send_command(hWnd, soc, TEXT(CMD_LIST), list_get_no, ErrStr) == FALSE) {
				return POP_ERR;
			}
			ret = POP_LIST;
		}
	} else {
		// 前回最後に取得したメールの位置が変わっていないかチェック
		list_get_no = tpMailBox->LastNo;
		get_no = item_get_number_to_index(tpMailBox, list_get_no);
		if (get_no != -1) {
			tpMailItem = *(tpMailBox->tpMailItem + get_no);
		}
		if (disable_uidl == FALSE && tpMailItem != NULL && tpMailItem->UIDL != NULL) {
			if (send_command(hWnd, soc, TEXT(CMD_UIDL), list_get_no, ErrStr) == FALSE) {
				return POP_ERR;
			}
			return POP_UIDL_CHECK;
		} else {
			receiving_data = FALSE;
			if (mail_buf_init(MAIL_BUF_SIZE) == FALSE) {
				lstrcpy(ErrStr, STR_ERR_MEMALLOC);
				return POP_ERR;
			}
			ret = send_command_top(hWnd, soc, list_get_no, ErrStr, 0, POP_TOP);
		}
	}
	return ret;
}

/*
 * list_proc_uidl_all - UIDLのレスポンスの解析
 */
static int list_proc_uidl_all(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	MAILITEM *tpMailItem;
	MAILITEM *tpLastMailItem;
	HWND hListView;
	int No;
	int i;

	// UIDLレスポンスの1行目
	if (receiving_uidl == FALSE) {
		SetSocStatusText(hWnd, buf);
		// 前コマンド結果に'.'が付いている場合はスキップする
		if (*buf == '.' && *(buf + 1) == '\0') {
			return POP_UIDL_ALL;
		}
		// レスポンスの解析
		if (check_response(buf) == TRUE) {
			receiving_uidl = TRUE;
			return POP_UIDL_ALL;
		}
		// UIDL未サポート
		disable_uidl = TRUE;
		SetSocStatusTextT(hWnd, TEXT(CMD_STAT)TEXT("\r\n"));
		if (send_buf(soc, CMD_STAT"\r\n") == -1) {
			lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
			return POP_ERR;
		}
		return POP_STAT;
	}
	// UIDLの終わりではない場合
	if (*buf != '.' || *(buf + 1) != '\0') {
		// 受信文字列を保存する
		if (uidl_set(buf, buflen) == FALSE) {
			uidl_free();
			lstrcpy(ErrStr, STR_ERR_MEMALLOC);
			return POP_ERR;
		}
		return POP_UIDL_ALL;
	}

	if (tpMailBox->LastNo == 0 || tpMailBox->LastMessageId == NULL) {
		// 指定番目から取得
		list_get_no = (tpMailBox->LastNo == 0) ? 1 : tpMailBox->LastNo;
		init_recv = TRUE;
		if (send_command(hWnd, soc, TEXT(CMD_LIST), list_get_no, ErrStr) == FALSE) {
			return POP_ERR;
		}
		return POP_LIST;
	}

	hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
	if (ShowFlag == TRUE) {
		ListView_SetRedraw(hListView, FALSE);
	}
	SwitchCursor(FALSE);
	// 現在表示されているメール一覧とUIDLを比較して番号を振り直す
	tpLastMailItem = NULL;
	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		tpMailItem = *(tpMailBox->tpMailItem + i);
		if (tpMailItem == NULL) {
			continue;
		}
		switch ((No = uidl_check(tpMailItem->UIDL))) {
#ifdef UNICODE
		case -1:
			uidl_free();
			lstrcpy(ErrStr, STR_ERR_MEMALLOC);
			if (ShowFlag == TRUE) {
				ListView_SetRedraw(hListView, TRUE);
			}
			SwitchCursor(TRUE);
			return POP_ERR;
#endif
		case 0:
			// UIDLのリストに存在しないメールは解放する
			if (ShowFlag == TRUE) {
				No = ListView_GetMemToItem(hListView, tpMailItem);
				ListView_DeleteItem(hListView, No);
			}
			item_free((tpMailBox->tpMailItem + i), 1);
			break;

		default:
			// メール番号を設定
			tpMailItem->No = No;
			if (tpLastMailItem == NULL || tpLastMailItem->No < tpMailItem->No) {
				tpLastMailItem = tpMailItem;
			}
			break;
		}
	}
	// 削除されたメールを一覧から消す
	item_resize_mailbox(tpMailBox);
	if (ShowFlag == TRUE) {
		ListView_SetRedraw(hListView, TRUE);
	}
	SwitchCursor(TRUE);
	SetItemCntStatusText(hWnd, tpMailBox);

	// 最後に受信したメールのメッセージIDを保存する
	mem_free(&tpMailBox->LastMessageId);
	tpMailBox->LastMessageId = NULL;
	tpMailBox->LastNo = 0;
	if (tpLastMailItem != NULL) {
		tpMailBox->LastMessageId = alloc_tchar_to_char(tpLastMailItem->MessageID);
		tpMailBox->LastNo = tpLastMailItem->No;
	}
	list_get_no = tpMailBox->LastNo + 1;
	if (list_get_no > tpMailBox->MailCnt) {
		return POP_QUIT;
	}
	if (send_command(hWnd, soc, TEXT(CMD_LIST), list_get_no, ErrStr) == FALSE) {
		return POP_ERR;
	}
	return POP_LIST;
}

/*
 * list_proc_uidl_check - UIDLのレスポンスの解析
 */
static int list_proc_uidl_check(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	MAILITEM *tpMailItem = NULL;
	TCHAR *UIDL = NULL;
	int get_no;
	int ret;

	if (op.SocLog == 1) SetSocStatusText(hWnd, buf);
	// 前コマンド結果に'.'が付いている場合はスキップする
	if (*buf == '.' && *(buf + 1) == '\0') {
		return POP_UIDL_CHECK;
	}
	// レスポンスの解析
	if (check_response(buf) == TRUE) {
		UIDL = alloc_char_to_tchar(skip_response(buf));
	} else {
		// UIDL未サポート
		disable_uidl = TRUE;
		receiving_data = FALSE;
		if (mail_buf_init(MAIL_BUF_SIZE) == FALSE) {
			lstrcpy(ErrStr, STR_ERR_MEMALLOC);
			return POP_ERR;
		}
		return send_command_top(hWnd, soc, list_get_no, ErrStr, 0, POP_TOP);
	}
	get_no = item_get_number_to_index(tpMailBox, list_get_no);
	if (get_no != -1) {
		tpMailItem = *(tpMailBox->tpMailItem + get_no);
	}
	// UIDLのチェック
	ret = check_last_mail(hWnd, soc,
		(tpMailItem ==NULL || tpMailItem->UIDL == NULL || UIDL == NULL || lstrcmp(tpMailItem->UIDL, UIDL) != 0),
		ErrStr, tpMailBox, ShowFlag);
	mem_free(&UIDL);
	return ret;
}

/*
 * list_proc_uidl_set - UIDLのレスポンスの解析
 */
static int list_proc_uidl_set(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	if (op.SocLog == 1) SetSocStatusText(hWnd, buf);
	// 前コマンド結果に'.'が付いている場合はスキップする
	if (*buf == '.' && *(buf + 1) == '\0') {
		return POP_UIDL;
	}
	// レスポンスの解析
	if (check_response(buf) == TRUE) {
		uidl_item->UIDL = alloc_char_to_tchar(skip_response(buf));
	} else {
		disable_uidl = TRUE;
	}
	// 次のメールを取得
	list_get_no++;
	if (list_get_no > tpMailBox->MailCnt) {
		return POP_QUIT;
	}
	if (send_command(hWnd, soc, TEXT(CMD_LIST), list_get_no, ErrStr) == FALSE) {
		return POP_ERR;
	}
	return POP_LIST;
}

/*
 * list_proc_list - LISTのレスポンスの解析
 */
static int list_proc_list(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	char *p, *r;
	int len = 0;

	if (op.SocLog == 1) SetSocStatusText(hWnd, buf);
	// 前コマンド結果に'.'が付いている場合はスキップする
	if (*buf == '.' && *(buf + 1) == '\0') {
		return POP_LIST;
	}
	// レスポンスの解析
	mail_size = -1;
	if (check_response(buf) == TRUE) {
		p = skip_response(buf);
		for (r = p; *r != ' ' && *r != '\0'; r++);
		mail_size = len = a2i(p);
		if (disable_top == FALSE) {
			// 受信バッファの初期サイズ
			len = (len > 0 && len < (op.ListGetLine + HEAD_LINE) * LINE_LEN)
				? len : ((op.ListGetLine + HEAD_LINE) * LINE_LEN);
		}
	}
	receiving_data = FALSE;
	if (mail_buf_init((len > 0) ? len : MAIL_BUF_SIZE) == FALSE) {
		lstrcpy(ErrStr, STR_ERR_MEMALLOC);
		return POP_ERR;
	}
	if (disable_top == TRUE) {
		// 一覧取得時に本文をすべてダウンロード
		if (tpMailBox->NoRETR == 1) {
			return send_command_top(hWnd, soc, list_get_no, ErrStr, (len > 0) ? len : DOWNLOAD_SIZE, POP_RETR);
		}
		if (send_command(hWnd, soc, TEXT(CMD_RETR), list_get_no, ErrStr) == FALSE) {
			return POP_ERR;
		}
		return POP_RETR;
	}
	return send_command_top(hWnd, soc, list_get_no, ErrStr, op.ListGetLine, POP_TOP);
}

/*
 * list_proc_top - TOPのレスポンスの解析
 */
static int list_proc_top(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	MAILITEM *tpMailItem;
	LV_ITEM lvi;
	HWND hListView;
	TCHAR *p;
	int i;
	int st;

	// TOPレスポンスの1行目
	if (receiving_data == FALSE) {
		SetSocStatusText(hWnd, buf);
		// 前コマンド結果に'.'が付いている場合はスキップする
		if (*buf == '.' && *(buf + 1) == '\0') {
			return POP_TOP;
		}
		// レスポンスの解析
		if (check_response(buf) == TRUE) {
			receiving_data = TRUE;
			return POP_TOP;
		}
		if (disable_top == FALSE && tpMailBox->NoRETR == 0) {
			// TOPが未サポートの可能性があるのでRETRを送信する
			disable_top = TRUE;
			if (send_command(hWnd, soc, TEXT(CMD_RETR), list_get_no, ErrStr) == FALSE) {
				return POP_ERR;
			}
			return POP_RETR;
		}
		lstrcpy(ErrStr, (disable_top == FALSE) ? STR_ERR_SOCK_TOP : STR_ERR_SOCK_RETR);
		str_cat_n(ErrStr, buf, BUF_SIZE - 1);
		return POP_ERR;
	}
	// TOPの終わりではない場合
	if (*buf != '.' || *(buf + 1) != '\0') {
		// 受信データを保存する
		if (mail_buf_set(buf, buflen) == FALSE) {
			lstrcpy(ErrStr, STR_ERR_MEMALLOC);
			return POP_ERR;
		}
		return POP_TOP;
	}

	if (list_get_no == tpMailBox->LastNo) {
		if (init_recv == TRUE) {
			// 新着取得位置が初期化されたためメールボックスを初期化
			init_mailbox(hWnd, tpMailBox, ShowFlag);
		} else {
			// 前回最後に受信したメールのMessage-IDをチェック
			char *content = item_get_message_id(mail_buf);
			i = check_last_mail(hWnd, soc,
				(tpMailBox->LastMessageId == NULL || content == NULL || tstrcmp(tpMailBox->LastMessageId, content) != 0),
				ErrStr, tpMailBox, ShowFlag);
			mem_free(&content);
			return i;
		}
	}

	// 受信の最大アイテム数分のメモリを確保
	if (mail_received == FALSE) {
		if (ShowFlag == TRUE) {
			ListView_SetItemCount(GetDlgItem(hWnd, IDC_LISTVIEW), tpMailBox->MailCnt);
		}
		if (item_set_count(tpMailBox, tpMailBox->MailCnt) == FALSE) {
			lstrcpy(ErrStr, STR_ERR_MEMALLOC);
			return POP_ERR;
		}
	}
	// ヘッダからアイテムを作成
	tpMailItem = item_header_to_item(tpMailBox, mail_buf, mail_size);
	if (tpMailItem == NULL) {
		lstrcpy(ErrStr, STR_ERR_MEMALLOC);
		return POP_ERR;
	}
	if ((int)tpMailItem != -1 && tpMailItem->Body == NULL && disable_top == TRUE) {
		tpMailItem->Body = (char *)mem_alloc(sizeof(char));
		if (tpMailItem->Body != NULL) {
			*tpMailItem->Body = '\0';
			tpMailItem->Status = tpMailItem->MailStatus = ICON_MAIL;
		}
	}

	if ((int)tpMailItem != -1) {
		// 新着フラグの除去
		if (mail_received == FALSE && NewMail_Flag == FALSE && ShowMsgFlag == FALSE) {
			for (i = 0; i < tpMailBox->MailItemCnt; i++) {
				if (*(tpMailBox->tpMailItem + i) == NULL) {
					continue;
				}
				(*(tpMailBox->tpMailItem + i))->New = FALSE;
			}
		}
		tpMailItem->New = TRUE;
		tpMailItem->Download = disable_top;
		tpMailItem->No = list_get_no;

		if (ShowFlag == TRUE) {
			hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
			// 新着のオーバレイマスク
			st = INDEXTOOVERLAYMASK(1);
			st |= ((tpMailItem->Multipart == TRUE) ? INDEXTOSTATEIMAGEMASK(1) : 0);
			if (mail_received == FALSE && NewMail_Flag == FALSE && ShowMsgFlag == FALSE) {
				// 全アイテムの新着のオーバーレイマスクを解除
				ListView_SetItemState(hListView, -1, 0, LVIS_OVERLAYMASK);
				ListView_RedrawItems(hListView, 0, ListView_GetItemCount(hListView));
				// 新着位置の選択
				ListView_SetItemState(hListView, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
				st |= (LVIS_FOCUSED | LVIS_SELECTED);
			}
			st |= ((tpMailItem->Download == FALSE && tpMailItem->Status != ICON_DOWN && tpMailItem->Status != ICON_DEL)
				? LVIS_CUT : 0);
			lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
			lvi.iItem = ListView_GetItemCount(hListView);
			lvi.iSubItem = 0;
			lvi.state = st;
			lvi.stateMask = LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK | LVIS_CUT | LVIS_FOCUSED | LVIS_SELECTED;
			lvi.pszText = LPSTR_TEXTCALLBACK;
			lvi.cchTextMax = 0;
			lvi.iImage = I_IMAGECALLBACK;
			lvi.lParam = (long)tpMailItem;
			// リストビューにアイテムを追加
			i = ListView_InsertItem(hListView, &lvi);
			if (mail_received == FALSE) {
				ListView_EnsureVisible(hListView, i, TRUE);
			}
			// 一行下へスクロール
			if (op.RecvScroll == 1) {
				SendMessage(hListView, WM_VSCROLL, SB_LINEDOWN, 0);
			}
			SetItemCntStatusText(hWnd, tpMailBox);
			EndThreadSortFlag = TRUE;
		}
		mail_received = TRUE;

		// 新着カウント
		NewMailCnt++;
		tpMailBox->NewMail = TRUE;
	}

	// 最後に受信したメールの番号とメッセージIDを保存する
	tpMailBox->LastNo = list_get_no;
	mem_free(&tpMailBox->LastMessageId);
	tpMailBox->LastMessageId = item_get_message_id(mail_buf);
	if (tpMailBox->LastMessageId == NULL) {
		lstrcpy(ErrStr, STR_ERR_SOCK_NOMESSAGEID);
		return POP_ERR;
	}

	if ((int)tpMailItem != -1 && disable_uidl == FALSE) {
		// メールアイテムにUIDLを設定
		if ((p = uidl_get(list_get_no)) != NULL) {
			tpMailItem->UIDL = alloc_copy_t(p);
		} else {
			uidl_item = tpMailItem;
			if (send_command(hWnd, soc, TEXT(CMD_UIDL), list_get_no, ErrStr) == FALSE) {
				return POP_ERR;
			}
			return POP_UIDL;
		}
	}

	// 次のメールを取得
	list_get_no++;
	if (list_get_no > tpMailBox->MailCnt) {
		return POP_QUIT;
	}
	if (send_command(hWnd, soc, TEXT(CMD_LIST), list_get_no, ErrStr) == FALSE) {
		return POP_ERR;
	}
	return POP_LIST;
}

/*
 * exec_send_check_command - 削除確認用コマンドを送信
 */
static int exec_send_check_command(HWND hWnd, SOCKET soc, int get_no, TCHAR *ErrStr, MAILBOX *tpMailBox)
{
	MAILITEM *tpMailItem = NULL;

	if (get_no != -1) {
		tpMailItem = *(tpMailBox->tpMailItem + get_no);
	}
	if (disable_uidl == TRUE || tpMailItem == NULL || tpMailItem->UIDL == NULL) {
		receiving_data = FALSE;
		if (mail_buf_init(MAIL_BUF_SIZE) == FALSE) {
			lstrcpy(ErrStr, STR_ERR_MEMALLOC);
			return POP_ERR;
		}
		if (disable_top == TRUE && tpMailBox->NoRETR == 0) {
			// RETRを送信
			if (send_command(hWnd, soc, TEXT(CMD_RETR), delete_get_no, ErrStr) == FALSE) {
				return POP_ERR;
			}
			return POP_TOP;
		}
		// TOPを送信
		return send_command_top(hWnd, soc, delete_get_no, ErrStr, 0, POP_TOP);
	}
	// UIDLを送信
	if (send_command(hWnd, soc, TEXT(CMD_UIDL), delete_get_no, ErrStr) == FALSE) {
		return POP_ERR;
	}
	return POP_UIDL;
}

/*
 * exec_proc_init - 実行処理の初期化
 */
static int exec_proc_init(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	MAILITEM *tpMailItem;
	int size;
	int get_no;

	get_no = item_get_next_download_mark(tpMailBox, -1, &download_get_no);
	if (get_no == -1) {
		// 削除
		get_no = item_get_next_delete_mark(tpMailBox, -1, &delete_get_no);
		if (get_no == -1) {
			return POP_QUIT;
		}
		// 削除メールの確認コマンド(UIDL)を送信
		return exec_send_check_command(hWnd, soc, get_no, ErrStr, tpMailBox);
	}

	// ダウンロード
	tpMailItem = *(tpMailBox->tpMailItem + get_no);
	if (tpMailItem == NULL) {
		lstrcpy(ErrStr, STR_ERR_SOCK_GETITEMINFO);
		return POP_ERR;
	}
	size = (tpMailItem->Size == NULL) ? 0 : _ttoi(tpMailItem->Size);
	receiving_data = FALSE;
	if (mail_buf_init((size > 0) ? size : MAIL_BUF_SIZE) == FALSE) {
		lstrcpy(ErrStr, STR_ERR_MEMALLOC);
		return POP_ERR;
	}
	// 全文受信するコマンドを送信
	if (tpMailBox->NoRETR == 1) {
		return send_command_top(hWnd, soc, download_get_no, ErrStr, (size > 0) ? size : DOWNLOAD_SIZE, POP_RETR);
	}
	if (send_command(hWnd, soc, TEXT(CMD_RETR), download_get_no, ErrStr) == FALSE) {
		return POP_ERR;
	}
	return POP_RETR;
}

/*
 * exec_proc_retr - RETRのレスポンスの解析
 */
static int exec_proc_retr(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	MAILITEM *tpMailItem;
	HWND hListView;
	int i, size;
	int get_no;
	static int recvlen;
	static int recvcnt;

	if (receiving_data == FALSE) {
		// 前コマンド結果に'.'が付いている場合はスキップする
		if (*buf == '.' && *(buf + 1) == '\0') {
			return POP_RETR;
		}
		// レスポンスの解析
		if (check_response(buf) == FALSE) {
			lstrcpy(ErrStr, (tpMailBox->NoRETR == 1) ? STR_ERR_SOCK_TOP : STR_ERR_SOCK_RETR);
			str_cat_n(ErrStr, buf, BUF_SIZE - 1);
			return POP_ERR;
		}
		recvlen = 0;
		recvcnt = REDRAWCNT;
		SetSocStatusTextT(hWnd, STR_STATUS_RECV);
		receiving_data = TRUE;
		return POP_RETR;
	}

	// RETRの終わりではない場合
	if (*buf != '.' || *(buf + 1) != '\0') {
		// 受信データを保存する
		if (mail_buf_set(buf, buflen) == FALSE) {
			lstrcpy(ErrStr, STR_ERR_MEMALLOC);
			return POP_ERR;
		}
		recvlen += buflen;
		recvcnt++;
		if (recvcnt > REDRAWCNT) {
			recvcnt = 0;
			SetStatusRecvLen(hWnd, recvlen, STR_STATUS_SOCKINFO_RECV);
		}
		return POP_RETR;
	}

	get_no = item_get_number_to_index(tpMailBox, download_get_no);
	if (get_no == -1) {
		lstrcpy(ErrStr, STR_ERR_SOCK_MAILSYNC);
		return POP_ERR;
	}
	tpMailItem = *(tpMailBox->tpMailItem + get_no);
	if (tpMailItem == NULL) {
		lstrcpy(ErrStr, STR_ERR_SOCK_GETITEMINFO);
		return POP_ERR;
	}
	// メッセージIDで要求したメールかどうかチェックする
	if (check_message_id(mail_buf, tpMailItem, ErrStr, tpMailBox) == FALSE) {
		return POP_ERR;
	}

	// 本文を取得
	item_mail_to_item(tpMailItem, mail_buf, -1, TRUE);
	tpMailItem->Download = TRUE;

	if (ShowFlag == TRUE) {
		hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
		// リストビューの更新
		i = ListView_GetMemToItem(hListView, tpMailItem);
		if (i != -1) {
			ListView_SetItemState(hListView, i, 0, LVIS_CUT);
			ListView_RedrawItems(hListView, i, i);
			UpdateWindow(hListView);
			SetItemCntStatusText(hWnd, tpMailBox);
		}
	}
	if (hViewWnd != NULL) {
		SendMessage(hViewWnd, WM_CHANGE_MARK, 0, 0);
	}

	get_no = item_get_next_download_mark(tpMailBox, -1, &download_get_no);
	if (get_no == -1) {
		get_no = item_get_next_delete_mark(tpMailBox, -1, &delete_get_no);
		if (get_no == -1) {
			return POP_QUIT;
		}
		// 削除メールの確認コマンド(UIDL)を送信
		return exec_send_check_command(hWnd, soc, get_no, ErrStr, tpMailBox);
	}
	tpMailItem = *(tpMailBox->tpMailItem + get_no);
	if (tpMailItem == NULL) {
		lstrcpy(ErrStr, STR_ERR_SOCK_GETITEMINFO);
		return POP_ERR;
	}
	size = (tpMailItem->Size == NULL) ? 0 : _ttoi(tpMailItem->Size);

	// 次のヘッダの取得
	receiving_data = FALSE;
	if (mail_buf_init((size > 0) ? size : MAIL_BUF_SIZE) == FALSE) {
		lstrcpy(ErrStr, STR_ERR_MEMALLOC);
		return POP_ERR;
	}
	// 全文受信するコマンドを送信
	if (tpMailBox->NoRETR == 1) {
		return send_command_top(hWnd, soc, download_get_no, ErrStr, (size > 0) ? size : DOWNLOAD_SIZE, POP_RETR);
	}
	if (send_command(hWnd, soc, TEXT(CMD_RETR), download_get_no, ErrStr) == FALSE) {
		return POP_ERR;
	}
	return POP_RETR;
}

/*
 * exec_proc_uidl - 削除メール確認用UIDLのレスポンスの解析
 */
static int exec_proc_uidl(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	MAILITEM *tpMailItem = NULL;
	TCHAR *UIDL = NULL;
	int get_no;

	if (op.SocLog == 1) SetSocStatusText(hWnd, buf);
	// 前コマンド結果に'.'が付いている場合はスキップする
	if (*buf == '.' && *(buf + 1) == '\0') {
		return POP_UIDL;
	}
	// レスポンスの解析
	if (check_response(buf) == TRUE) {
		UIDL = alloc_char_to_tchar(skip_response(buf));
	} else {
		// UIDL未サポート
		disable_uidl = TRUE;
		// 削除メールの確認コマンド(TOP)を送信
		return exec_send_check_command(hWnd, soc, -1, ErrStr, tpMailBox);
	}
	get_no = item_get_number_to_index(tpMailBox, delete_get_no);
	if (get_no != -1) {
		tpMailItem = *(tpMailBox->tpMailItem + get_no);
	}
	if (UIDL == NULL || tpMailItem == NULL || tpMailItem->UIDL == NULL) {
		mem_free(&UIDL);
		// 削除メールの確認コマンド(TOP)を送信
		return exec_send_check_command(hWnd, soc, -1, ErrStr, tpMailBox);
	}
	// メッセージIDで要求したメールかどうかチェックする
	if (lstrcmp(tpMailItem->UIDL, UIDL) != 0) {
		mem_free(&UIDL);
		lstrcpy(ErrStr, STR_ERR_SOCK_MAILSYNC);
		return POP_ERR;
	}
	mem_free(&UIDL);
	// 削除コマンドの送信
	if (send_command(hWnd, soc, TEXT(CMD_DELE), delete_get_no, ErrStr) == FALSE) {
		return POP_ERR;
	}
	return POP_DELE;
}

/*
 * exec_proc_top - 削除メール確認用TOPのレスポンスの解析
 */
static int exec_proc_top(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	MAILITEM *tpMailItem;
	int get_no;

	if (receiving_data == FALSE) {
		SetSocStatusText(hWnd, buf);
		// 前コマンド結果に'.'が付いている場合はスキップする
		if (*buf == '.' && *(buf + 1) == '\0') {
			return POP_TOP;
		}
		// レスポンスの解析
		if (check_response(buf) == FALSE) {
			// 削除確認のTOPとRETRが両方失敗した場合はエラー
			if (disable_top == TRUE || tpMailBox->NoRETR == 1) {
				lstrcpy(ErrStr, (tpMailBox->NoRETR == 1) ? STR_ERR_SOCK_TOP : STR_ERR_SOCK_RETR);
				str_cat_n(ErrStr, buf, BUF_SIZE - 1);
				return POP_ERR;
			}
			// 削除確認のTOPで失敗したためRETRで削除確認を行う
			disable_top = TRUE;
			// 削除メールの確認コマンド(RETR)を送信
			return exec_send_check_command(hWnd, soc, -1, ErrStr, tpMailBox);
		}
		receiving_data = TRUE;
		return POP_TOP;
	}

	// TOPの終わりではない場合
	if (*buf != '.' || *(buf + 1) != '\0') {
		// 受信ヘッダを保存する
		if (mail_buf_set(buf, buflen) == FALSE) {
			lstrcpy(ErrStr, STR_ERR_MEMALLOC);
			return POP_ERR;
		}
		return POP_TOP;
	}

	get_no = item_get_number_to_index(tpMailBox, delete_get_no);
	if (get_no == -1) {
		lstrcpy(ErrStr, STR_ERR_SOCK_MAILSYNC);
		return POP_ERR;
	}
	tpMailItem = *(tpMailBox->tpMailItem + get_no);
	if (tpMailItem == NULL) {
		lstrcpy(ErrStr, STR_ERR_SOCK_GETITEMINFO);
		return POP_ERR;
	}
	// メッセージIDで要求したメールかどうかチェックする
	if (check_message_id(mail_buf, tpMailItem, ErrStr, tpMailBox) == FALSE) {
		return POP_ERR;
	}
	// 削除コマンドの送信
	if (send_command(hWnd, soc, TEXT(CMD_DELE), delete_get_no, ErrStr) == FALSE) {
		return POP_ERR;
	}
	return POP_DELE;
}

/*
 * exec_proc_dele - DELEのレスポンスの解析
 */
static int exec_proc_dele(HWND hWnd, SOCKET soc, char *buf, int buflen, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	MAILITEM *tpMailItem;
	HWND hListView;
	int i, j;
	int get_no;

	if (op.SocLog == 1) SetSocStatusText(hWnd, buf);
	// 前コマンド結果に'.'が付いている場合はスキップする
	if (*buf == '.' && *(buf + 1) == '\0') {
		return POP_DELE;
	}
	// レスポンスの解析
	if (check_response(buf) == FALSE) {
		lstrcpy(ErrStr, STR_ERR_SOCK_DELE);
		str_cat_n(ErrStr, buf, BUF_SIZE - 1);
		send_buf(soc, CMD_RSET"\r\n");
		return POP_ERR;
	}

	get_no = item_get_number_to_index(tpMailBox, delete_get_no);
	if (get_no == -1) {
		lstrcpy(ErrStr, STR_ERR_SOCK_MAILSYNC);
		return POP_ERR;
	}
	get_no = item_get_next_delete_mark(tpMailBox, get_no, &delete_get_no);
	if (get_no != -1) {
		// 削除メールの確認コマンドを送信
		return exec_send_check_command(hWnd, soc, get_no, ErrStr, tpMailBox);
	}

	// 削除処理完了
	if (ShowFlag == TRUE) {
		// リストビューから削除
		hListView = GetDlgItem(hWnd, IDC_LISTVIEW);
		ListView_SetRedraw(hListView, FALSE);
		while ((get_no =  ListView_GetNextDeleteItem(hListView, -1)) != -1) {
			ListView_DeleteItem(hListView, get_no);
		}
		ListView_SetRedraw(hListView, TRUE);
	}
	// メールアイテムの解放
	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		tpMailItem = *(tpMailBox->tpMailItem + i);
		if (tpMailItem == NULL || tpMailItem->Status != ICON_DEL) {
			continue;
		}
		item_free((tpMailBox->tpMailItem + i), 1);

		// 削除したメールより後ろのメールの番号を減らす
		for (j = i + 1; j < tpMailBox->MailItemCnt; j++) {
			tpMailItem = *(tpMailBox->tpMailItem + j);
			if (tpMailItem == NULL) {
				continue;
			}
			tpMailItem->No--;
		}
		tpMailBox->MailCnt--;
		tpMailBox->LastNo--;
	}
	// 最後のメッセージが削除されている場合は一覧取得用メッセージIDを変更する
	if (*(tpMailBox->tpMailItem + tpMailBox->MailItemCnt - 1) == NULL) {
		tpMailItem = NULL;
		for (i = 0; i < tpMailBox->MailItemCnt; i++) {
			if (*(tpMailBox->tpMailItem + i) == NULL) {
				continue;
			}
			if (tpMailItem == NULL || tpMailItem->No < (*(tpMailBox->tpMailItem + i))->No) {
				tpMailItem = *(tpMailBox->tpMailItem + i);
			}
		}
		if (tpMailItem != NULL) {
			mem_free(&tpMailBox->LastMessageId);
			tpMailBox->LastMessageId = alloc_tchar_to_char(tpMailItem->MessageID);
			tpMailBox->LastNo = tpMailItem->No;
		}
	}
	item_resize_mailbox(tpMailBox);
	SetItemCntStatusText(hWnd, tpMailBox);
	return POP_QUIT;
}

/*
 * pop3_list_proc - メール一覧取得の処理 (新着チェック)
 */
BOOL pop3_list_proc(HWND hWnd, SOCKET soc, char *buf, int len, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	switch (command_status) {
	case POP_START:
	case POP_STARTTLS:
	case POP_USER:
	case POP_PASS:
		command_status = login_proc(hWnd, soc, buf, len, ErrStr, tpMailBox);
		if (command_status == POP_LOGIN) {
			if (PopBeforeSmtpFlag == TRUE) {
				command_status = POP_QUIT;
			} else {
				SetSocStatusTextT(hWnd, TEXT(CMD_STAT)TEXT("\r\n"));
				if (send_buf(soc, CMD_STAT"\r\n") == -1) {
					lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
					return FALSE;
				}
				command_status = POP_STAT;
			}
		}
		break;

	case POP_STAT:
		DateAdd(NULL, NULL);	// タイムゾーンの初期化
		command_status = list_proc_stat(hWnd, soc, buf, len, ErrStr, tpMailBox, ShowFlag);
		break;

	case POP_LIST:
		command_status = list_proc_list(hWnd, soc, buf, len, ErrStr, tpMailBox, ShowFlag);
		break;

	case POP_UIDL_ALL:
		command_status = list_proc_uidl_all(hWnd, soc, buf, len, ErrStr, tpMailBox, ShowFlag);
		break;

	case POP_UIDL_CHECK:
		command_status = list_proc_uidl_check(hWnd, soc, buf, len, ErrStr, tpMailBox, ShowFlag);
		break;

	case POP_UIDL:
		command_status = list_proc_uidl_set(hWnd, soc, buf, len, ErrStr, tpMailBox, ShowFlag);
		break;

	case POP_TOP:
	case POP_RETR:
		command_status = list_proc_top(hWnd, soc, buf, len, ErrStr, tpMailBox, ShowFlag);
		break;

	case POP_QUIT:
		if (str_cmp_ni(buf, "+OK", 3) == 0 || str_cmp_ni(buf, "-ERR", 3) == 0) {
			SetSocStatusText(hWnd, buf);
		}
		return TRUE;
	}

	switch (command_status) {
	case POP_ERR:
		item_resize_mailbox(tpMailBox);
		send_buf(soc, CMD_QUIT"\r\n");
		return FALSE;

	case POP_QUIT:
		item_resize_mailbox(tpMailBox);
		SetSocStatusTextT(hWnd, TEXT(CMD_QUIT));
		if (send_buf(soc, CMD_QUIT"\r\n") == -1) {
			lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
			return FALSE;
		}
		break;
	}
	return TRUE;
}

/*
 * pop3_exec_proc - RETRとDELEの処理 (実行)
 */
BOOL pop3_exec_proc(HWND hWnd, SOCKET soc, char *buf, int len, TCHAR *ErrStr, MAILBOX *tpMailBox, BOOL ShowFlag)
{
	switch (command_status) {
	case POP_START:
	case POP_STARTTLS:
	case POP_USER:
	case POP_PASS:
		command_status = login_proc(hWnd, soc, buf, len, ErrStr, tpMailBox);
		if (command_status == POP_LOGIN) {
			command_status = exec_proc_init(hWnd, soc, buf, len, ErrStr, tpMailBox, ShowFlag);
		}
		break;

	case POP_RETR:
		command_status = exec_proc_retr(hWnd, soc, buf, len, ErrStr, tpMailBox, ShowFlag);
		break;

	case POP_UIDL:
		command_status = exec_proc_uidl(hWnd, soc, buf, len, ErrStr, tpMailBox, ShowFlag);
		break;

	case POP_TOP:
		command_status = exec_proc_top(hWnd, soc, buf, len, ErrStr, tpMailBox, ShowFlag);
		break;

	case POP_DELE:
		command_status = exec_proc_dele(hWnd, soc, buf, len, ErrStr, tpMailBox, ShowFlag);
		break;

	case POP_QUIT:
		if (str_cmp_ni(buf, "+OK", 3) == 0 || str_cmp_ni(buf, "-ERR", 3) == 0) {
			SetSocStatusText(hWnd, buf);
		}
		return TRUE;
	}

	switch (command_status) {
	case POP_ERR:
		send_buf(soc, CMD_QUIT"\r\n");
		return FALSE;

	case POP_QUIT:
		SetSocStatusTextT(hWnd, TEXT(CMD_QUIT));
		if (send_buf(soc, CMD_QUIT"\r\n") == -1) {
			lstrcpy(ErrStr, STR_ERR_SOCK_SEND);
			return FALSE;
		}
		break;
	}
	return TRUE;
}

/*
 * pop3_free - POP3情報の解放
 */
void pop3_free(void)
{
	mail_buf_free();
	uidl_free();
}
/* End of source */
