/*
 * nPOP
 *
 * WinSock.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include "General.h"
#include "Memory.h"
#include "String.h"

/* Define */
#define CRLF_LEN				2

#define RECV_SIZE				32768		// ��M�o�b�t�@�T�C�Y
#define SEND_SIZE				4096		// ���M���̕����T�C�Y

/* Global Variables */
static char *recv_buf;						// ��M�o�b�t�@
static char *old_buf;						// �������o�b�t�@
static int old_buf_len;
static int old_buf_size;

SSL *ssl;
static TCHAR* ssl_server;
static int ssl_type;
static int ssl_verify;

// �O���Q��
extern OPTION op;

/* Local Function Prototypes */

/*
 * get_host_by_name - �z�X�g������IP�A�h���X���擾����
 */
unsigned long get_host_by_name(HWND hWnd, TCHAR *server, TCHAR *ErrStr)
{
	unsigned long ret;
	LPHOSTENT lpHostEnt;
#ifdef UNICODE
	char *HostName;
#endif

	SetSocStatusTextT(hWnd, STR_STATUS_GETHOSTBYNAME);
	if (server == NULL || *server == TEXT('\0')) {
		lstrcpy(ErrStr, STR_ERR_SOCK_NOSERVER);
		return 0;
	}
#ifdef UNICODE
	HostName = alloc_tchar_to_char(server);
	if (HostName == NULL) {
		lstrcpy(ErrStr, STR_ERR_MEMALLOC);
		return 0;
	}
	ret = inet_addr(HostName);
	if (ret == -1) {
		lpHostEnt = gethostbyname(HostName);
		if (lpHostEnt != NULL) {
			ret = ((struct in_addr *)lpHostEnt->h_addr_list[0])->s_addr;
		} else {
			lstrcpy(ErrStr, STR_ERR_SOCK_GETIPADDR);
			ret = 0;
		}
	}
	mem_free(&HostName);
#else
	ret = inet_addr(server);
	if (ret == -1) {
		lpHostEnt = gethostbyname(server);
		if (lpHostEnt != NULL) {
			ret = ((struct in_addr *)lpHostEnt->h_addr_list[0])->s_addr;
		} else {
			lstrcpy(ErrStr, STR_ERR_SOCK_GETIPADDR);
			ret = 0;
		}
	}
#endif
	return ret;
}

/*
 * connect_server - �T�[�o�ɐڑ�����
 */
SOCKET connect_server(HWND hWnd, unsigned long ip_addr, unsigned short port, TCHAR* server, const int use_ssl, const int _ssl_type, const int _ssl_verify, TCHAR* ErrStr)
{
	SOCKET soc;
	struct sockaddr_in serversockaddr;

	SetSocStatusTextT(hWnd, STR_STATUS_CONNECT);

	if (use_ssl == 1 && _ssl_type != 4) {
		ssl = ssl_init();
	}
	ssl_server = server;
	ssl_type = _ssl_type;
	ssl_verify = _ssl_verify;

	// �\�P�b�g�̍쐬
	soc = socket(PF_INET, SOCK_STREAM, 0);
	if (soc == INVALID_SOCKET) {
		lstrcpy(ErrStr, STR_ERR_SOCK_CREATESOCKET);
		return -1;
	}
	// �ڑ���̐ݒ�
	serversockaddr.sin_family = AF_INET;
	serversockaddr.sin_addr.s_addr = ip_addr;
	serversockaddr.sin_port = htons((unsigned short)port);
	ZeroMemory(serversockaddr.sin_zero, sizeof(serversockaddr.sin_zero));
#ifdef WSAASYNC
	if (WSAAsyncSelect(soc, hWnd, WM_SOCK_SELECT, FD_CONNECT | FD_READ | FD_CLOSE) == SOCKET_ERROR) {
		lstrcpy(ErrStr, STR_ERR_SOCK_EVENT);
		return -1;
	}
#endif
	// �ڑ�
	if (connect(soc, (struct sockaddr*) & serversockaddr, sizeof(serversockaddr)) == SOCKET_ERROR) {
#ifdef WSAASYNC
		if (WSAGetLastError() == WSAEWOULDBLOCK) {
			return soc;
		}
#endif
		lstrcpy(ErrStr, STR_ERR_SOCK_CONNECT);
		return -1;
	}
#ifndef WSAASYNC
	// SSL�̏�����
	if (init_ssl(hWnd, soc, ErrStr) == -1) {
		return -1;
	}
#endif
	return soc;
}

/*
 * recv_proc - ��M���ĉ��s�P�ʂŏ�������
 */
