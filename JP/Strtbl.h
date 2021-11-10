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
#define STR_DEFAULT_FONT			TEXT("�l�r �S�V�b�N")
#define STR_DEFAULT_FONTCHARSET		SHIFTJIS_CHARSET

#define STR_DEFAULT_BURA			TEXT("�A�B�C�D�H�I�J�K�c�d�E�f�h�F�G�j�l�n�p�r�t�v�x�z��X�T�U����������������@�B�D�F�H�b�������[")
#define STR_DEFAULT_OIDA			TEXT("�e�g�����i�m�o�u�k�s�w�y�q��")

#define STR_DEFAULT_HEAD_CHARSET	TEXT("UTF-8")
#define STR_DEFAULT_HEAD_ENCODE		2		// 0-7bit 1-8bit 2-base64 3-quoted-printable
#define STR_DEFAULT_BODY_CHARSET	TEXT("UTF-8")
#define STR_DEFAULT_BODY_ENCODE		2		// 0-7bit 1-8bit 2-base64 3-quoted-printable

#define STR_DEFAULT_DATEFORMAT		TEXT("yyyy/MM/dd")
#define STR_DEFAULT_TIMEFORMAT		TEXT("HH:mm")

#define STR_FILE_FILTER				TEXT("���ׂẴt�@�C�� (*.*)\0*.*\0\0")
#define STR_TEXT_FILTER				TEXT("�e�L�X�g �t�@�C�� (*.txt)\0*.txt\0���ׂẴt�@�C�� (*.*)\0*.*\0\0")
#define STR_WAVE_FILTER				TEXT("�T�E���h �t�@�C�� (*.wav)\0*.wav\0���ׂẴt�@�C�� (*.*)\0*.*\0\0")

// Error
#define STR_ERR_MEMALLOC			TEXT("�������m�ۂɎ��s���܂���")
#define STR_ERR_INIT				TEXT("�������Ɏ��s���܂���")
#define STR_ERR_OPEN				TEXT("�t�@�C���I�[�v���Ɏ��s���܂���")
#define STR_ERR_SAVEEND				TEXT("�ۑ��Ɏ��s���܂���\n�I�������𑱍s���܂����H")
#define STR_ERR_SAVE				TEXT("�ۑ��Ɏ��s���܂���")
#define STR_ERR_ADD					TEXT("�ǉ��Ɏ��s���܂���")
#define STR_ERR_VIEW				TEXT("�\���Ɏ��s���܂���")
#define STR_ERR_SELECTMAILBOX		TEXT("�A�J�E���g���w�肳��Ă��܂���")
#define STR_ERR_SELECTMAILADDR		TEXT("���[���A�h���X���I������Ă��܂���")
#define STR_ERR_SETMAILADDR			TEXT("���[���A�h���X���ݒ肳��Ă��܂���")
#define STR_ERR_INPUTMAILADDR		TEXT("���[���A�h���X�����͂���Ă��܂���")
#define STR_ERR_CREATECOPY			TEXT("�R�s�[�쐬�Ɏ��s���܂���")
#define STR_ERR_SAVECOPY			TEXT("�ۑ����ւ̃R�s�[�Ɏ��s���܂���")
#define STR_ERR_NOITEM1				TEXT("����1���ݒ肳��Ă��܂���")
#define STR_ERR_INPUTFINDSTRING		TEXT("���������񂪓��͂���Ă��܂���")
#define STR_ERR_NOMAIL				TEXT("�ꗗ���烁�[����������܂���")
#define STR_ERR_SENDLOCK			TEXT("���̑���M���s���Ă��鎞�͑��M�ł��܂���")

// Socket error
#define STR_ERR_SOCK_SELECT			TEXT("select�Ɏ��s���܂���")
#define STR_ERR_SOCK_DISCONNECT		TEXT("�T�[�o����ؒf���܂���")
#define STR_ERR_SOCK_CONNECT		TEXT("�T�[�o�ڑ��Ɏ��s���܂���")
#define STR_ERR_SOCK_SENDRECV		TEXT("����M���ɃG���[���������܂���")
#define STR_ERR_SOCK_EVENT			TEXT("�C�x���g�ݒ�Ɏ��s���܂���")
#define STR_ERR_SOCK_NOSERVER		TEXT("�T�[�o�����ݒ肳��Ă��܂���")
#define STR_ERR_SOCK_GETIPADDR		TEXT("IP�A�h���X�擾�Ɏ��s���܂���\n�T�[�o�����m�F���Ă�������")
#define STR_ERR_SOCK_CREATESOCKET	TEXT("�\�P�b�g�쐬�Ɏ��s���܂���")
#define STR_ERR_SOCK_TIMEOUT		TEXT("�^�C���A�E�g���܂���")
#define STR_ERR_SOCK_SEND			TEXT("���M�Ɏ��s���܂���")
#define STR_ERR_SOCK_RESPONSE		TEXT("���X�|���X����͂ł��܂���ł���\n\n")
#define STR_ERR_SOCK_GETITEMINFO	TEXT("�A�C�e�������擾�ł��܂���ł���")
#define STR_ERR_SOCK_MAILSYNC		TEXT("���[���ԍ��̓��������܂���ł���\n\n")\
									TEXT("�V���`�F�b�N���s�����[���ԍ��𓯊������Ă�������")
