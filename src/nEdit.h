/*
 * nEdit
 *
 * nEdit.h
 *
 * Copyright (C) 1996-2005 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_NEDIT_H
#define _INC_NEDIT_H

/* Include Files */
#include <windows.h>
#include <tchar.h>
#ifdef OP_XP_STYLE
#include <uxtheme.h>
#include <tmschema.h>
#endif	// OP_XP_STYLE

/* Define */
#define NEDIT_WND_CLASS					TEXT("nEdit")

#define WM_GETBUFFERINFO				(WM_APP + 1)
#define WM_REFLECT						(WM_APP + 2)
#define WM_GETWORDWRAP					(WM_APP + 3)
#define WM_SETWORDWRAP					(WM_APP + 4)
#define WM_GETMEMSIZE					(WM_APP + 5)
#define WM_GETMEM						(WM_APP + 6)
#define WM_SETMEM						(WM_APP + 7)

#define EM_GETREADONLY					(WM_APP + 100)

#ifndef EM_REDO
#define EM_REDO							(WM_USER + 84)
#endif
#ifndef EM_CANREDO
#define EM_CANREDO						(WM_USER + 85)
#endif

#define UNDO_TYPE_INPUT					1
#define UNDO_TYPE_DELETE				2

/* Struct */
typedef struct _UNDO {
	BYTE type;

	DWORD st;
	DWORD len;
	TCHAR *buf;
} UNDO;

typedef struct _BUFFER {
	// �ێ����Ă�����e
	TCHAR *buf;
	DWORD buf_size;
	DWORD buf_len;

	// ���̓o�b�t�@
	TCHAR *input_buf;
	DWORD input_size;
	DWORD input_len;

	// �\���s���̃I�t�Z�b�g
	DWORD *line;
	int line_size;
	int line_len;
	int line_add_index;
	int line_add_len;

	// UNDO�o�b�t�@
	UNDO *undo;
	int undo_size;
	int undo_len;
	int undo_pos;

	// ���͊J�n�ʒu
	TCHAR *ip;
	DWORD ip_len;
	// �폜�J�n�ʒu
	TCHAR *dp;
	DWORD del_len;

	// �L�����b�g�̈ʒu
	DWORD cp;
	// �I���ʒu
	DWORD sp;
	// �㉺�ړ����̃L�����b�g��X���W
	int cpx;

	// 1�s�̕�����
	int line_max;
	// �s�̍ő啝
	int line_width;
	// �E�B���h�E�̕�
	int width;

	// �X�N���[�� �o�[
	int pos_x;
	int max_x;
	int pos_y;
	int max_y;

	// �^�u�X�g�b�v
	int tab_stop;
	// ���}�[�W��
	int left_margin;
	// ��}�[�W��
	int top_margin;
	// �E�}�[�W��
	int right_margin;
	// ���}�[�W��
	int bottom_margin;
	// �s��
	int spacing;

	// �`��p���
	HDC mdc;
	HBITMAP ret_bmp;
	HRGN hrgn;
	HFONT hfont;
	HFONT ret_font;

	// �t�H���g
	int font_height;
	int char_width;

	// �܂�Ԃ��t���O
	BOOL wordwrap;
	// �t�H�[�J�X�������Ă��I��\��
	BOOL no_hide_sel;
	// �������ɕϊ�
	BOOL lowercase;
	// �啶���ɕϊ�
	BOOL uppercase;
	// ���b�N�t���O
	BOOL lock;
	// �C���t���O
	BOOL modified;
	// ���̓��[�h
	BOOL insert_mode;
	// �I���t���O
	BOOL sel;
	// �}�E�X���t���O
	BOOL mousedown;
	// ���͒�����
	DWORD limit_len;

	// �����̕�
	BYTE cwidth[256];

	// �R���g���[�����ʎq
	int id;
	// IME
	HIMC himc;
#ifdef OP_XP_STYLE
	// XP
	HMODULE hModThemes;
	HTHEME hTheme;
#endif	// OP_XP_STYLE
} BUFFER;

/* Function Prototypes */
BOOL nedit_regist(const HINSTANCE hInstance);

#endif
/* End of source */
