/*
 * nPOP
 *
 * Item.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include "General.h"
#include "Memory.h"
#include "String.h"
#include "code.h"
#include "mime.h"
#include "multipart.h"

#include "global.h"
#include "md5.h"

/* Define */

/* Global Variables */
extern OPTION op;
extern BOOL KeyShowHeader;

extern MAILBOX *MailBox;
extern MAILBOX *AddressBox;

/* Local Function Prototypes */
static int item_get_content(char *buf, char *header, char **ret);
static void item_get_content_t(char *buf, char *header, TCHAR **ret);
static void item_get_content_utf8(char* buf, char* header, TCHAR** ret);
static int item_get_content_int(char *buf, char *header, int DefaultRet);
static int item_get_multi_content(char *buf, char *header, char **ret);
static int item_get_mime_content(char *buf, char *header, TCHAR **ret, BOOL multi_flag);
static void item_set_body(MAILITEM *tpMailItem, char *buf, BOOL download);
static int item_save_header_size(TCHAR *header, TCHAR *buf);
static char *item_save_header(TCHAR *header, TCHAR *buf, char *ret);
static BOOL item_filter_check_content(char *buf, TCHAR *filter_header, TCHAR *filter_content);
static int item_filter_check(MAILBOX *tpMailBox, char *buf);

/*
 * item_is_mailbox - メールボックス内のメールリストに指定のメールが存在するか調べる
 */
BOOL item_is_mailbox(MAILBOX *tpMailBox, MAILITEM *tpMailItem)
{
	int i;

	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		if (*(tpMailBox->tpMailItem + i) == tpMailItem) {
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * item_set_count - アイテム数分のメモリを確保
 */
BOOL item_set_count(MAILBOX *tpMailBox, int i)
{
	MAILITEM **tpMailList;

	if (i <= tpMailBox->MailItemCnt) {
		tpMailBox->AllocCnt = tpMailBox->MailItemCnt;
		return TRUE;
	}
	tpMailList = (MAILITEM **)mem_calloc(sizeof(MAILITEM *) * i);
	if (tpMailList == NULL) {
		return FALSE;
	}
	if (tpMailBox->tpMailItem != NULL) {
		CopyMemory(tpMailList, tpMailBox->tpMailItem,
			sizeof(MAILITEM *) * tpMailBox->MailItemCnt);
	}

	mem_free((void **)&tpMailBox->tpMailItem);
	tpMailBox->tpMailItem = tpMailList;
	tpMailBox->AllocCnt = i;
	return TRUE;
}

/*
 * item_add - アイテムの追加
 */
BOOL item_add(MAILBOX *tpMailBox, MAILITEM *tpNewMailItem)
{
	MAILITEM **tpMailList = tpMailBox->tpMailItem;

	if (tpMailBox->AllocCnt <= tpMailBox->MailItemCnt) {
		tpMailList = (MAILITEM **)mem_alloc(sizeof(MAILITEM *) * (tpMailBox->MailItemCnt + 1));
		if (tpMailList == NULL) {
			return FALSE;
		}
		if (tpMailBox->tpMailItem != NULL) {
			CopyMemory(tpMailList, tpMailBox->tpMailItem,
				sizeof(MAILITEM *) * tpMailBox->MailItemCnt);
			mem_free((void **)&tpMailBox->tpMailItem);
		}
		tpMailBox->tpMailItem = tpMailList;
		tpMailBox->AllocCnt = tpMailBox->MailItemCnt + 1;
	}
	*(tpMailList + tpMailBox->MailItemCnt) = tpNewMailItem;
	tpMailBox->MailItemCnt++;
	if (*tpMailBox->tpMailItem != NULL) {
		(*tpMailBox->tpMailItem)->NextNo = 0;
	}
	return TRUE;
}

/*
 * item_copy - アイテムのコピー
 */
void item_copy(MAILITEM *tpFromMailItem, MAILITEM *tpToMailItem)
{
	CopyMemory(tpToMailItem, tpFromMailItem, sizeof(MAILITEM));
	tpToMailItem->Status = tpToMailItem->MailStatus;
	tpToMailItem->New = FALSE;
	tpToMailItem->No = 0;
	tpToMailItem->UIDL = NULL;

	tpToMailItem->From = alloc_copy_t(tpFromMailItem->From);
	tpToMailItem->To = alloc_copy_t(tpFromMailItem->To);
	tpToMailItem->Cc = alloc_copy_t(tpFromMailItem->Cc);
	tpToMailItem->Bcc = alloc_copy_t(tpFromMailItem->Bcc);
	tpToMailItem->Subject = alloc_copy_t(tpFromMailItem->Subject);
	tpToMailItem->Date = alloc_copy_t(tpFromMailItem->Date);
	tpToMailItem->Size = alloc_copy_t(tpFromMailItem->Size);
	tpToMailItem->ReplyTo = alloc_copy_t(tpFromMailItem->ReplyTo);
	tpToMailItem->ContentType = alloc_copy_t(tpFromMailItem->ContentType);
	tpToMailItem->Encoding = alloc_copy_t(tpFromMailItem->Encoding);
	tpToMailItem->MessageID = alloc_copy_t(tpFromMailItem->MessageID);
	tpToMailItem->InReplyTo = alloc_copy_t(tpFromMailItem->InReplyTo);
	tpToMailItem->References = alloc_copy_t(tpFromMailItem->References);
	tpToMailItem->Body = alloc_copy(tpFromMailItem->Body);
	tpToMailItem->MailBox = alloc_copy_t(tpFromMailItem->MailBox);
	tpToMailItem->Attach = alloc_copy_t(tpFromMailItem->Attach);

	tpToMailItem->HeadCharset = alloc_copy_t(tpFromMailItem->HeadCharset);
	tpToMailItem->BodyCharset = alloc_copy_t(tpFromMailItem->BodyCharset);
}

/*
 * item_to_mailbox - アイテムをメールボックスに追加
 */
MAILITEM *item_to_mailbox(MAILBOX *tpMailBox, MAILITEM *tpNewMailItem, TCHAR *MailBoxName, BOOL SendClear)
{
	MAILITEM **tpMailList;
	int i = 0;

	tpMailList = (MAILITEM **)mem_calloc(sizeof(MAILITEM *) * (tpMailBox->MailItemCnt + 1));
	if (tpMailList == NULL) {
		return NULL;
	}
	if (tpMailBox->tpMailItem != NULL) {
		for (i = 0; i < tpMailBox->MailItemCnt; i++) {
			*(tpMailList + i) = *(tpMailBox->tpMailItem + i);
		}
	}

	*(tpMailList + i) = (MAILITEM *)mem_calloc(sizeof(MAILITEM));
	if (*(tpMailList + i) == NULL) {
		mem_free((void **)&tpMailList);
		return NULL;
	}
	item_copy(tpNewMailItem, *(tpMailList + i));
	if ((*(tpMailList + i))->MailBox == NULL) {
		(*(tpMailList + i))->MailBox = alloc_copy_t(MailBoxName);
	}
	if (SendClear == TRUE) {
		(*(tpMailList + i))->MailStatus = (*(tpMailList + i))->Status = ICON_NON;
		mem_free(&(*(tpMailList + i))->Date);
		(*(tpMailList + i))->Date = NULL;
		mem_free(&(*(tpMailList + i))->MessageID);
		(*(tpMailList + i))->MessageID = NULL;
		(*(tpMailList + i))->hEditWnd = NULL;
	}

	mem_free((void **)&tpMailBox->tpMailItem);
	tpMailBox->tpMailItem = tpMailList;
	tpMailBox->MailItemCnt++;
	tpMailBox->AllocCnt = tpMailBox->MailItemCnt;
	if (*tpMailBox->tpMailItem != NULL) {
		(*tpMailBox->tpMailItem)->NextNo = 0;
	}
	return *(tpMailList + i);
}

/*
 * item_resize_mailbox - アイテム情報の整理
 */
BOOL item_resize_mailbox(MAILBOX *tpMailBox)
{
	MAILITEM **tpMailList;
	int i, cnt = 0;

	if (tpMailBox->tpMailItem == NULL) {
		tpMailBox->AllocCnt = tpMailBox->MailItemCnt = 0;
		return FALSE;
	}

	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		if (*(tpMailBox->tpMailItem + i) == NULL) {
			continue;
		}
		cnt++;
	}

	tpMailList = (MAILITEM **)mem_calloc(sizeof(MAILITEM *) * cnt);
	if (tpMailList == NULL) {
		mem_free((void **)&tpMailBox->tpMailItem);
		tpMailBox->tpMailItem = NULL;
		tpMailBox->AllocCnt = tpMailBox->MailItemCnt = 0;
		return FALSE;
	}
	cnt = 0;
	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		if (*(tpMailBox->tpMailItem + i) == NULL) {
			continue;
		}
		*(tpMailList + cnt) = *(tpMailBox->tpMailItem + i);
		cnt++;
	}

	mem_free((void **)&tpMailBox->tpMailItem);
	tpMailBox->tpMailItem = tpMailList;
	tpMailBox->AllocCnt = tpMailBox->MailItemCnt = cnt;
	if (cnt != 0 && *tpMailBox->tpMailItem != NULL) {
		(*tpMailBox->tpMailItem)->NextNo = 0;
	}
	return TRUE;
}