#define STR_ERR_SOCK_NOMESSAGEID	TEXT("Message-Id���擾�o���܂���ł���")
#define STR_ERR_SOCK_NOUSERID		TEXT("���[�U�����ݒ肳��Ă��܂���")
#define STR_ERR_SOCK_NOPASSWORD		TEXT("�p�X���[�h���ݒ肳��Ă��܂���")
#define STR_ERR_SOCK_BADPASSWORD	TEXT("���[�U�����p�X���[�h���Ԉ���Ă��܂�\n\n")
#define STR_ERR_SOCK_ACCOUNT		TEXT("�A�J�E���g���󂯕t�����܂���ł���\n\n")
#define STR_ERR_SOCK_NOAPOP			TEXT("APOP�ɑΉ����Ă��Ȃ��T�[�o�ł�")
#define STR_ERR_SOCK_STAT			TEXT("STAT�Ɏ��s���܂���\n\n")
#define STR_ERR_SOCK_TOP			TEXT("TOP�Ɏ��s���܂���\n\n")
#define STR_ERR_SOCK_RETR			TEXT("RETR�Ɏ��s���܂���\n\n")
#define STR_ERR_SOCK_DELE			TEXT("�폜�Ɏ��s���܂���\n\n")
#define STR_ERR_SOCK_NOATTACH		TEXT("�Y�t�t�@�C����������܂���ł���")
#define STR_ERR_SOCK_BADFROM		TEXT("���M�����[���A�h���X���������ݒ肳��Ă��܂���")
#define STR_ERR_SOCK_HELO			TEXT("HELO�Ɏ��s���܂���\n")\
									TEXT("���M�����[���A�h���X���������ݒ肳��Ă��邩�m�F���Ă�������\n\n")
#define STR_ERR_SOCK_SMTPAUTH		TEXT("SMTP�T�[�o�̃��O�C���Ɏ��s���܂���\n\n")
#define STR_ERR_SOCK_RSET			TEXT("RSET�Ɏ��s���܂���\n\n")
#define STR_ERR_SOCK_MAILFROM		TEXT("MAIL FROM�Ɏ��s���܂���\n")\
									TEXT("���M�����[���A�h���X���������ݒ肳��Ă��邩�m�F���Ă�������\n\n")
#define STR_ERR_SOCK_NOTO			TEXT("���M�悪�ݒ肳��Ă��܂���")
#define STR_ERR_SOCK_RCPTTO			TEXT("RCPT TO�Ɏ��s���܂���\n")\
									TEXT("���M�惁�[���A�h���X���������ݒ肳��Ă��邩�m�F���Ă�������\n\n")
#define STR_ERR_SOCK_DATA			TEXT("DATA�Ɏ��s���܂���\n\n")
#define STR_ERR_SOCK_MAILSEND		TEXT("���[�����M�Ɏ��s���܂���\n\n")
#define STR_ERR_SOCK_SSL_INIT		TEXT("SSL�̏������Ɏ��s���܂���")
#define STR_ERR_SOCK_SSL_VERIFY		TEXT("�T�[�o�ؖ����̌��؂Ɏ��s���܂���")

// Question
#define STR_Q_DELETE				TEXT("�폜���Ă���낵���ł����H")
#define STR_Q_DELSERVERMAIL			TEXT("�T�[�o����폜����郁�[��������܂������s���܂����H")
#define STR_Q_DELLISTMAIL			TEXT("%d ���̃��[�����ꗗ����폜���Ă���낵���ł����H%s")
#define STR_Q_DELLISTMAIL_NOSERVER	TEXT("\n(�T�[�o����͍폜����܂���)")
#define STR_Q_DELMAILBOX			TEXT("���ݕ\������Ă���A�J�E���g���폜���Ă�낵���ł����H")
#define STR_Q_DELATTACH				TEXT("�Y�t�t�@�C�����폜���Ă���낵���ł����H")
#define STR_Q_COPY					TEXT("%d ���̃��[����ۑ����փR�s�[���Ă���낵���ł����H")
#define STR_Q_DEPENDENCE			TEXT("�@��ˑ����������݂��܂�����낵���ł����H")
#define STR_Q_UNLINKATTACH			TEXT("�Y�t�t�@�C���ւ̃����N���������Ă���낵���ł����H")
#define STR_Q_ADDADDRESS			TEXT("%d ���̃��[���A�h���X���A�h���X���ɒǉ����Ă���낵���ł����H")
#define STR_Q_NEXTFIND				TEXT("�Ō�܂Ō������܂���\n���߂��猟�����Ȃ����܂����H")
#define STR_Q_EDITCANSEL			TEXT("�ҏW���L�����Z�����Ă���낵���ł����H")
#define STR_Q_SENDMAIL				TEXT("���M���Ă���낵���ł����H")
#define STR_Q_ATTACH				TEXT("�Y�t�t�@�C���ɃE�B���X�Ȃǂ��܂܂�Ă���ꍇ�A\n")\
									TEXT("�R���s���[�^�ɔ�Q���y�ڂ��\��������܂��B\n\n")\
									TEXT("���s���Ă�낵���ł����H")

