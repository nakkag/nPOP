/*
 * nPOP
 *
 * ssl.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#pragma once
#define SECURITY_WIN32

 /* Include Files */
#include <windows.h>
#include <winsock.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <schannel.h>
#include <security.h>
#include <sspi.h>

/* Define */
#define IO_BUFFER_SIZE  0x10000

#ifndef SP_PROT_TLS1_3_CLIENT
#define SP_PROT_TLS1_3_CLIENT           0x00002000
#endif

/* Struct */
typedef struct _SSL {
	CredHandle hClientCreds;
	CtxtHandle hContext;

	HCERTSTORE hMyCertStore;
	SCHANNEL_CRED SchannelCred;

	BOOL fCredsInitialized;
	BOOL fContextInitialized;

	TCHAR *pszServerName;
	DWORD ssl_type;
	int verify;

	PBYTE pbIoBuffer;
	DWORD cbIoBuffer;
} SSL;

/* Function Prototypes */
SSL* ssl_init();
void ssl_free(SSL* __ssl_info);

SECURITY_STATUS ssl_create_credentials(SSL* __ssl_info, DWORD dwProtocol);
SECURITY_STATUS ssl_handshake_loop(SOCKET Socket, SSL* __ssl_info, BOOL fDoInitialRead, SecBuffer* pExtraData);
SECURITY_STATUS ssl_handshake(SOCKET Socket, SSL* __ssl_info);
DWORD ssl_verify_certificate(SSL* __ssl_info);
LONG ssl_close(SOCKET Socket, SSL* __ssl_info);

DWORD ssl_send(SOCKET Socket, SSL* __ssl_info, PBYTE pbMessage, DWORD cbMessage);
DWORD ssl_recv(SOCKET Socket, SSL* __ssl_info, PBYTE pbMessage, DWORD cbMessage);

void get_protocol(SSL* _ssl_info, TCHAR* buf);
/* End of source */