/*
 * item_free - メールアイテムの解放
 */
void item_free(MAILITEM **tpMailItem, int cnt)
{
	int i;

	for (i = 0; i < cnt; i++) {
		if (*(tpMailItem + i) == NULL) {
			continue;
		}
		if ((*(tpMailItem + i))->hEditWnd != NULL) {
			(*(tpMailItem + i)) = NULL;
			continue;
		}
		mem_free(&(*(tpMailItem + i))->From);
		mem_free(&(*(tpMailItem + i))->To);
		mem_free(&(*(tpMailItem + i))->Cc);
		mem_free(&(*(tpMailItem + i))->Bcc);
		mem_free(&(*(tpMailItem + i))->Date);
		mem_free(&(*(tpMailItem + i))->Size);
		mem_free(&(*(tpMailItem + i))->Subject);
		mem_free(&(*(tpMailItem + i))->ReplyTo);
		mem_free(&(*(tpMailItem + i))->ContentType);
		mem_free(&(*(tpMailItem + i))->Encoding);
		mem_free(&(*(tpMailItem + i))->MessageID);
		mem_free(&(*(tpMailItem + i))->UIDL);
		mem_free(&(*(tpMailItem + i))->InReplyTo);
		mem_free(&(*(tpMailItem + i))->References);
		mem_free(&(*(tpMailItem + i))->Body);
		mem_free(&(*(tpMailItem + i))->MailBox);
		mem_free(&(*(tpMailItem + i))->Attach);
		mem_free(&(*(tpMailItem + i))->HeadCharset);
		mem_free(&(*(tpMailItem + i))->BodyCharset);

		mem_free(&*(tpMailItem + i));
		(*(tpMailItem + i)) = NULL;
	}
}

/*
 * item_get_content - コンテンツの取得
 */
static int item_get_content(char *buf, char *header, char **ret)
{
	char *p;
	int len;

	// 位置の取得
	p = GetHeaderStringPoint(buf, header);
	if (p == NULL) {
		*ret = NULL;
		return 0;
	}
	// サイズの取得
	len = GetHeaderStringSize(p, FALSE);
	*ret = (char *)mem_alloc(len + 1);
	if (*ret == NULL) {
		return 0;
	}
	GetHeaderString(p, *ret, FALSE);
	return len;
}

/*
 * item_get_content_t - コンテンツの取得
 */
static void item_get_content_t(char *buf, char *header, TCHAR **ret)
{
#ifdef UNICODE
	char *cret;
#endif
	char *p;
	int len;

	// 位置の取得
	p = GetHeaderStringPoint(buf, header);
	if (p == NULL) {
		*ret = NULL;
		return;
	}
	// サイズの取得
	len = GetHeaderStringSize(p, TRUE);
#ifdef UNICODE
	cret = (char *)mem_alloc(sizeof(char) * (len + 1));
	if (cret == NULL) {
		return;
	}
	GetHeaderString(p, cret, TRUE);
	*ret = alloc_char_to_tchar(cret);
	mem_free(&cret);
#else
	*ret = (char *)mem_alloc(sizeof(char) * (len + 1));
	if (*ret == NULL) {
		return;
	}
	GetHeaderString(p, *ret, TRUE);
#endif
}

/*
 * item_get_content_utf8 - コンテンツの取得
 */
static void item_get_content_utf8(char* buf, char* header, TCHAR** ret)
{
#ifdef UNICODE
	char* cret;
#endif
	char* p;
	int len;

	// 位置の取得
	p = GetHeaderStringPoint(buf, header);
	if (p == NULL) {
		*ret = NULL;
		return;
	}
	// サイズの取得
	len = GetHeaderStringSize(p, TRUE);
#ifdef UNICODE
	cret = (char*)mem_alloc(sizeof(char) * (len + 1));
	if (cret == NULL) {
		return;
	}
	GetHeaderString(p, cret, TRUE);
	*ret = alloc_utf8_to_tchar(cret);
	mem_free(&cret);
#else
	* ret = (char*)mem_alloc(sizeof(char) * (len + 1));
	if (*ret == NULL) {
		return;
	}
	GetHeaderString(p, *ret, TRUE);
#endif
}
/*
 * item_get_content_int - コンテンツの取得
 */
static int item_get_content_int(char *buf, char *header, int DefaultRet)
{
	TCHAR *Content;
	int ret;

	item_get_content_utf8(buf, header, &Content);
	if (Content == NULL) {
		return DefaultRet;
	}
	ret = _ttoi(Content);
	mem_free(&Content);
	return ret;
}

/*
 * item_get_multi_content - 複数ある場合は一つにまとめてコンテンツの取得
 */
