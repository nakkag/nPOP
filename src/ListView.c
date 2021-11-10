/*
 * nPOP
 *
 * ListView.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include "General.h"
#include "Memory.h"
#include "String.h"
#include "dpi.h"

/* Define */
#define ICONCOUNT			9

/* Global Variables */
// 外部参照
extern OPTION op;

extern HINSTANCE hInst;
extern int SelBox;

/* Local Function Prototypes */
static int ImageListIconAdd(HIMAGELIST IconList, int Index);
static void ListView_GetDispItem(LV_ITEM *hLVItem);
static int GetIconSortStatus(MAILITEM *tpMailItem);

/*
 * ImageListIconAdd - イメージリストにアイコンを追加
 */
static int ImageListIconAdd(HIMAGELIST IconList, int Index)
{
	HICON hIcon = NULL;
	int ret;

	hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(Index), IMAGE_ICON,
		SICONSIZE, SICONSIZE, 0);

	// イメージリストにアイコンを追加
	ret = ImageList_AddIcon(IconList, hIcon);

	DestroyIcon(hIcon);
	return ret;
}

/*
 * ListView_SetColumn - リストビューにカラムを追加
 */
void ListView_AddColumn(HWND hListView, int fmt, int cx, TCHAR *buf, int iSubItem)
{
	LV_COLUMN lvc;

	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	lvc.fmt = fmt;
	lvc.cx = cx;
	lvc.pszText = buf;
	lvc.iSubItem = iSubItem;
	ListView_InsertColumn(hListView, iSubItem, &lvc);
}

/*
 * CreateListView - リストビューの作成と初期化
 */
HWND CreateListView(HWND hWnd, int Top, int bottom)
{
	HIMAGELIST IconList;
	HIMAGELIST TmpIconList;
	HICON TmpIcon;
	RECT rcClient;
	HWND hListView;

	GetClientRect(hWnd, &rcClient);

#define WS_EX_STYLE		WS_EX_NOPARENTNOTIFY
	hListView = CreateWindowEx(WS_EX_STYLE, WC_LISTVIEW, TEXT(""),
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | op.LvStyle,
		0, Top,
		rcClient.right, rcClient.bottom - Top - bottom, hWnd,
		(HMENU)IDC_LISTVIEW, hInst, NULL);
	if (hListView == NULL) {
		return NULL;
	}

	// 拡張スタイル
	ListView_SetExtendedListViewStyle(hListView, op.LvStyleEx);

	// ヘッダの設定
	ListView_AddColumn(hListView, LVCFMT_LEFT, *(op.LvColSize + 0), STR_LIST_LVHEAD_SUBJECT, 0);
	ListView_AddColumn(hListView, LVCFMT_LEFT, *(op.LvColSize + 1), STR_LIST_LVHEAD_FROM, 1);
	ListView_AddColumn(hListView, LVCFMT_LEFT, *(op.LvColSize + 2), STR_LIST_LVHEAD_DATE, 2);
	ListView_AddColumn(hListView, LVCFMT_RIGHT, *(op.LvColSize + 3), STR_LIST_LVHEAD_SIZE, 3);

	// イメージリストの設定
	IconList = ImageList_Create(SICONSIZE, SICONSIZE, ILC_COLOR | ILC_MASK, ICONCOUNT, ICONCOUNT);
	ImageListIconAdd(IconList, IDI_ICON_NON);
	ImageListIconAdd(IconList, IDI_ICON_MAIN);
	ImageListIconAdd(IconList, IDI_ICON_READ);
	ImageListIconAdd(IconList, IDI_ICON_DOWN);
	ImageListIconAdd(IconList, IDI_ICON_DEL);
	ImageListIconAdd(IconList, IDI_ICON_SENDMAIL);
	ImageListIconAdd(IconList, IDI_ICON_SEND);

	// 送信エラーのメールを作成 (送信済みメールと削除マークの合成)
	TmpIconList = ImageList_Merge(IconList, ICON_SENDMAIL, IconList, ICON_DEL, 0, 0);
	TmpIcon = ImageList_GetIcon(TmpIconList, 0, ILD_NORMAL);
	ImageList_AddIcon(IconList, TmpIcon);
	DestroyIcon(TmpIcon);
	ImageList_Destroy((void *)TmpIconList);

	// オーバーレイ
	ImageListIconAdd(IconList, IDI_ICON_NEW);
	ImageList_SetOverlayImage(IconList, ICON_NEW, 1);

	ListView_SetImageList(hListView, IconList, LVSIL_SMALL);

	IconList = ImageList_Create(SICONSIZE, SICONSIZE, ILC_COLOR | ILC_MASK, 1, 1);
	// 添付ファイル用ステータスアイコン
	ImageListIconAdd(IconList, IDI_ICON_CLIP);
	ListView_SetImageList(hListView, IconList, LVSIL_STATE);

	return hListView;
}