// Message
#define STR_MSG_NOMARK				TEXT("�}�[�N���ꂽ���[��������܂���")
#define STR_MSG_NOBODY				TEXT("�{�����擾����Ă��Ȃ����ߊJ�����Ƃ��ł��܂���\n\n")\
									TEXT("'��M�p�Ƀ}�[�N'��t����'�}�[�N�����s'���Ė{�����擾���Ă�������")
#define STR_MSG_NONEWMAIL			TEXT("�V�����[���͂���܂���")
#define STR_MSG_NOFIND				TEXT("\"%s\" ��������܂���")

// Window title
#define STR_TITLE_NOREADMAILBOX		TEXT("%s - [���ǃA�J�E���g: %d]")
#define STR_TITLE_MAILEDIT			TEXT("���[���ҏW")
#define STR_TITLE_MAILVIEW			TEXT("���[���\��")
#define STR_TITLE_MAILVIEW_COUNT	TEXT(" - [%d ����]")

// Message title
#define STR_TITLE_EXEC				TEXT("���s")
#define STR_TITLE_ALLEXEC			TEXT("������s")
#define STR_TITLE_SEND				TEXT("�����ɑ��M")
#define STR_TITLE_OPEN				TEXT("�J��")
#define STR_TITLE_SAVE				TEXT("�ۑ�")
#define STR_TITLE_COPY				TEXT("�R�s�[")
#define STR_TITLE_DELETE			TEXT("�폜")
#define STR_TITLE_ERROR				TEXT("�G���[")
#define STR_TITLE_SETMAILBOX		TEXT("�A�J�E���g�ݒ�")
#define STR_TITLE_OPTION			TEXT("�I�v�V����")
#define STR_TITLE_STARTPASSWORD		TEXT("�N��")
#define STR_TITLE_SHOWPASSWORD		TEXT("�\��")
#define STR_TITLE_FIND				TEXT("����")
#define STR_TITLE_ALLFIND			TEXT("\"%s\" �̌���")
#define STR_TITLE_ATTACH_MSG		TEXT("�J��")

// Window status
#define STR_STATUS_VIEWINFO			TEXT("�\�� %d ��")
#define STR_STATUS_MAILBOXINFO		TEXT("�\�� %d/ �T�[�o %d")
#define STR_STATUS_MAILINFO			TEXT("�V�� %d, ���J�� %d")

// Socket status
#define STR_STATUS_GETHOSTBYNAME	TEXT("�z�X�g��������...")
#define STR_STATUS_CONNECT			TEXT("�ڑ���...")
#define STR_STATUS_SSL				TEXT("SSL�ڑ���...")
#define STR_STATUS_SSL_COMPLETE		TEXT("SSL�ڑ�����")
#define STR_STATUS_RECV				TEXT("��M��...")
#define STR_STATUS_SENDBODY			TEXT("�{�����M��...")
#define STR_STATUS_SOCKINFO			TEXT("%d �o�C�g%s")
#define STR_STATUS_SOCKINFO_RECV	TEXT("��M")
#define STR_STATUS_SOCKINFO_SEND	TEXT("���M")
#define STR_STATUS_SEND_USER		TEXT("���[�U�����M")
#define STR_STATUS_SEND_PASS		TEXT("�p�X���[�h���M")

// Initialize status
#define STR_STATUS_INIT_MAILCNT		TEXT("%d ��")
#define STR_STATUS_INIT_MAILSIZE_B	TEXT("%s �o�C�g")
#define STR_STATUS_INIT_MAILSIZE_KB	TEXT("%s KB")

// Mail list
#define STR_SAVEBOX_NAME			TEXT("[�ۑ���]")
#define STR_SENDBOX_NAME			TEXT("[���M��]")
#define STR_MAILBOX_NONAME			TEXT("���̖��ݒ�")
#define STR_LIST_LVHEAD_SUBJECT		TEXT("����")
#define STR_LIST_LVHEAD_FROM		TEXT("���o�l")
#define STR_LIST_LVHEAD_TO			TEXT("���l")
#define STR_LIST_LVHEAD_DATE		TEXT("���t")
#define STR_LIST_LVHEAD_SIZE		TEXT("�T�C�Y")
#define STR_LIST_NOSUBJECT			TEXT("(no subject)")
#define STR_LIST_THREADSTR			TEXT("  + ")