static int item_get_multi_content(char *buf, char *header, char **ret)
{
	char *tmp;
	char *p, *r;
	int len;
	int ret_len = 0;

	*ret = NULL;
	p = buf;
	while (1) {
		// 位置の取得
		p = GetHeaderStringPoint(p, header);
		if (p == NULL) {
			return ret_len;
		}
		// サイズの取得
		len = GetHeaderStringSize(p, FALSE);
		if (*ret != NULL) {
			r = tmp = (char *)mem_alloc(ret_len + len + 2);
			if (tmp == NULL) {
				return ret_len;
			}
			r = str_cpy(r, *ret);
			r = str_cpy(r, ",");
			mem_free(&*ret);
			*ret = tmp;
			ret_len += (len + 1);
		} else {
			r = *ret = (char *)mem_alloc(len + 1);
			if (*ret == NULL) {
				return ret_len;
			}
			ret_len = len;
		}
		GetHeaderString(p, r, FALSE);
		p += len;
	}
}

/*
 * item_get_mime_content - ヘッダのコンテンツを取得してMIMEデコードを行う
 */
static int item_get_mime_content(char *buf, char *header, TCHAR **ret, BOOL multi_flag)
{
	char *Content;
	int len;

	*ret = NULL;

	len = ((multi_flag == TRUE) ? item_get_multi_content : item_get_content)(buf, header, &Content);
	if (Content != NULL) {
		len = MIME_decode(Content, NULL);
		*ret = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
		if (*ret != NULL) {
			MIME_decode(Content, *ret);
		}
		mem_free(&Content);
	}
	return len;
}

/*
 * item_get_message_id - メッセージIDの取得
 */
char *item_get_message_id(char *buf)
{
	char *Content, *p;
	MD5_CTX context;
	unsigned char digest[16];
	int len;

	// Message-Id取得
	Content = NULL;
	item_get_content(buf, HEAD_MESSAGEID, &Content);
	TrimMessageId(Content);
	if (Content != NULL && *Content != '\0') {
		return Content;
	}
	mem_free(&Content);

	// UIDLを取得
	Content = NULL;
	item_get_content(buf, HEAD_X_UIDL, &Content);
	if (Content != NULL && *Content != '\0') {
		return Content;
	}
	mem_free(&Content);

	// Dateを取得
	Content = NULL;
	item_get_content(buf, HEAD_DATE, &Content);
	if (Content != NULL && *Content != '\0') {
		return Content;
	}
	mem_free(&Content);

	// ヘッダのハッシュ値を取得
	p = GetBodyPointa(buf);
	if (p != NULL) {
		len = p - buf;
	} else {
		len = tstrlen(buf);
	}
	MD5Init(&context);
	MD5Update(&context, buf, len);
	MD5Final(digest, &context);

	Content = (char *)mem_alloc(16 * 2 + 1);
	if (Content == NULL) {
		return NULL;
	}
	base64_encode(digest, Content, 16);
	return Content;
}

/*
 * item_get_number_to_index - メール番号からアイテムのインデックスを取得
 */
int item_get_number_to_index(MAILBOX *tpMailBox, int No)
{
	int i;

	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		if (*(tpMailBox->tpMailItem + i) == NULL) {
			continue;
		}
		if ((*(tpMailBox->tpMailItem + i))->No == No) {
			return i;
		}
	}
	return -1;
}

/*
 * item_get_next_download_mark - ダウンロードマークのアイテムのインデックスを取得
 */
int item_get_next_download_mark(MAILBOX *tpMailBox, int Index, int *No)
{
	MAILITEM *tpMailItem;
	int i;

	for (i = Index + 1; i < tpMailBox->MailItemCnt; i++) {
		tpMailItem = *(tpMailBox->tpMailItem + i);
		if (tpMailItem == NULL) {
			continue;
		}
		if (tpMailItem->Status == ICON_DOWN) {
			if (No != NULL) {
				*No = tpMailItem->No;
			}
			return i;
		}
	}
	return -1;
}

/*
 * item_get_next_delete_mark - 削除マークのアイテムのインデックスを取得
 */
int item_get_next_delete_mark(MAILBOX *tpMailBox, int Index, int *No)
{
	MAILITEM *tpMailItem;
	int i;

	for (i = Index + 1; i < tpMailBox->MailItemCnt; i++) {
		tpMailItem = *(tpMailBox->tpMailItem + i);
		if (tpMailItem == NULL) {
			continue;
		}
		if (tpMailItem->Status == ICON_DEL) {
			if (No != NULL) {
				*No = tpMailItem->No;
			}
			return i;
		}
	}
	return -1;
}

/*
 * item_get_next_send_mark - 送信マークの付いたアイテムのインデックスを取得
 */
int item_get_next_send_mark(MAILBOX *tpMailBox, int Index, int *MailBoxIndex)
{
	MAILITEM *tpMailItem;
	int BoxIndex;
	int i;
	int wkIndex = -1;
	int wkBoxIndex = -1;

	for (i = Index + 1; i < tpMailBox->MailItemCnt; i++) {
		tpMailItem = *(tpMailBox->tpMailItem + i);
		if (tpMailItem == NULL || tpMailItem->Status != ICON_SEND) {
			continue;
		}
		if (MailBoxIndex == NULL) {
			return i;
		}
		BoxIndex = mailbox_name_to_index(tpMailItem->MailBox);
		if (*MailBoxIndex == -1 || *MailBoxIndex == BoxIndex) {
			wkIndex = i;
			wkBoxIndex = BoxIndex;
			break;
		}
		if (wkIndex == -1) {
			wkIndex = i;
			wkBoxIndex = BoxIndex;
		}
	}
	if (MailBoxIndex != NULL) {
		*MailBoxIndex = wkBoxIndex;
	}
	return wkIndex;
}

/*
 * item_get_next_send_mark_mailbox - 指定のメールボックスの送信マークの付いたアイテムのインデックスを取得
 */
int item_get_next_send_mark_mailbox(MAILBOX *tpMailBox, int Index, int MailBoxIndex)
{
	MAILITEM *tpMailItem;
	int BoxIndex;
	int i;

	if (MailBoxIndex == -1) {
		return -1;
	}
	for (i = Index + 1; i < tpMailBox->MailItemCnt; i++) {
		tpMailItem = *(tpMailBox->tpMailItem + i);
		if (tpMailItem == NULL || tpMailItem->Status != ICON_SEND) {
			continue;
		}
		BoxIndex = mailbox_name_to_index(tpMailItem->MailBox);
		if (MailBoxIndex == BoxIndex) {
			return i;
		}
	}
	return -1;
}

/*
 * item_set_body - アイテムに本文を設定
 */