int recv_proc(HWND hWnd, SOCKET soc)
{
	char *buf;
	char *line;
	char *p;
	int buf_len;
	int len;

	// ��M�p�o�b�t�@�̊m��
	if (recv_buf == NULL && (recv_buf = (char *)mem_alloc(RECV_SIZE)) == NULL) {
		return SELECT_MEM_ERROR;
	}
	// ��M
	if (ssl == NULL) {
		buf_len = recv(soc, recv_buf, RECV_SIZE, 0);
	} else {
		buf_len = ssl_recv(soc, ssl, recv_buf, RECV_SIZE);
	}
	if (buf_len == SOCKET_ERROR || buf_len == 0) {
#ifdef WSAASYNC
		if (WSAGetLastError() == WSAEWOULDBLOCK) {
			return SELECT_SOC_SUCCEED;
		}
#endif
		return SELECT_SOC_CLOSE;
	}
	buf = recv_buf;
	// ������������Ǝ�M�����������
	if (old_buf_len > 0 && old_buf != NULL) {
		if (old_buf_size < old_buf_len + buf_len) {
			old_buf_size += buf_len;
			buf = (char *)mem_alloc(old_buf_size);
			if (buf == NULL) {
				return SELECT_MEM_ERROR;
			}
			CopyMemory(buf, old_buf, old_buf_len);
			CopyMemory(buf + old_buf_len, recv_buf, buf_len);
			mem_free(&old_buf);
			old_buf = buf;
		} else {
			buf = old_buf;
			CopyMemory(old_buf + old_buf_len, recv_buf, buf_len);
		}
		buf_len += old_buf_len;
	}
	// �s�P�ʂɏ���
	p = buf;
	while (1) {
		// ��s���o
		line = p;
		for (len = 0; (p - buf) < buf_len; p++, len++) {
			if (*p == '\r' && (p - buf) + 1 < buf_len && *(p + 1) == '\n') {
				break;
			}
		}
		if ((p - buf) >= buf_len) {
			break;
		}
		*p = '\0';
		p += CRLF_LEN;
		// �E�B���h�E�ɕ������n��
		if (SendMessage(hWnd, WM_SOCK_RECV, len, (LPARAM)line) == FALSE) {
			return SELECT_SOC_SUCCEED;
		}
	}
	// �������̕������ۑ�
	old_buf_len = len;
	if (old_buf_len > 0) {
		if (old_buf == buf) {
			MoveMemory(old_buf, line, len);
		} else if (old_buf != NULL && old_buf_size >= len) {
			CopyMemory(old_buf, line, len);
		} else {
			old_buf_size += len;
			buf = (char *)mem_alloc(old_buf_size);
			if (buf == NULL) {
				return SELECT_MEM_ERROR;
			}
			CopyMemory(buf, line, len);
			mem_free(&old_buf);
			old_buf = buf;
		}
	}
	return SELECT_SOC_SUCCEED;
}

/*
 * recv_select - ��M�\�ȃo�b�t�@������ꍇ�Ɏ�M���s��
 */
#ifndef WSAASYNC
int recv_select(HWND hWnd, SOCKET soc)
{
#define TIMEOUT			0		// �^�C���A�E�g�b��
	struct timeval waittime;
	fd_set rdps;
	int selret;

	waittime.tv_sec = TIMEOUT;
	waittime.tv_usec = 0;

	ZeroMemory(&rdps, sizeof(fd_set));
	FD_ZERO(&rdps);
	FD_SET(soc, &rdps);
	selret = select(FD_SETSIZE, &rdps, (fd_set *)0, (fd_set *)0, &waittime);
	if (selret == SOCKET_ERROR) {
		return SELECT_SOC_ERROR;
	}
	if (selret == 0 || FD_ISSET(soc, &rdps) == FALSE) {
		return SELECT_SOC_NODATA;
	}
	return recv_proc(hWnd, soc);
}
#endif

/*
 * send_data - ������̑��M
 */
int send_data(SOCKET soc, char *wbuf, int len)
{
	// �f�[�^���M
	if (ssl == NULL) {
		return send(soc, wbuf, len, 0);
	}
	return ssl_send(soc, ssl, wbuf, len);
}

/*
 * send_buf - ������̑��M
 */