/*
 * ListView_SetRedraw - リストビューの描画切り替え
 */
void ListView_SetRedraw(HWND hListView, BOOL DrawFlag)
{
	switch (DrawFlag) {
	case FALSE:
		// 描画しない
		SendMessage(hListView, WM_SETREDRAW, (WPARAM)FALSE, 0);
		break;

	case TRUE:
		// 描画する
		SendMessage(hListView, WM_SETREDRAW, (WPARAM)TRUE, 0);
		UpdateWindow(hListView);
		break;
	}
}

/*
 * ListView_InsertItemEx - リストビューにアイテムを追加する
 */
int ListView_InsertItemEx(HWND hListView, TCHAR *buf, int len, int Img, long lp, int iItem)
{
	LV_ITEM lvi;

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iItem = iItem;
	lvi.iSubItem = 0;
	lvi.pszText = buf;
	lvi.cchTextMax = len;
	lvi.iImage = Img;
	lvi.lParam = lp;

	// リストビューにアイテムを追加する
	return ListView_InsertItem(hListView, &lvi);
}

/*
 * ListView_MoveItem - リストビューのアイテムを移動
 */
void ListView_MoveItem(HWND hListView, int SelectItem, int Move, int ColCnt)
{
	TCHAR buf[10][BUF_SIZE];
	int i = 0;

	for (i = 0; i < ColCnt; i++) {
		*(*(buf + i)) = TEXT('\0');
		ListView_GetItemText(hListView, SelectItem, i, *(buf + i), BUF_SIZE - 1);
	}
	ListView_DeleteItem(hListView, SelectItem);

	SelectItem = SelectItem + Move;

	ListView_InsertItemEx(hListView, *buf, BUF_SIZE, 0, 0, SelectItem);
	for (i = 1; i < ColCnt; i++) {
		ListView_SetItemText(hListView, SelectItem, i, *(buf + i));
	}
	ListView_SetItemState(hListView, SelectItem,
		LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(hListView, SelectItem, TRUE);
}

/*
 * ListView_GetSelStringList - 選択アイテムのタイトルリストを作成
 */
TCHAR *ListView_GetSelStringList(HWND hListView)
{
	TCHAR buf[BUF_SIZE];
	TCHAR *StrAddr, *p;
	int SelectItem;
	int len;

	SelectItem = -1;
	len = 0;
	while ((SelectItem = ListView_GetNextItem(hListView, SelectItem, LVNI_SELECTED)) != -1) {
		*buf = TEXT('\0');
		ListView_GetItemText(hListView, SelectItem, 0, buf, BUF_SIZE - 1);
		len += lstrlen(buf) + 2;
	}

	p = StrAddr = (TCHAR *)mem_calloc(sizeof(TCHAR) * (len + 1));
	if (StrAddr == NULL) {
		return NULL;
	}

	SelectItem = -1;
	while ((SelectItem = ListView_GetNextItem(hListView, SelectItem, LVNI_SELECTED)) != -1) {
		if (p != StrAddr) {
			*(p++) = TEXT(',');
			*(p++) = TEXT(' ');
		}
		ListView_GetItemText(hListView, SelectItem, 0, p, BUF_SIZE - 1);
		p += lstrlen(p);
	}
	*p = TEXT('\0');
	return StrAddr;
}

/*
 * ListView_GetlParam - アイテムのLPARAMを取得
 */
long ListView_GetlParam(HWND hListView, int i)
{
	LV_ITEM lvi;

	lvi.mask = LVIF_PARAM;
	lvi.iItem = i;
	lvi.iSubItem = 0;
	lvi.lParam = 0;
	ListView_GetItem(hListView, &lvi);
	return lvi.lParam;
}

/*
 * ListView_GetMemToItem - メールアイテムからリストビューのインデックスを取得
 */
int ListView_GetMemToItem(HWND hListView, MAILITEM *tpMemMailItem)
{
	MAILITEM *tpMailItem;
	int i, ItemCnt;

	ItemCnt = ListView_GetItemCount(hListView);
	for (i = 0; i < ItemCnt; i++) {
		tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
		if (tpMailItem == NULL) {
			continue;
		}
		if (tpMailItem == tpMemMailItem) {
			return i;
		}
	}
	return -1;
}

/*
 * ListView_GetNextDeleteItem - 削除マークのアイテムのインデックスを取得
 */
int ListView_GetNextDeleteItem(HWND hListView, int Index)
{
	MAILITEM *tpMailItem;
	int i, ItemCnt;

	ItemCnt = ListView_GetItemCount(hListView);
	for (i = Index + 1; i < ItemCnt; i++) {
		tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
		if (tpMailItem == NULL) {
			continue;
		}
		if (tpMailItem->Status == ICON_DEL) {
			return i;
		}
	}
	return -1;
}

/*
 * ListView_GetNextMailItem - 指定インデックスより後ろの本文を持つメールアイテムのインデックスを取得
 */
int ListView_GetNextMailItem(HWND hListView, int Index)
{
	MAILITEM *tpMailItem;
	int i, ItemCnt;

	ItemCnt = ListView_GetItemCount(hListView);
	for (i = Index + 1; i < ItemCnt; i++) {
		tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
		if (tpMailItem == NULL || tpMailItem->Body == NULL) {
			continue;
		}
		return i;
	}
	return -1;
}

/*
 * ListView_GetPrevMailItem - 指定インデックスより前の本文を持つメールアイテムのインデックスを取得
 */
int ListView_GetPrevMailItem(HWND hListView, int Index)
{
	MAILITEM *tpMailItem;
	int i;

	if (Index == -1) {
		Index = ListView_GetItemCount(hListView);
	}
	for (i = Index - 1; i >= 0; i--) {
		tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
		if (tpMailItem == NULL || tpMailItem->Body == NULL) {
			continue;
		}
		return i;
	}
	return -1;
}

/*
 * ListView_GetNextNoReadItem - 未開封のメールアイテムのインデックスを取得
 */
int ListView_GetNextNoReadItem(HWND hListView, int Index, int endindex)
{
	MAILITEM *tpMailItem;
	int i, ItemCnt;

	ItemCnt = ListView_GetItemCount(hListView);
	for (i = ((Index <= -1) ? 0 : Index); i < endindex; i++) {
		tpMailItem = (MAILITEM *)ListView_GetlParam(hListView, i);
		if (tpMailItem == NULL || tpMailItem->Body == NULL) {
			continue;
		}
		if (tpMailItem->MailStatus == ICON_MAIL) {
			return i;
		}
	}
	return -1;
}

/*
 * ListView_GetNewItem - 新着のインデックスを取得
 */
int ListView_GetNewItem(HWND hListView, MAILBOX *tpMailBox)
{
	MAILITEM *tpMailItem;
	int i;

	for (i = 0; i < tpMailBox->MailItemCnt; i++) {
		tpMailItem = *(tpMailBox->tpMailItem + i);
		if (tpMailItem == NULL) {
			continue;
		}
		if (tpMailItem->New == TRUE) {
			return ListView_GetMemToItem(hListView, tpMailItem);
		}
	}
	return -1;
}

/*
 * ListView_ShowItem - リストビューにアイテムを表示する
 */
BOOL ListView_ShowItem(HWND hListView, MAILBOX *tpMailBox)
{
	MAILITEM *tpMailItem;
	LV_ITEM lvi;
	int i, j;
	int index = -1;

	ListView_SetRedraw(hListView, FALSE);
	// すべてのアイテムを削除
	ListView_DeleteAllItems(hListView);

	ListView_SetItemCount(hListView, tpMailBox->MailItemCnt);

	// アイテムの追加
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvi.iSubItem = 0;
	lvi.stateMask = LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK | LVIS_CUT;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.cchTextMax = 0;
	lvi.iImage = I_IMAGECALLBACK;

	for (i = 0, j = -1; i < tpMailBox->MailItemCnt; i++) {
		if ((tpMailItem = (*(tpMailBox->tpMailItem + i))) == NULL) {
			continue;
		}
		lvi.state = (tpMailItem->New == TRUE) ? INDEXTOOVERLAYMASK(1) : 0;
		lvi.state |= ((tpMailItem->Multipart == TRUE) ? INDEXTOSTATEIMAGEMASK(1) : 0);
		if (tpMailItem->Download == FALSE && tpMailItem->Status != ICON_DOWN &&
			tpMailItem->Status != ICON_DEL && MAILBOX_SEND != SelBox) {
			lvi.state |= LVIS_CUT;
		}
		lvi.iItem = j + 1;
		lvi.lParam = (LPARAM)tpMailItem;

		// リストビューにアイテムを追加する
		j = ListView_InsertItem(hListView, &lvi);
		if (j == -1) {
			ListView_SetRedraw(hListView, TRUE);
			return FALSE;
		}
	}

	if (op.LvThreadView == 1) {
		// スレッド表示
		item_create_thread(tpMailBox);
		ListView_SortItems(hListView, CompareFunc, SORT_THREAD + 1);
	} else if ((SelBox == MAILBOX_SAVE && op.LvAutoSort == 1) || op.LvAutoSort == 2) {
		// 自動ソート
		ListView_SortItems(hListView, CompareFunc, op.LvSortItem);
	}
	ListView_SetRedraw(hListView, FALSE);

	// アイテムを選択状態にする
	if ((j = ListView_GetItemCount(hListView)) > 0) {
		ListView_EnsureVisible(hListView, j - 1, TRUE);
		// 未開封位置の取得
		i = ListView_GetNextNoReadItem(hListView, -1, ListView_GetItemCount(hListView));
		if (SelBox < MAILBOX_USER || i == -1) {
			// 最後のメールを選択する
			index = (op.LvDefSelectPos == 0) ? 0 : j - 1;
			ListView_SetItemState(hListView, index,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		} else {
			// 未開封メールを選択する
			ListView_SetItemState(hListView, i,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			index = (i <= 0) ? 0 : (i - 1);
			ListView_EnsureVisible(hListView, index, TRUE);
		}
	}
	ListView_SetRedraw(hListView, TRUE);
	if (index != -1) {
		ListView_EnsureVisible(hListView, index, TRUE);
	}
	return TRUE;
}

/*
 * ListView_GetDispItem - リストビューに表示するアイテム情報の設定
 */
static void ListView_GetDispItem(LV_ITEM *hLVItem)
{
	MAILITEM *tpMailItem = (MAILITEM *)hLVItem->lParam;
	TCHAR *p = NULL, *r;
	int i, j;

	if (tpMailItem == NULL) {
		return;
	}

	// アイコン
	if (hLVItem->mask & LVIF_IMAGE) {
		hLVItem->iImage = tpMailItem->Status;
	}

	// テキスト
	if (hLVItem->mask & LVIF_TEXT) {
		switch (hLVItem->iSubItem) {
		// 件名
		case 0:
			p = (tpMailItem->Subject != NULL && *tpMailItem->Subject != TEXT('\0')) ?
				tpMailItem->Subject : STR_LIST_NOSUBJECT;

			if (op.LvThreadView == 1 && tpMailItem->Indent != 0) {
				r = hLVItem->pszText;
				// インデントから追加する空白の数を計算する
				j = ((tpMailItem->Indent * 4) > (hLVItem->cchTextMax - 1)) ?
					((hLVItem->cchTextMax - 1) / 4) : tpMailItem->Indent;
				j = (j - 1) * 4;
				// 空白の追加
				for (i = 1; i < j; i++) {
					*(r++) = TEXT(' ');
				}
				// 枝の追加
				r = str_cpy_t(r, STR_LIST_THREADSTR);
				// 件名の追加
				str_cpy_n_t(r, p, BUF_SIZE - (r - hLVItem->pszText) - 1);
				return;
			}
			break;

		// 差出人 (送信箱の場合は受取人)
		case 1:
			p = (MAILBOX_SEND == SelBox) ? tpMailItem->To : tpMailItem->From;
			break;

		// 日付
		case 2:
			p = tpMailItem->Date;
			break;

		// サイズ
		case 3:
			p = tpMailItem->Size;
			break;

		default:
			return;
		}
		str_cpy_n_t(hLVItem->pszText, (p != NULL) ? p : TEXT("\0"), BUF_SIZE - 1);
	}
}

/*
 * GetIconSortStatus - ソート用のステータスの取得
 */
static int GetIconSortStatus(MAILITEM *tpMailItem)
{
	switch (tpMailItem->Status) {
	case ICON_MAIL:		return (tpMailItem->Download == TRUE) ? 1 : 2;
	case ICON_READ:		return (tpMailItem->Download == TRUE) ? 3 : 4;
	case ICON_SENDMAIL:	return 5;
	case ICON_ERROR:	return 6;
	case ICON_DOWN:		return 7;
	case ICON_DEL:		return 8;
	case ICON_SEND:		return 9;
	case ICON_NON:		return 10;
	}
	return 0;
}

/*
 * CompareFunc - ソートフラグに従って文字列の比較を行う
 */
int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	TCHAR *wbuf1, *wbuf2;
	TCHAR tbuf1[BUF_SIZE], tbuf2[BUF_SIZE];
#ifdef UNICODE
	char tmp1[BUF_SIZE], tmp2[BUF_SIZE];
#endif
	int ret, sfg, ghed;
	int len1 = 0, len2 = 0;
	BOOL NumFlag = FALSE;

	sfg = (lParamSort < 0) ? 1 : 0;	// 昇順／降順
	ghed = ABS(lParamSort) - 1;		// ソートを行うヘッダ

	if (lParam1 == 0 || lParam2 == 0) {
		return 0;
	}

	wbuf1 = TEXT("\0");
	wbuf2 = TEXT("\0");

	switch (ghed) {
	// 件名
	case 0:
		if (((MAILITEM *)lParam1)->Subject != NULL)
			wbuf1 = ((MAILITEM *)lParam1)->Subject;
		if (((MAILITEM *)lParam2)->Subject != NULL)
			wbuf2 = ((MAILITEM *)lParam2)->Subject;
		break;

	// 差出人 (送信箱の場合は受取人)
	case 1:
		if (MAILBOX_SEND == SelBox) {
			if (((MAILITEM *)lParam1)->To != NULL)
				wbuf1 = ((MAILITEM *)lParam1)->To;
			if (((MAILITEM *)lParam2)->To != NULL)
				wbuf2 = ((MAILITEM *)lParam2)->To;
		} else {
			if (((MAILITEM *)lParam1)->From != NULL)
				wbuf1 = ((MAILITEM *)lParam1)->From;
			if (((MAILITEM *)lParam2)->From != NULL)
				wbuf2 = ((MAILITEM *)lParam2)->From;
		}
		break;

	// 日付
	case 2:
		if (((MAILITEM *)lParam1)->Date != NULL)
			wbuf1 = ((MAILITEM *)lParam1)->Date;
		if (((MAILITEM *)lParam2)->Date != NULL)
			wbuf2 = ((MAILITEM *)lParam2)->Date;
		if (MAILBOX_SEND == SelBox) {
			char dbuf[BUF_SIZE];
#ifdef UNICODE
			tchar_to_char(wbuf1, tmp1, BUF_SIZE);
			DateConv(tmp1, tmp2);
			SortDateConv(tmp2, dbuf);
			char_to_tchar(dbuf, tbuf1, BUF_SIZE);

			tchar_to_char(wbuf2, tmp1, BUF_SIZE);
			DateConv(tmp1, tmp2);
			SortDateConv(tmp2, dbuf);
			char_to_tchar(dbuf, tbuf2, BUF_SIZE);
#else
			DateConv(wbuf1, dbuf);
			SortDateConv(dbuf, tbuf1);
			DateConv(wbuf2, dbuf);
			SortDateConv(dbuf, tbuf2);
#endif
		} else {
#ifdef UNICODE
			tchar_to_char(wbuf1, tmp1, BUF_SIZE);
			SortDateConv(tmp1, tmp2);
			char_to_tchar(tmp2, tbuf1, BUF_SIZE);

			tchar_to_char(wbuf2, tmp1, BUF_SIZE);
			SortDateConv(tmp1, tmp2);
			char_to_tchar(tmp2, tbuf2, BUF_SIZE);
#else
			SortDateConv(wbuf1, tbuf1);
			SortDateConv(wbuf2, tbuf2);
#endif
		}
		wbuf1 = tbuf1;
		wbuf2 = tbuf2;
		break;

	// サイズ
	case 3:
		if (((MAILITEM *)lParam1)->Size != NULL)
			len1 = _ttoi(((MAILITEM *)lParam1)->Size);
		if (((MAILITEM *)lParam2)->Size != NULL)
			len2 = _ttoi(((MAILITEM *)lParam2)->Size);
		NumFlag = TRUE;
		break;

	// 番号
	case SORT_NO:
		len1 = ((MAILITEM *)lParam1)->No;
		len2 = ((MAILITEM *)lParam2)->No;
		NumFlag = TRUE;
		break;

	// アイコン
	case SORT_IOCN:
		len1 = GetIconSortStatus((MAILITEM *)lParam1);
		len2 = GetIconSortStatus((MAILITEM *)lParam2);
		NumFlag = TRUE;
		break;

	// スレッド
	case SORT_THREAD:
		len1 = ((MAILITEM *)lParam1)->PrevNo;
		len2 = ((MAILITEM *)lParam2)->PrevNo;
		NumFlag = TRUE;
		break;
	}

	ret = (NumFlag == FALSE) ? lstrcmpi(wbuf1, wbuf2) : (len1 - len2);
	return (((ret < 0 && sfg == 1) || (ret > 0 && sfg == 0)) ? 1 : -1);
}

/*
 * ListView_NotifyProc - リストビューイベント
 */
LRESULT ListView_NotifyProc(HWND hWnd, LPARAM lParam)
{
	LV_DISPINFO *plv = (LV_DISPINFO *)lParam;
	NMHDR *CForm = (NMHDR *)lParam;

	switch (plv->hdr.code) {
	case LVN_GETDISPINFO:		// 表示アイテムの要求
		ListView_GetDispItem(&(plv->item));
		return TRUE;

	case LVN_ITEMCHANGED:		// アイテムの選択状態の変更
		return SendMessage(hWnd, WM_LV_EVENT, plv->hdr.code, lParam);
	}

	switch (CForm->code) {
	case NM_DBLCLK:				// ダブルクリック
	case NM_CLICK:				// クリック
	case NM_RCLICK:				// 右クリック
		return SendMessage(hWnd, WM_LV_EVENT, CForm->code, lParam);
	}
	return FALSE;
}
/* End of source */