static void item_set_body(MAILITEM *tpMailItem, char *buf, BOOL download)
{
	char *p, *r;
	int len;
	int header_size;

	p = GetBodyPointa(buf);
	if (p != NULL && *p != '\0') {
		// デコード
		r = MIME_body_decode_transfer(tpMailItem, p);
		if (r == NULL) {
			return;
		}
		len = tstrlen(r);

		// ヘッダを表示する設定の場合
		header_size = (op.ShowHeader == 1 || KeyShowHeader == TRUE) ? (p - buf) : 0;

		mem_free(&tpMailItem->Body);
		tpMailItem->Body = (char *)mem_alloc(sizeof(char) * (len + header_size + 1));
		if (tpMailItem->Body != NULL) {
			if (op.ShowHeader == 1 || KeyShowHeader == TRUE) {
				// ヘッダ
				str_cpy_n(tpMailItem->Body, buf, header_size + 1);
			}
			// 本文
			tstrcpy(tpMailItem->Body + header_size, r);
		}
		mem_free(&r);

	} else if (op.ShowHeader == 1 || KeyShowHeader == TRUE) {
		// 本文が存在しない場合はヘッダのみ設定
		mem_free(&tpMailItem->Body);
		tpMailItem->Body = alloc_copy(buf);

	} else if (download == TRUE) {
		mem_free(&tpMailItem->Body);
		tpMailItem->Body = (char *)mem_alloc(sizeof(char));
		if (tpMailItem->Body != NULL) {
			*tpMailItem->Body = '\0';
		}
	}
	if (tpMailItem->Body != NULL) {
		tpMailItem->Status = tpMailItem->MailStatus = ICON_MAIL;
	}
}

/*
 * item_mail_to_item - アイテムにヘッダと本文を設定
 */
BOOL item_mail_to_item(MAILITEM *tpMailItem, char *buf, int Size, BOOL download)
{
	TCHAR *msgid1 = NULL, *msgid2 = NULL, *t = NULL;
	char *Content;
#ifdef UNICODE
	char *dcode;
#endif

	if (download == TRUE) {
		// 既存の情報を解放
		mem_free(&tpMailItem->Subject);
		mem_free(&tpMailItem->From);
		mem_free(&tpMailItem->To);
		mem_free(&tpMailItem->Cc);
		mem_free(&tpMailItem->ReplyTo);
		mem_free(&tpMailItem->ContentType);
		mem_free(&tpMailItem->Encoding);
		mem_free(&tpMailItem->Date);
		if (Size >= 0) {
			mem_free(&tpMailItem->Size);
		}
		mem_free(&tpMailItem->MessageID);
		mem_free(&tpMailItem->InReplyTo);
		mem_free(&tpMailItem->References);
	}
	// Subject
	item_get_mime_content(buf, HEAD_SUBJECT, &tpMailItem->Subject, FALSE);
	// From
	item_get_mime_content(buf, HEAD_FROM, &tpMailItem->From, FALSE);
	// To
	item_get_mime_content(buf, HEAD_TO, &tpMailItem->To, TRUE);
	// Cc
	item_get_mime_content(buf, HEAD_CC, &tpMailItem->Cc, TRUE);
	// Reply-To
	item_get_mime_content(buf, HEAD_REPLYTO, &tpMailItem->ReplyTo, FALSE);
	// Content-Type
	item_get_mime_content(buf, HEAD_CONTENTTYPE, &tpMailItem->ContentType, FALSE);
	if (tpMailItem->ContentType != NULL &&
		str_cmp_ni_t(tpMailItem->ContentType, TEXT("multipart"), lstrlen(TEXT("multipart"))) == 0) {
		tpMailItem->Multipart = TRUE;
	} else {
		// Content-Transfer-Encoding
#ifdef UNICODE
		item_get_content(buf, HEAD_ENCODING, &Content);
		if (Content != NULL) {
			tpMailItem->Encoding = alloc_char_to_tchar(Content);
			mem_free(&Content);
		}
#else
		item_get_content(buf, HEAD_ENCODING, &tpMailItem->Encoding);
#endif
	}

	// Date
	item_get_content(buf, HEAD_DATE, &Content);
	if (Content != NULL) {
#ifdef UNICODE
		dcode = (char *)mem_alloc(BUF_SIZE);
		if (dcode != NULL) {
			DateConv(Content, dcode);
			tpMailItem->Date = alloc_char_to_tchar(dcode);
			mem_free(&dcode);
		}
#else
		tpMailItem->Date = (char *)mem_alloc(BUF_SIZE);
		if (tpMailItem->Date != NULL) {
			DateConv(Content, tpMailItem->Date);
		}
#endif
		mem_free(&Content);
	}

	// Size
	if (Size >= 0) {
		TCHAR num[BUF_SIZE];
#ifndef _itot_s
		wsprintf(num, TEXT("%d"), Size);
#else
		_itot_s(Size, num, 256, 10);
#endif
		tpMailItem->Size = alloc_copy_t(num);
	}

	// Message-Id
#ifdef UNICODE
	Content = item_get_message_id(buf);
	if (Content != NULL) {
		tpMailItem->MessageID = alloc_char_to_tchar(Content);
		mem_free(&Content);
	}
#else
	tpMailItem->MessageID = item_get_message_id(buf);
#endif

	// In-Reply-To
#ifdef UNICODE
	item_get_content(buf, HEAD_INREPLYTO, &Content);
	TrimMessageId(Content);
	if (Content != NULL) {
		tpMailItem->InReplyTo = alloc_char_to_tchar(Content);
		mem_free(&Content);
	}
#else
	item_get_content(buf, HEAD_INREPLYTO, &tpMailItem->InReplyTo);
	TrimMessageId(tpMailItem->InReplyTo);
#endif

	// References
	item_get_content(buf, HEAD_REFERENCES, &Content);
	if (Content != NULL) {
#ifdef UNICODE
		dcode = (char *)mem_alloc(GetReferencesSize(Content, TRUE) + 1);
		if (dcode != NULL) {
			ConvReferences(Content, dcode, FALSE);
			msgid1 = alloc_char_to_tchar(dcode);

			ConvReferences(Content, dcode, TRUE);
			msgid2 = alloc_char_to_tchar(dcode);
			mem_free(&dcode);
		}
#else
		msgid1 = (char *)mem_alloc(GetReferencesSize(Content, FALSE) + 1);
		if (msgid1 != NULL) {
			ConvReferences(Content, msgid1, FALSE);
		}

		msgid2 = (char *)mem_alloc(GetReferencesSize(Content, TRUE) + 1);
		if (msgid2 != NULL) {
			ConvReferences(Content, msgid2, TRUE);
		}
#endif
		if (msgid1 != NULL && msgid2 != NULL && lstrcmp(msgid1, msgid2) == 0) {
			mem_free(&msgid2);
			msgid2 = NULL;
		}
		mem_free(&Content);
	}

	if (tpMailItem->InReplyTo == NULL || *tpMailItem->InReplyTo == TEXT('\0')) {
		mem_free(&tpMailItem->InReplyTo);
		tpMailItem->InReplyTo = alloc_copy_t(msgid1);
		t = msgid2;
	} else {
		t = (msgid1 != NULL && lstrcmp(tpMailItem->InReplyTo, msgid1) != 0) ? msgid1 : msgid2;
	}
	tpMailItem->References = alloc_copy_t(t);
	mem_free(&msgid1);
	mem_free(&msgid2);

	// Body
	item_set_body(tpMailItem, buf, download);
	return TRUE;
}