#define STR_LIST_MENU_SENDINFO		TEXT("���M���(&R)...")
#define STR_LIST_MENU_REPLY			TEXT("�ԐM(&R)...")
#define STR_LIST_MENU_SENDMARK		TEXT("���M�p�Ƀ}�[�N(&M)\tCtrl+D")
#define STR_LIST_MENU_CREATECOPY	TEXT("�R�s�[�̍쐬(&C)\tCtrl+C")
#define STR_LIST_MENU_RECVMARK		TEXT("��M�p�Ƀ}�[�N(&M)\tCtrl+D")
#define STR_LIST_MENU_SAVEBOXCOPY	TEXT("�ۑ����փR�s�[(&C)\tCtrl+C")

// Mail view
#define STR_VIEW_HEAD_FROM			TEXT("���o�l: ")
#define STR_VIEW_HEAD_SUBJECT		TEXT("\r\n����: ")
#define STR_VIEW_HEAD_DATE			TEXT("\r\n���t: ")

#define STR_VIEW_MENU_ATTACH		TEXT("�Y�t�\��(&M)")
#define STR_VIEW_MENU_SOURCE		TEXT("�\�[�X�\��(&S)")
#define STR_VIEW_MENU_DELATTACH		TEXT("�Y�t�폜(&T)")

// Mail edit
#define STR_EDIT_HEAD_MAILBOX		TEXT("�A�J�E���g: ")
#define STR_EDIT_HEAD_TO			TEXT("\r\n����: ")
#define STR_EDIT_HEAD_SUBJECT		TEXT("\r\n����: ")

// SSL
#define STR_SSL_AUTO				TEXT("����")
#define STR_SSL_TLS10				TEXT("TLS 1.0")
#define STR_SSL_SSL30				TEXT("SSL 3.0")
#define STR_SSL_SSL20				TEXT("SSL 2.0")
#define STR_SSL_STARTTLS			TEXT("STARTTLS")
#define STR_SSL_TLS11				TEXT("TLS 1.1")
#define STR_SSL_TLS12				TEXT("TLS 1.2")
#define STR_SSL_TLS13				TEXT("TLS 1.3")

// SMTP-AUTH
#define STR_SMTP_AUTH_AUTO			TEXT("����")
#define STR_SMTP_AUTH_CRAM_MD5		TEXT("CRAM-MD5")
#define STR_SMTP_AUTH_LOGIN			TEXT("LOGIN")
#define STR_SMTP_AUTH_PLAIN			TEXT("PLAIN")

// Filter
#define STR_FILTER_USE				TEXT("�g�p")
#define STR_FILTER_NOUSE			TEXT("���g�p")
#define STR_FILTER_STATUS			TEXT("���")
#define STR_FILTER_ACTION			TEXT("����")
#define STR_FILTER_ITEM1			TEXT("����1")
#define STR_FILTER_CONTENT1			TEXT("���e1")
#define STR_FILTER_ITEM2			TEXT("����2")
#define STR_FILTER_CONTENT2			TEXT("���e2")

#define STR_FILTER_UNRECV			TEXT("��M���Ȃ�")
#define STR_FILTER_RECV				TEXT("��M����")
#define STR_FILTER_DOWNLOADMARK		TEXT("��M�p�Ƀ}�[�N")
#define STR_FILTER_DELETEMARK		TEXT("�폜�p�Ƀ}�[�N")
#define STR_FILTER_READICON			TEXT("�J���ς݂ɂ���")
#define STR_FILTER_SAVE				TEXT("�ۑ����փR�s�[")

// Cc list
#define STR_CCLIST_TYPE				TEXT("���")
#define STR_CCLIST_MAILADDRESS		TEXT("���[���A�h���X")

// Set send
#define STR_SETSEND_BTN_CC			TEXT("Cc, Bcc")
#define STR_SETSEND_BTN_ATTACH		TEXT("�Y�t�t�@�C��")
#define STR_SETSEND_BTN_ETC			TEXT("���̑�")

// Mail Prop
#define STR_MAILPROP_HEADER			TEXT("�w�b�_")
#define STR_MAILPROP_MAILADDRESS	TEXT("���[���A�h���X")

// Address list
#define STR_ADDRESSLIST_MAILADDRESS	TEXT("���[���A�h���X")
#define STR_ADDRESSLIST_COMMENT		TEXT("�R�����g")

#endif	//_INC_STR_TBL_H
/* End of source */