int send_buf(SOCKET soc, char *buf)
{
#define TIMEOUT			0
	struct timeval waittime;
	fd_set rdps;
	char *send_buf;
	int send_len;
	int selret;
	int len;

	// ���M�o�b�t�@�̐ݒ�
	send_buf = buf;
	send_len = tstrlen(send_buf);
	// �^�C���A�E�g�ݒ�
	waittime.tv_sec = TIMEOUT;
	waittime.tv_usec = 0;

	while (*send_buf != '\0') {
		// ���M�o�b�t�@�̋���m�F
		ZeroMemory(&rdps, sizeof(fd_set));
		FD_ZERO(&rdps);
		FD_SET(soc, &rdps);
		selret = select(FD_SETSIZE, (fd_set *)0, &rdps, (fd_set *)0, &waittime);
		if (selret == SOCKET_ERROR) {
			return -1;
		}
		if (selret == 0 || FD_ISSET(soc, &rdps) == FALSE) {
			continue;
		}
		// �������đ��M
		len = (send_len - (send_buf - buf) < SEND_SIZE) ? send_len - (send_buf - buf) : SEND_SIZE;
		if (ssl == NULL) {
			len = send(soc, send_buf, len, 0);
		} else {
			len = ssl_send(soc, ssl, send_buf, len);
		}
		if (len == -1) {
			return -1;
		}
		send_buf += len;
	}
	return 0;
}

/*
 * send_buf_t - TCHAR�^�̕�����̑��M
 */
#ifdef UNICODE
int send_buf_t(SOCKET soc, TCHAR *wbuf)
{
	char *p;
	int ret;

	p = alloc_tchar_to_char(wbuf);
	if (p == NULL) {
		return -1;
	}
	ret = send_buf(soc, p);
	mem_free(&p);
	return ret;
}
#endif

/*
 * socket_close - �ʐM�I��
 */
void socket_close(HWND hWnd, SOCKET soc)
{
	if (ssl != NULL) {
		// SSL�̉��
		ssl_free(ssl);
		ssl = NULL;
	}
	if (soc != -1) {
#ifdef WSAASYNC
		WSAAsyncSelect(soc, hWnd, 0, 0);
#endif
		// �ؒf
		shutdown(soc, 2);
		closesocket(soc);
	}
	// POP3���̉��
	pop3_free();
	// SMTP���̉��
	smtp_free();

	// ��M�o�b�t�@�̉��
	mem_free(&recv_buf);
	recv_buf = NULL;
	// �������o�b�t�@�̉��
	mem_free(&old_buf);
	old_buf = NULL;
	old_buf_len = 0;
	old_buf_size = 0;
}

int init_ssl(const HWND hWnd, const SOCKET soc, TCHAR* ErrStr)
{
	if (ssl == NULL) {
		return 0;
	}
	SetSocStatusTextT(hWnd, STR_STATUS_SSL);

	ssl->pszServerName = ssl_server;
	ssl->verify = ssl_verify;
	switch (ssl_type) {
	case 0:
		ssl->ssl_type = SP_PROT_ALL;
		break;
	case 1:
		ssl->ssl_type = SP_PROT_TLS1_0_CLIENT;
		break;
	case 2:
		ssl->ssl_type = SP_PROT_SSL3_CLIENT;
		break;
	case 3:
		ssl->ssl_type = SP_PROT_SSL2_CLIENT;
		break;
	case 4:
		ssl->ssl_type = SP_PROT_ALL;
		break;
	case 5:
		ssl->ssl_type = SP_PROT_TLS1_1_CLIENT;
		break;
	case 6:
		ssl->ssl_type = SP_PROT_TLS1_2_CLIENT;
		break;
	case 7:
		ssl->ssl_type = SP_PROT_TLS1_3_CLIENT;
		break;
	}
	SECURITY_STATUS ret = ssl_create_credentials(ssl, ssl->ssl_type);
	if (ret) {
		wsprintf(ErrStr, TEXT("%s (1: 0x%X)"), STR_ERR_SOCK_SSL_INIT, ret);
		return -1;
	}
	ret = ssl_handshake(soc, ssl);
	if (ret) {
		wsprintf(ErrStr, TEXT("%s (2: 0x%X)"), STR_ERR_SOCK_SSL_INIT, ret);
		return -1;
	}
	if (ssl->verify) {
		ret = ssl_verify_certificate(ssl);
		if (ret) {
			wsprintf(ErrStr, TEXT("%s (0x%X)"), STR_ERR_SOCK_SSL_VERIFY, ret);
			return -1;
		}
	}

	TCHAR buf[BUF_SIZE];
	get_protocol(ssl, buf);
	TCHAR msg[BUF_SIZE];
	wsprintf(msg, TEXT("%s (%s)"), STR_STATUS_SSL_COMPLETE, buf);
	SetSocStatusTextT(hWnd, msg);
	return 0;
}
/* End of source */