/*
 * item_header_to_item - メールヘッダからアイテムを作成する
 */
MAILITEM *item_header_to_item(MAILBOX *tpMailBox, char *buf, int Size)
{
	MAILITEM *tpMailItem;
	int fret;

	// フィルタをチェック
	fret = item_filter_check(tpMailBox, buf);
	if (fret == FILTER_UNRECV) {
		return (MAILITEM *)-1;
	}

	// メール情報の確保
	tpMailItem = (MAILITEM *)mem_calloc(sizeof(MAILITEM));
	if (tpMailItem == NULL) {
		return NULL;
	}
	// ヘッダと本文を設定
	item_mail_to_item(tpMailItem, buf, Size, FALSE);

	// メール情報のリストに追加
	if (!(fret & FILTER_UNRECV) && item_add(tpMailBox, tpMailItem) == -1) {
		item_free(&tpMailItem, 1);
		return NULL;
	}

	// フィルタ動作設定
	// 開封済み設定
	if (fret & FILTER_READICON && tpMailItem->MailStatus != ICON_NON) {
		tpMailItem->Status = tpMailItem->MailStatus = ICON_READ;
	}
	// マーク設定
	if (fret & FILTER_DOWNLOADMARK) {
		tpMailItem->Status = ICON_DOWN;
	} else if (fret & FILTER_DELETEMARK) {
		tpMailItem->Status = ICON_DEL;
	}
	// 保存箱へコピー
	if (fret & FILTER_SAVE &&
		tpMailItem->MailStatus != ICON_NON &&
		item_find_thread(MailBox + MAILBOX_SAVE, tpMailItem->MessageID, (MailBox + MAILBOX_SAVE)->MailItemCnt) == -1) {
		item_to_mailbox(MailBox + MAILBOX_SAVE, tpMailItem, tpMailBox->Name, FALSE);
	}
	if (fret & FILTER_UNRECV) {
		// 受信しないフラグが有効の場合は解放する
		item_free(&tpMailItem, 1);
		return (MAILITEM *)-1;
	}
	return tpMailItem;
}

/*
 * item_string_to_item - 文字列からアイテムを作成する
 */
MAILITEM *item_string_to_item(MAILBOX *tpMailBox, char *buf)
{
	MAILITEM *tpMailItem;
	int i;

	tpMailItem = (MAILITEM *)mem_calloc(sizeof(MAILITEM));
	if (tpMailItem == NULL) {
		return NULL;
	}
	item_get_content_t(buf, HEAD_SUBJECT, &tpMailItem->Subject);
	item_get_content_t(buf, HEAD_FROM, &tpMailItem->From);
	item_get_content_t(buf, HEAD_TO, &tpMailItem->To);
	item_get_content_t(buf, HEAD_CC, &tpMailItem->Cc);
	item_get_content_t(buf, HEAD_BCC, &tpMailItem->Bcc);
	item_get_content_t(buf, HEAD_DATE, &tpMailItem->Date);
	item_get_content_t(buf, HEAD_SIZE, &tpMailItem->Size);
	item_get_content_t(buf, HEAD_REPLYTO, &tpMailItem->ReplyTo);
	item_get_content_t(buf, HEAD_CONTENTTYPE, &tpMailItem->ContentType);
	item_get_content_t(buf, HEAD_ENCODING, &tpMailItem->Encoding);
	item_get_content_t(buf, HEAD_MESSAGEID, &tpMailItem->MessageID);
	item_get_content_t(buf, HEAD_INREPLYTO, &tpMailItem->InReplyTo);
	item_get_content_t(buf, HEAD_REFERENCES, &tpMailItem->References);
	item_get_content_t(buf, HEAD_X_UIDL, &tpMailItem->UIDL);

	item_get_content_t(buf, HEAD_X_MAILBOX, &tpMailItem->MailBox);
	item_get_content_t(buf, HEAD_X_ATTACH, &tpMailItem->Attach);
	if (tpMailItem->Attach == NULL) {
		item_get_content_t(buf, HEAD_X_ATTACH_OLD, &tpMailItem->Attach);
		if (tpMailItem->Attach != NULL) {
			TCHAR *p;
			for (p = tpMailItem->Attach; *p != TEXT('\0'); p++) {
#ifndef UNICODE
				// 2バイトコードの場合は2バイト進める
				if (IsDBCSLeadByte((BYTE)*p) == TRUE && *(p + 1) != TEXT('\0')) {
					p++;
					continue;
				}
#endif
				if (*p == TEXT(',')) {
					*p = ATTACH_SEP;
				}
			}
		}
	}
	item_get_content_t(buf, HEAD_X_HEADCHARSET, &tpMailItem->HeadCharset);
	tpMailItem->HeadEncoding = item_get_content_int(buf, HEAD_X_HEADENCODE, 0);
	item_get_content_t(buf, HEAD_X_BODYCHARSET, &tpMailItem->BodyCharset);
	tpMailItem->BodyEncoding = item_get_content_int(buf, HEAD_X_BODYENCODE, 0);

	// No
	tpMailItem->No = item_get_content_int(buf, HEAD_X_NO, -1);
	if (tpMailItem->No == -1) {
		tpMailItem->No = item_get_content_int(buf, HEAD_X_NO_OLD, 0);
	}
	// MailStatus
	tpMailItem->MailStatus = item_get_content_int(buf, HEAD_X_STATUS, -1);
	if (tpMailItem->MailStatus == -1) {
		tpMailItem->MailStatus = item_get_content_int(buf, HEAD_X_STATUS_OLD, 0);
	}
	// MarkStatus
	i = item_get_content_int(buf, HEAD_X_MARK, -1);
	tpMailItem->Status = (i != -1) ? i : tpMailItem->MailStatus;
	// Download
	tpMailItem->Download = item_get_content_int(buf, HEAD_X_DOWNLOAD, -1);
	if (tpMailItem->Download == -1) {
		tpMailItem->Download = item_get_content_int(buf, HEAD_X_DOWNLOAD_OLD, 0);
	}
	// Multipart
	if (tpMailItem->Attach != NULL || (tpMailItem->ContentType != NULL &&
		str_cmp_ni_t(tpMailItem->ContentType, TEXT("multipart"), lstrlen(TEXT("multipart"))) == 0)) {
		tpMailItem->Multipart = TRUE;
	}
	return tpMailItem;
}

/*
 * item_string_to_item_utf8 - 文字列からアイテムを作成する(UTF-8)
 */
