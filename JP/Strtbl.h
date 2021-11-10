/*
 * nPOP
 *
 * Strtbl.h (JP)
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_STR_TBL_H
#define _INC_STR_TBL_H

// General
#define STR_DEFAULT_FONT			TEXT("ＭＳ ゴシック")
#define STR_DEFAULT_FONTCHARSET		SHIFTJIS_CHARSET

#define STR_DEFAULT_BURA			TEXT("、。，．？！゛゜…‥・’”：；）〕］｝〉》」』】≫々ゝゞぁぃぅぇぉっゃゅょァィゥェォッャュョー")
#define STR_DEFAULT_OIDA			TEXT("‘“￥＄（［｛「〔《『【〈≪")

#define STR_DEFAULT_HEAD_CHARSET	TEXT("UTF-8")
#define STR_DEFAULT_HEAD_ENCODE		2		// 0-7bit 1-8bit 2-base64 3-quoted-printable
#define STR_DEFAULT_BODY_CHARSET	TEXT("UTF-8")
#define STR_DEFAULT_BODY_ENCODE		2		// 0-7bit 1-8bit 2-base64 3-quoted-printable

#define STR_DEFAULT_DATEFORMAT		TEXT("yyyy/MM/dd")
#define STR_DEFAULT_TIMEFORMAT		TEXT("HH:mm")

#define STR_FILE_FILTER				TEXT("すべてのファイル (*.*)\0*.*\0\0")
#define STR_TEXT_FILTER				TEXT("テキスト ファイル (*.txt)\0*.txt\0すべてのファイル (*.*)\0*.*\0\0")
#define STR_WAVE_FILTER				TEXT("サウンド ファイル (*.wav)\0*.wav\0すべてのファイル (*.*)\0*.*\0\0")

// Error
#define STR_ERR_MEMALLOC			TEXT("メモリ確保に失敗しました")
#define STR_ERR_INIT				TEXT("初期化に失敗しました")
#define STR_ERR_OPEN				TEXT("ファイルオープンに失敗しました")
#define STR_ERR_SAVEEND				TEXT("保存に失敗しました\n終了処理を続行しますか？")
#define STR_ERR_SAVE				TEXT("保存に失敗しました")
#define STR_ERR_ADD					TEXT("追加に失敗しました")
#define STR_ERR_VIEW				TEXT("表示に失敗しました")
#define STR_ERR_SELECTMAILBOX		TEXT("アカウントが指定されていません")
#define STR_ERR_SELECTMAILADDR		TEXT("メールアドレスが選択されていません")
#define STR_ERR_SETMAILADDR			TEXT("メールアドレスが設定されていません")
#define STR_ERR_INPUTMAILADDR		TEXT("メールアドレスが入力されていません")
#define STR_ERR_CREATECOPY			TEXT("コピー作成に失敗しました")
#define STR_ERR_SAVECOPY			TEXT("保存箱へのコピーに失敗しました")
#define STR_ERR_NOITEM1				TEXT("項目1が設定されていません")
#define STR_ERR_INPUTFINDSTRING		TEXT("検索文字列が入力されていません")
#define STR_ERR_NOMAIL				TEXT("一覧からメールが見つかりません")
#define STR_ERR_SENDLOCK			TEXT("他の送受信を行っている時は送信できません")

// Socket error
#define STR_ERR_SOCK_SELECT			TEXT("selectに失敗しました")
#define STR_ERR_SOCK_DISCONNECT		TEXT("サーバから切断しました")
#define STR_ERR_SOCK_CONNECT		TEXT("サーバ接続に失敗しました")
#define STR_ERR_SOCK_SENDRECV		TEXT("送受信時にエラーが発生しました")
#define STR_ERR_SOCK_EVENT			TEXT("イベント設定に失敗しました")
#define STR_ERR_SOCK_NOSERVER		TEXT("サーバ名が設定されていません")
#define STR_ERR_SOCK_GETIPADDR		TEXT("IPアドレス取得に失敗しました\nサーバ名を確認してください")
#define STR_ERR_SOCK_CREATESOCKET	TEXT("ソケット作成に失敗しました")
#define STR_ERR_SOCK_TIMEOUT		TEXT("タイムアウトしました")
#define STR_ERR_SOCK_SEND			TEXT("送信に失敗しました")
#define STR_ERR_SOCK_RESPONSE		TEXT("レスポンスが解析できませんでした\n\n")
#define STR_ERR_SOCK_GETITEMINFO	TEXT("アイテム情報を取得できませんでした")
#define STR_ERR_SOCK_MAILSYNC		TEXT("メール番号の同期が取れませんでした\n\n")\
									TEXT("新着チェックを行いメール番号を同期させてください")
#define STR_ERR_SOCK_NOMESSAGEID	TEXT("Message-Idを取得出来ませんでした")
#define STR_ERR_SOCK_NOUSERID		TEXT("ユーザ名が設定されていません")
#define STR_ERR_SOCK_NOPASSWORD		TEXT("パスワードが設定されていません")
#define STR_ERR_SOCK_BADPASSWORD	TEXT("ユーザ名かパスワードが間違っています\n\n")
#define STR_ERR_SOCK_ACCOUNT		TEXT("アカウントが受け付けられませんでした\n\n")
#define STR_ERR_SOCK_NOAPOP			TEXT("APOPに対応していないサーバです")
#define STR_ERR_SOCK_STAT			TEXT("STATに失敗しました\n\n")
#define STR_ERR_SOCK_TOP			TEXT("TOPに失敗しました\n\n")
#define STR_ERR_SOCK_RETR			TEXT("RETRに失敗しました\n\n")
#define STR_ERR_SOCK_DELE			TEXT("削除に失敗しました\n\n")
#define STR_ERR_SOCK_NOATTACH		TEXT("添付ファイルが見つかりませんでした")
#define STR_ERR_SOCK_BADFROM		TEXT("送信元メールアドレスが正しく設定されていません")
#define STR_ERR_SOCK_HELO			TEXT("HELOに失敗しました\n")\
									TEXT("送信元メールアドレスが正しく設定されているか確認してください\n\n")
#define STR_ERR_SOCK_SMTPAUTH		TEXT("SMTPサーバのログインに失敗しました\n\n")
#define STR_ERR_SOCK_RSET			TEXT("RSETに失敗しました\n\n")
#define STR_ERR_SOCK_MAILFROM		TEXT("MAIL FROMに失敗しました\n")\
									TEXT("送信元メールアドレスが正しく設定されているか確認してください\n\n")
#define STR_ERR_SOCK_NOTO			TEXT("送信先が設定されていません")
#define STR_ERR_SOCK_RCPTTO			TEXT("RCPT TOに失敗しました\n")\
									TEXT("送信先メールアドレスが正しく設定されているか確認してください\n\n")
#define STR_ERR_SOCK_DATA			TEXT("DATAに失敗しました\n\n")
#define STR_ERR_SOCK_MAILSEND		TEXT("メール送信に失敗しました\n\n")
#define STR_ERR_SOCK_SSL_INIT		TEXT("SSLの初期化に失敗しました")
#define STR_ERR_SOCK_SSL_VERIFY		TEXT("サーバ証明書の検証に失敗しました")

// Question
#define STR_Q_DELETE				TEXT("削除してもよろしいですか？")
#define STR_Q_DELSERVERMAIL			TEXT("サーバから削除されるメールがありますが実行しますか？")
#define STR_Q_DELLISTMAIL			TEXT("%d 件のメールを一覧から削除してもよろしいですか？%s")
#define STR_Q_DELLISTMAIL_NOSERVER	TEXT("\n(サーバからは削除されません)")
#define STR_Q_DELMAILBOX			TEXT("現在表示されているアカウントを削除してよろしいですか？")
#define STR_Q_DELATTACH				TEXT("添付ファイルを削除してもよろしいですか？")
#define STR_Q_COPY					TEXT("%d 件のメールを保存箱へコピーしてもよろしいですか？")
#define STR_Q_DEPENDENCE			TEXT("機種依存文字が存在しますがよろしいですか？")
#define STR_Q_UNLINKATTACH			TEXT("添付ファイルへのリンクを解除してもよろしいですか？")
#define STR_Q_ADDADDRESS			TEXT("%d 件のメールアドレスをアドレス帳に追加してもよろしいですか？")
#define STR_Q_NEXTFIND				TEXT("最後まで検索しました\n初めから検索しなおしますか？")
#define STR_Q_EDITCANSEL			TEXT("編集をキャンセルしてもよろしいですか？")
#define STR_Q_SENDMAIL				TEXT("送信してもよろしいですか？")
#define STR_Q_ATTACH				TEXT("添付ファイルにウィルスなどが含まれている場合、\n")\
									TEXT("コンピュータに被害を及ぼす可能性があります。\n\n")\
									TEXT("実行してよろしいですか？")

// Message
#define STR_MSG_NOMARK				TEXT("マークされたメールがありません")
#define STR_MSG_NOBODY				TEXT("本文が取得されていないため開くことができません\n\n")\
									TEXT("'受信用にマーク'を付けて'マークを実行'して本文を取得してください")
#define STR_MSG_NONEWMAIL			TEXT("新着メールはありません")
#define STR_MSG_NOFIND				TEXT("\"%s\" が見つかりません")

// Window title
#define STR_TITLE_NOREADMAILBOX		TEXT("%s - [未読アカウント: %d]")
#define STR_TITLE_MAILEDIT			TEXT("メール編集")
#define STR_TITLE_MAILVIEW			TEXT("メール表示")
#define STR_TITLE_MAILVIEW_COUNT	TEXT(" - [%d 件目]")

// Message title
#define STR_TITLE_EXEC				TEXT("実行")
#define STR_TITLE_ALLEXEC			TEXT("巡回実行")
#define STR_TITLE_SEND				TEXT("直ちに送信")
#define STR_TITLE_OPEN				TEXT("開く")
#define STR_TITLE_SAVE				TEXT("保存")
#define STR_TITLE_COPY				TEXT("コピー")
#define STR_TITLE_DELETE			TEXT("削除")
#define STR_TITLE_ERROR				TEXT("エラー")
#define STR_TITLE_SETMAILBOX		TEXT("アカウント設定")
#define STR_TITLE_OPTION			TEXT("オプション")
#define STR_TITLE_STARTPASSWORD		TEXT("起動")
#define STR_TITLE_SHOWPASSWORD		TEXT("表示")
#define STR_TITLE_FIND				TEXT("検索")
#define STR_TITLE_ALLFIND			TEXT("\"%s\" の検索")
#define STR_TITLE_ATTACH_MSG		TEXT("開く")

// Window status
#define STR_STATUS_VIEWINFO			TEXT("表示 %d 件")
#define STR_STATUS_MAILBOXINFO		TEXT("表示 %d/ サーバ %d")
#define STR_STATUS_MAILINFO			TEXT("新着 %d, 未開封 %d")

// Socket status
#define STR_STATUS_GETHOSTBYNAME	TEXT("ホスト名解決中...")
#define STR_STATUS_CONNECT			TEXT("接続中...")
#define STR_STATUS_SSL				TEXT("SSL接続中...")
#define STR_STATUS_SSL_COMPLETE		TEXT("SSL接続完了")
#define STR_STATUS_RECV				TEXT("受信中...")
#define STR_STATUS_SENDBODY			TEXT("本文送信中...")
#define STR_STATUS_SOCKINFO			TEXT("%d バイト%s")
#define STR_STATUS_SOCKINFO_RECV	TEXT("受信")
#define STR_STATUS_SOCKINFO_SEND	TEXT("送信")
#define STR_STATUS_SEND_USER		TEXT("ユーザ名送信")
#define STR_STATUS_SEND_PASS		TEXT("パスワード送信")

// Initialize status
#define STR_STATUS_INIT_MAILCNT		TEXT("%d 通")
#define STR_STATUS_INIT_MAILSIZE_B	TEXT("%s バイト")
#define STR_STATUS_INIT_MAILSIZE_KB	TEXT("%s KB")

// Mail list
#define STR_SAVEBOX_NAME			TEXT("[保存箱]")
#define STR_SENDBOX_NAME			TEXT("[送信箱]")
#define STR_MAILBOX_NONAME			TEXT("名称未設定")
#define STR_LIST_LVHEAD_SUBJECT		TEXT("件名")
#define STR_LIST_LVHEAD_FROM		TEXT("差出人")
#define STR_LIST_LVHEAD_TO			TEXT("受取人")
#define STR_LIST_LVHEAD_DATE		TEXT("日付")
#define STR_LIST_LVHEAD_SIZE		TEXT("サイズ")
#define STR_LIST_NOSUBJECT			TEXT("(no subject)")
#define STR_LIST_THREADSTR			TEXT("  + ")

#define STR_LIST_MENU_SENDINFO		TEXT("送信情報(&R)...")
#define STR_LIST_MENU_REPLY			TEXT("返信(&R)...")
#define STR_LIST_MENU_SENDMARK		TEXT("送信用にマーク(&M)\tCtrl+D")
#define STR_LIST_MENU_CREATECOPY	TEXT("コピーの作成(&C)\tCtrl+C")
#define STR_LIST_MENU_RECVMARK		TEXT("受信用にマーク(&M)\tCtrl+D")
#define STR_LIST_MENU_SAVEBOXCOPY	TEXT("保存箱へコピー(&C)\tCtrl+C")

// Mail view
#define STR_VIEW_HEAD_FROM			TEXT("差出人: ")
#define STR_VIEW_HEAD_SUBJECT		TEXT("\r\n件名: ")
#define STR_VIEW_HEAD_DATE			TEXT("\r\n日付: ")

#define STR_VIEW_MENU_ATTACH		TEXT("添付表示(&M)")
#define STR_VIEW_MENU_SOURCE		TEXT("ソース表示(&S)")
#define STR_VIEW_MENU_DELATTACH		TEXT("添付削除(&T)")

// Mail edit
#define STR_EDIT_HEAD_MAILBOX		TEXT("アカウント: ")
#define STR_EDIT_HEAD_TO			TEXT("\r\n宛先: ")
#define STR_EDIT_HEAD_SUBJECT		TEXT("\r\n件名: ")

// SSL
#define STR_SSL_AUTO				TEXT("自動")
#define STR_SSL_TLS10				TEXT("TLS 1.0")
#define STR_SSL_SSL30				TEXT("SSL 3.0")
#define STR_SSL_SSL20				TEXT("SSL 2.0")
#define STR_SSL_STARTTLS			TEXT("STARTTLS")
#define STR_SSL_TLS11				TEXT("TLS 1.1")
#define STR_SSL_TLS12				TEXT("TLS 1.2")
#define STR_SSL_TLS13				TEXT("TLS 1.3")

// SMTP-AUTH
#define STR_SMTP_AUTH_AUTO			TEXT("自動")
#define STR_SMTP_AUTH_CRAM_MD5		TEXT("CRAM-MD5")
#define STR_SMTP_AUTH_LOGIN			TEXT("LOGIN")
#define STR_SMTP_AUTH_PLAIN			TEXT("PLAIN")

// Filter
#define STR_FILTER_USE				TEXT("使用")
#define STR_FILTER_NOUSE			TEXT("未使用")
#define STR_FILTER_STATUS			TEXT("状態")
#define STR_FILTER_ACTION			TEXT("動作")
#define STR_FILTER_ITEM1			TEXT("項目1")
#define STR_FILTER_CONTENT1			TEXT("内容1")
#define STR_FILTER_ITEM2			TEXT("項目2")
#define STR_FILTER_CONTENT2			TEXT("内容2")

#define STR_FILTER_UNRECV			TEXT("受信しない")
#define STR_FILTER_RECV				TEXT("受信する")
#define STR_FILTER_DOWNLOADMARK		TEXT("受信用にマーク")
#define STR_FILTER_DELETEMARK		TEXT("削除用にマーク")
#define STR_FILTER_READICON			TEXT("開封済みにする")
#define STR_FILTER_SAVE				TEXT("保存箱へコピー")

// Cc list
#define STR_CCLIST_TYPE				TEXT("種別")
#define STR_CCLIST_MAILADDRESS		TEXT("メールアドレス")

// Set send
#define STR_SETSEND_BTN_CC			TEXT("Cc, Bcc")
#define STR_SETSEND_BTN_ATTACH		TEXT("添付ファイル")
#define STR_SETSEND_BTN_ETC			TEXT("その他")

// Mail Prop
#define STR_MAILPROP_HEADER			TEXT("ヘッダ")
#define STR_MAILPROP_MAILADDRESS	TEXT("メールアドレス")

// Address list
#define STR_ADDRESSLIST_MAILADDRESS	TEXT("メールアドレス")
#define STR_ADDRESSLIST_COMMENT		TEXT("コメント")

#endif	//_INC_STR_TBL_H
/* End of source */