MAILITEM* item_string_to_item_utf8(MAILBOX* tpMailBox, char* buf)
{
	MAILITEM* tpMailItem;
	int i;

	tpMailItem = (MAILITEM*)mem_calloc(sizeof(MAILITEM));
	if (tpMailItem == NULL) {
		return NULL;
	}
	item_get_content_utf8(buf, HEAD_SUBJECT, &tpMailItem->Subject);
	item_get_content_utf8(buf, HEAD_FROM, &tpMailItem->From);
	item_get_content_utf8(buf, HEAD_TO, &tpMailItem->To);
	item_get_content_utf8(buf, HEAD_CC, &tpMailItem->Cc);
	item_get_content_utf8(buf, HEAD_BCC, &tpMailItem->Bcc);
	item_get_content_utf8(buf, HEAD_DATE, &tpMailItem->Date);
	item_get_content_utf8(buf, HEAD_SIZE, &tpMailItem->Size);
	item_get_content_utf8(buf, HEAD_REPLYTO, &tpMailItem->ReplyTo);
	item_get_content_utf8(buf, HEAD_CONTENTTYPE, &tpMailItem->ContentType);
	item_get_content_utf8(buf, HEAD_ENCODING, &tpMailItem->Encoding);
	item_get_content_utf8(buf, HEAD_MESSAGEID, &tpMailItem->MessageID);
	item_get_content_utf8(buf, HEAD_INREPLYTO, &tpMailItem->InReplyTo);
	item_get_content_utf8(buf, HEAD_REFERENCES, &tpMailItem->References);
	item_get_content_utf8(buf, HEAD_X_UIDL, &tpMailItem->UIDL);

	item_get_content_utf8(buf, HEAD_X_MAILBOX, &tpMailItem->MailBox);
	item_get_content_utf8(buf, HEAD_X_ATTACH, &tpMailItem->Attach);
	if (tpMailItem->Attach == NULL) {
		item_get_content_utf8(buf, HEAD_X_ATTACH_OLD, &tpMailItem->Attach);
		if (tpMailItem->Attach != NULL) {
			TCHAR* p;
			for (p = tpMailItem->Attach; *p != TEXT('\0'); p++) {
#ifndef UNICODE
				// 2バイトコードの場合は2バイト進める
				if (IsDBCSLeadByte((BYTE)*p) == TRUE && *(p + 1) != TEXT('\0')) {
					p++;
					continue;
				}
#endif
				if (*p == TEXT(',')) {
					*p = ATTACH_SEP;
				}
			}
		}
	}
	item_get_content_utf8(buf, HEAD_X_HEADCHARSET, &tpMailItem->HeadCharset);
	tpMailItem->HeadEncoding = item_get_content_int(buf, HEAD_X_HEADENCODE, 0);
	item_get_content_utf8(buf, HEAD_X_BODYCHARSET, &tpMailItem->BodyCharset);
	tpMailItem->BodyEncoding = item_get_content_int(buf, HEAD_X_BODYENCODE, 0);

	// No
	tpMailItem->No = item_get_content_int(buf, HEAD_X_NO, -1);
	if (tpMailItem->No == -1) {
		tpMailItem->No = item_get_content_int(buf, HEAD_X_NO_OLD, 0);
	}
	// MailStatus
	tpMailItem->MailStatus = item_get_content_int(buf, HEAD_X_STATUS, -1);
	if (tpMailItem->MailStatus == -1) {
		tpMailItem->MailStatus = item_get_content_int(buf, HEAD_X_STATUS_OLD, 0);
	}
	// MarkStatus
	i = item_get_content_int(buf, HEAD_X_MARK, -1);
	tpMailItem->Status = (i != -1) ? i : tpMailItem->MailStatus;
	// Download
	tpMailItem->Download = item_get_content_int(buf, HEAD_X_DOWNLOAD, -1);
	if (tpMailItem->Download == -1) {
		tpMailItem->Download = item_get_content_int(buf, HEAD_X_DOWNLOAD_OLD, 0);
	}
	// Multipart
	if (tpMailItem->Attach != NULL || (tpMailItem->ContentType != NULL &&
		str_cmp_ni_t(tpMailItem->ContentType, TEXT("multipart"), lstrlen(TEXT("multipart"))) == 0)) {
		tpMailItem->Multipart = TRUE;
	}
	return tpMailItem;
}

/*
 * item_save_header_size - 保存するヘッダのサイズ
 */
static int item_save_header_size(TCHAR *header, TCHAR *buf)
{
	if (buf == NULL) {
		return 0;
	}
#ifdef UNICODE
	return (tchar_to_utf8_size(header) + 1 + tchar_to_utf8_size(buf) + 2 - 2);
#else
	return (lstrlen(header) + 1 + lstrlen(buf) + 2);
#endif
}

/*
 * item_save_header - ヘッダを保存する文字列の作成
 */
static char *item_save_header(TCHAR *header, TCHAR *buf, char *ret)
{
#ifdef UNICODE
	TCHAR *wret;
	int len;

	if (buf == NULL) {
		return ret;
	}
	wret = mem_alloc(sizeof(TCHAR) * (lstrlen(header) + 1 + lstrlen(buf) + 2 + 1));
	if (buf == NULL) {
		return ret;
	}
	str_join_t(wret, header, TEXT(" "), buf, TEXT("\r\n"), (TCHAR *)-1);
	len = tchar_to_utf8_size(wret);
	tchar_to_utf8(wret, ret, len);
	mem_free(&wret);
	*(ret + len) = '\0';
	return (ret + len - 1);
#else
	if (buf == NULL) {
		return ret;
	}
	return str_join_t(ret, header, TEXT(" "), buf, TEXT("\r\n"), (TCHAR *)-1);
#endif
}

/*
 * item_to_string_size - メールの保存文字列のサイズ取得
 */
int item_to_string_size(MAILITEM *tpMailItem, BOOL BodyFlag)
{
	TCHAR X_No[10], X_Mstatus[10], X_Status[10], X_Downflag[10];
	TCHAR X_HeadEncoding[10], X_BodyEncoding[10];
	int len = 0;

#ifndef _itot_s
	wsprintf(X_No, TEXT("%d"), tpMailItem->No);
	wsprintf(X_Mstatus, TEXT("%d"), tpMailItem->MailStatus);
	wsprintf(X_Status, TEXT("%d"), tpMailItem->Status);
	wsprintf(X_Downflag, TEXT("%d"), tpMailItem->Download);
	wsprintf(X_HeadEncoding, TEXT("%d"), tpMailItem->HeadEncoding);
	wsprintf(X_BodyEncoding, TEXT("%d"), tpMailItem->BodyEncoding);
#else
	_itot_s(tpMailItem->No, X_No, 10, 10);
	_itot_s(tpMailItem->MailStatus, X_Mstatus, 10, 10);
	_itot_s(tpMailItem->Status, X_Status, 10, 10);
	_itot_s(tpMailItem->Download, X_Downflag, 10, 10);
	_itot_s(tpMailItem->HeadEncoding, X_HeadEncoding, 10, 10);
	_itot_s(tpMailItem->BodyEncoding, X_BodyEncoding, 10, 10);
#endif

	len += item_save_header_size(TEXT(HEAD_FROM), tpMailItem->From);
	len += item_save_header_size(TEXT(HEAD_TO), tpMailItem->To);
	len += item_save_header_size(TEXT(HEAD_CC), tpMailItem->Cc);
	len += item_save_header_size(TEXT(HEAD_BCC), tpMailItem->Bcc);
	len += item_save_header_size(TEXT(HEAD_DATE), tpMailItem->Date);
	len += item_save_header_size(TEXT(HEAD_SUBJECT), tpMailItem->Subject);
	len += item_save_header_size(TEXT(HEAD_SIZE), tpMailItem->Size);
	len += item_save_header_size(TEXT(HEAD_REPLYTO), tpMailItem->ReplyTo);
	len += item_save_header_size(TEXT(HEAD_CONTENTTYPE), tpMailItem->ContentType);
	len += item_save_header_size(TEXT(HEAD_ENCODING), tpMailItem->Encoding);
	len += item_save_header_size(TEXT(HEAD_MESSAGEID), tpMailItem->MessageID);
	len += item_save_header_size(TEXT(HEAD_INREPLYTO), tpMailItem->InReplyTo);
	len += item_save_header_size(TEXT(HEAD_REFERENCES), tpMailItem->References);
	len += item_save_header_size(TEXT(HEAD_X_UIDL), tpMailItem->UIDL);

	len += item_save_header_size(TEXT(HEAD_X_MAILBOX), tpMailItem->MailBox);
	len += item_save_header_size(TEXT(HEAD_X_ATTACH), tpMailItem->Attach);
	if (tpMailItem->HeadCharset != NULL) {
		len += item_save_header_size(TEXT(HEAD_X_HEADCHARSET), tpMailItem->HeadCharset);
		len += item_save_header_size(TEXT(HEAD_X_HEADENCODE), X_HeadEncoding);
	}
	if (tpMailItem->BodyCharset != NULL) {
		len += item_save_header_size(TEXT(HEAD_X_BODYCHARSET), tpMailItem->BodyCharset);
		len += item_save_header_size(TEXT(HEAD_X_BODYENCODE), X_BodyEncoding);
	}

	len += item_save_header_size(TEXT(HEAD_X_NO), X_No);
	len += item_save_header_size(TEXT(HEAD_X_STATUS), X_Mstatus);
	if (tpMailItem->MailStatus != tpMailItem->Status) {
		len += item_save_header_size(TEXT(HEAD_X_MARK), X_Status);
	}
	len += item_save_header_size(TEXT(HEAD_X_DOWNLOAD), X_Downflag);
	len += 2;

	if (BodyFlag == TRUE && tpMailItem->Body != NULL && *tpMailItem->Body != '\0') {
		len += tstrlen(tpMailItem->Body);
	}
	len += 5;
	return len;
}

/*
 * item_to_string - メールの保存文字列の取得
 */
char *item_to_string(char *buf, MAILITEM *tpMailItem, BOOL BodyFlag)
{
	char *p = buf;
	TCHAR X_No[10], X_Mstatus[10], X_Status[10], X_Downflag[10];
	TCHAR X_HeadEncoding[10], X_BodyEncoding[10];

#ifndef _itot_s
	wsprintf(X_No, TEXT("%d"), tpMailItem->No);
	wsprintf(X_Mstatus, TEXT("%d"), tpMailItem->MailStatus);
	wsprintf(X_Status, TEXT("%d"), tpMailItem->Status);
	wsprintf(X_Downflag, TEXT("%d"), tpMailItem->Download);
	wsprintf(X_HeadEncoding, TEXT("%d"), tpMailItem->HeadEncoding);
	wsprintf(X_BodyEncoding, TEXT("%d"), tpMailItem->BodyEncoding);
#else
	_itot_s(tpMailItem->No, X_No, 10, 10);
	_itot_s(tpMailItem->MailStatus, X_Mstatus, 10, 10);
	_itot_s(tpMailItem->Status, X_Status, 10, 10);
	_itot_s(tpMailItem->Download, X_Downflag, 10, 10);
	_itot_s(tpMailItem->HeadEncoding, X_HeadEncoding, 10, 10);
	_itot_s(tpMailItem->BodyEncoding, X_BodyEncoding, 10, 10);
#endif

	p = item_save_header(TEXT(HEAD_FROM), tpMailItem->From, p);
	p = item_save_header(TEXT(HEAD_TO), tpMailItem->To, p);
	p = item_save_header(TEXT(HEAD_CC), tpMailItem->Cc, p);
	p = item_save_header(TEXT(HEAD_BCC), tpMailItem->Bcc, p);
	p = item_save_header(TEXT(HEAD_DATE), tpMailItem->Date, p);
	p = item_save_header(TEXT(HEAD_SUBJECT), tpMailItem->Subject, p);
	p = item_save_header(TEXT(HEAD_SIZE), tpMailItem->Size, p);
	p = item_save_header(TEXT(HEAD_REPLYTO), tpMailItem->ReplyTo, p);
	p = item_save_header(TEXT(HEAD_CONTENTTYPE), tpMailItem->ContentType, p);
	p = item_save_header(TEXT(HEAD_ENCODING), tpMailItem->Encoding, p);
	p = item_save_header(TEXT(HEAD_MESSAGEID), tpMailItem->MessageID, p);
	p = item_save_header(TEXT(HEAD_INREPLYTO), tpMailItem->InReplyTo, p);
	p = item_save_header(TEXT(HEAD_REFERENCES), tpMailItem->References, p);
	p = item_save_header(TEXT(HEAD_X_UIDL), tpMailItem->UIDL, p);

	p = item_save_header(TEXT(HEAD_X_MAILBOX), tpMailItem->MailBox, p);
	p = item_save_header(TEXT(HEAD_X_ATTACH), tpMailItem->Attach, p);
	if (tpMailItem->HeadCharset != NULL) {
		p = item_save_header(TEXT(HEAD_X_HEADCHARSET), tpMailItem->HeadCharset, p);
		p = item_save_header(TEXT(HEAD_X_HEADENCODE), X_HeadEncoding, p);
	}
	if (tpMailItem->BodyCharset != NULL) {
		p = item_save_header(TEXT(HEAD_X_BODYCHARSET), tpMailItem->BodyCharset, p);
		p = item_save_header(TEXT(HEAD_X_BODYENCODE), X_BodyEncoding, p);
	}

	p = item_save_header(TEXT(HEAD_X_NO), X_No, p);
	p = item_save_header(TEXT(HEAD_X_STATUS), X_Mstatus, p);
	if (tpMailItem->MailStatus != tpMailItem->Status) {
		p = item_save_header(TEXT(HEAD_X_MARK), X_Status, p);
	}
	p = item_save_header(TEXT(HEAD_X_DOWNLOAD), X_Downflag, p);
	p = str_cpy(p, "\r\n");

	if (BodyFlag == TRUE && tpMailItem->Body != NULL && *tpMailItem->Body != '\0') {
		p = str_cpy(p, tpMailItem->Body);
	}
	p = str_cpy(p, "\r\n.\r\n");
	return p;
}

/*
 * item_filter_check_content - 文字列のチェック
 */
static BOOL item_filter_check_content(char *buf, TCHAR *filter_header, TCHAR *filter_content)
{
	TCHAR *Content;
	BOOL ret;
	int len;
	char *cbuf;

	if (filter_content == NULL || *filter_content == TEXT('\0')) {
		return TRUE;
	}
	if (lstrcmpi(filter_header, TEXT("body")) == 0) {
		char *p;
#ifdef UNICODE
		cbuf = alloc_tchar_to_char(filter_content);
#else
		cbuf = filter_content;
#endif
		// 本文の位置取得
		p = GetBodyPointa(buf);
		if (p == NULL) {
			ret = str_match(cbuf, "");
		} else {
			// 比較
			ret = str_match(cbuf, p);
		}
#ifdef UNICODE
		mem_free(&cbuf);
#endif
		return ret;
	}
#ifdef UNICODE
	cbuf = alloc_tchar_to_char(filter_header);
	if (cbuf == NULL) {
		return FALSE;
	}
	// コンテンツの取得
	len = item_get_mime_content(buf, cbuf, &Content, TRUE);
	mem_free(&cbuf);
#else
	// コンテンツの取得
	len = item_get_mime_content(buf, filter_header, &Content, TRUE);
#endif
	if (Content == NULL) {
		return str_match_t(filter_content, TEXT(""));
	}
	// 比較
	ret = str_match_t(filter_content, Content);
	mem_free(&Content);
	return ret;
}

/*
 * item_filter_check - フィルタ文字列のチェック
 */
static int item_filter_check(MAILBOX *tpMailBox, char *buf)
{
	int RetFlag = 0;
	int fret;
	int i, j;

	if (tpMailBox->FilterEnable == 0 || tpMailBox->tpFilter == NULL ||
		buf == NULL || *buf == '\0') {
		return FILTER_RECV;
	}
	for (i = 0; i < tpMailBox->FilterCnt; i++) {
		if (*(tpMailBox->tpFilter + i) == NULL ||
			(*(tpMailBox->tpFilter + i))->Enable == 0) {
			continue;
		}
		// 項目のチェック
		if ((*(tpMailBox->tpFilter + i))->Header1 == NULL ||
			*(*(tpMailBox->tpFilter + i))->Header1 == TEXT('\0')) {
			continue;
		}
		if (item_filter_check_content(buf, (*(tpMailBox->tpFilter + i))->Header1,
			(*(tpMailBox->tpFilter + i))->Content1) == FALSE) {
			continue;
		}
		if ((*(tpMailBox->tpFilter + i))->Header2 == NULL ||
			*(*(tpMailBox->tpFilter + i))->Header2 == TEXT('\0') ||
			item_filter_check_content(buf, (*(tpMailBox->tpFilter + i))->Header2,
			(*(tpMailBox->tpFilter + i))->Content2) == TRUE) {

			j = (*(tpMailBox->tpFilter + i))->Action;
			for (fret = 1; j > 0; j--) {
				fret *= 2;
			}
			switch (fret) {
			case FILTER_UNRECV:
			case FILTER_RECV:
				// 受信フラグ
				if (!(RetFlag & FILTER_RECV) && !(RetFlag & FILTER_UNRECV)) {
					RetFlag |= fret;
				}
				break;

			case FILTER_DOWNLOADMARK:
			case FILTER_DELETEMARK:
				// マークフラグ
				if (!(RetFlag & FILTER_RECV) && !(RetFlag & FILTER_DOWNLOADMARK) && !(RetFlag & FILTER_DELETEMARK)) {
					RetFlag |= fret;
				}
				break;

			default:
				if (!(RetFlag & FILTER_RECV)) {
					RetFlag |= fret;
				}
				break;
			}
		}
	}
	return ((RetFlag == 0) ? FILTER_RECV : RetFlag);
}

/*
 * item_find_thread - メッセージIDを検索する
 */
int item_find_thread(MAILBOX *tpMailBox, TCHAR *p, int Index)
{
	MAILITEM *tpMailItem;
	int j;

	if (p == NULL) {
		return -1;
	}
	for (j = Index - 1; j >= 0; j--) {
		if ((tpMailItem = (*(tpMailBox->tpMailItem + j))) == NULL ||
			tpMailItem->MessageID == NULL) {
			continue;
		}
		if (lstrcmp(tpMailItem->MessageID, p) == 0) {
			return j;
		}
	}
	return -1;
}

/*
 * item_create_thread - スレッドを構築する
 */
void item_create_thread(MAILBOX *tpMailBox)
{
	MAILITEM *tpMailItem;
	MAILITEM *tpNextMailItem;
	int i, no = 0, n;
	int parent;

	if (tpMailBox->MailItemCnt == 0 ||
		(*tpMailBox->tpMailItem != NULL && (*tpMailBox->tpMailItem)->NextNo != 0)) {
		return;
	}
	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		if ((tpMailItem = *(tpMailBox->tpMailItem + i)) == NULL) {
			continue;
		}
		tpMailItem->NextNo = tpMailItem->PrevNo = tpMailItem->Indent = 0;
		// 元メールの検索
		parent = item_find_thread(tpMailBox, tpMailItem->InReplyTo, i);
		if (parent == -1) {
			parent = item_find_thread(tpMailBox, tpMailItem->References, i);
		}
		// 元メールなし
		if (parent == -1) {
			(*(tpMailBox->tpMailItem + no))->NextNo = i;
			tpMailItem->PrevNo = no;
			no = i;
			continue;
		}
		// インデントを設定する
		tpMailItem->Indent = (*(tpMailBox->tpMailItem + parent))->Indent + 1;
		n = (*(tpMailBox->tpMailItem + parent))->NextNo;
		while (n != 0) {
			if ((tpNextMailItem = (*(tpMailBox->tpMailItem + n))) == NULL) {
				n = 0;
				break;
			}
			// インデントからメールの追加位置を取得する
			if (tpNextMailItem->Indent < tpMailItem->Indent) {
				tpMailItem->PrevNo = tpNextMailItem->PrevNo;
				tpMailItem->NextNo = n;

				(*(tpMailBox->tpMailItem + tpNextMailItem->PrevNo))->NextNo = i;
				tpNextMailItem->PrevNo = i;
				break;
			}
			n = tpNextMailItem->NextNo;
		}
		if (n == 0) {
			(*(tpMailBox->tpMailItem + no))->NextNo = i;
			tpMailItem->PrevNo = no;
			no = i;
		}
	}
	// ソート数値を設定する
	no = 0;
	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		if (*(tpMailBox->tpMailItem + no) == NULL) {
			break;
		}
		(*(tpMailBox->tpMailItem + no))->PrevNo = i;
		no = (*(tpMailBox->tpMailItem + no))->NextNo;
	}
}
/* End of source */
