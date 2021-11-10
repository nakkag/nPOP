/*
 * nPOP
 *
 * ssl.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

 /* Include Files */
#include <stdio.h>

#include "Memory.h"
#include "ssl.h"

#pragma comment(lib, "Secur32.lib")
#pragma comment(lib, "WSock32.Lib")
#pragma comment(lib, "Crypt32.Lib")

/* Define */

/* Global Variables */

/* Local Function Prototypes */

/*
 * ssl_init - SSLの初期化
 */
SSL* ssl_init()
{
	return mem_calloc(sizeof(SSL));
}

/*
 * ssl_free - SSLの解放
 */
void ssl_free(SSL* _ssl_info)
{
	if (_ssl_info != NULL) {
		if (_ssl_info->fContextInitialized) {
			DeleteSecurityContext(&(_ssl_info->hContext));
			_ssl_info->fContextInitialized = FALSE;
		}
		if (_ssl_info->fCredsInitialized) {
			FreeCredentialsHandle(&(_ssl_info->hClientCreds));
			_ssl_info->fCredsInitialized = FALSE;
		}
		if (_ssl_info->hMyCertStore) {
			CertCloseStore(_ssl_info->hMyCertStore, 0);
		}
		if (_ssl_info->pbIoBuffer != NULL) {
			mem_free(&(_ssl_info->pbIoBuffer));
		}
		mem_free(&_ssl_info);
	}
}

/*
 * ssl_create_credentials - クライアントの資格情報を作成
 */
SECURITY_STATUS ssl_create_credentials(SSL* _ssl_info, DWORD dwProtocol)
{
	TimeStamp tsExpiry;

	if (_ssl_info->hMyCertStore == NULL) {
		_ssl_info->hMyCertStore = CertOpenSystemStore(0, TEXT("MY"));
		if (!_ssl_info->hMyCertStore) {
			return SEC_E_NO_CREDENTIALS;
		}
	}

	ZeroMemory(&(_ssl_info->SchannelCred), sizeof(SCHANNEL_CRED));
	_ssl_info->SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
	_ssl_info->SchannelCred.grbitEnabledProtocols = dwProtocol;
	_ssl_info->SchannelCred.dwFlags = SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_MANUAL_CRED_VALIDATION;
	SECURITY_STATUS ret = AcquireCredentialsHandle(NULL,
		UNISP_NAME,
		SECPKG_CRED_OUTBOUND,
		NULL,
		&(_ssl_info->SchannelCred),
		NULL,
		NULL,
		&(_ssl_info->hClientCreds),
		&tsExpiry);
	if (ret) {
		_ssl_info->fCredsInitialized = TRUE;
	}
	return ret;
}

/*
 * ssl_new_client_credentials - クライアントの資格情報を取得
 */
static void ssl_new_client_credentials(SSL* _ssl_info)
{
	CredHandle hCreds;
	SecPkgContext_IssuerListInfoEx IssuerListInfo;
	PCCERT_CHAIN_CONTEXT pChainContext;
	CERT_CHAIN_FIND_BY_ISSUER_PARA FindByIssuerPara;
	PCCERT_CONTEXT pCertContext;
	TimeStamp tsExpiry;
	SECURITY_STATUS Status;

	if (QueryContextAttributes(&(_ssl_info->hContext), SECPKG_ATTR_ISSUER_LIST_EX, (PVOID)&IssuerListInfo) != SEC_E_OK) {
		return;
	}
	ZeroMemory(&FindByIssuerPara, sizeof(FindByIssuerPara));
	FindByIssuerPara.cbSize = sizeof(FindByIssuerPara);
	FindByIssuerPara.pszUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;
	FindByIssuerPara.dwKeySpec = 0;
	FindByIssuerPara.cIssuer = IssuerListInfo.cIssuers;
	FindByIssuerPara.rgIssuer = IssuerListInfo.aIssuers;

	pChainContext = NULL;
	while (TRUE) {
		pChainContext = CertFindChainInStore(_ssl_info->hMyCertStore,
			X509_ASN_ENCODING,
			0,
			CERT_CHAIN_FIND_BY_ISSUER,
			&FindByIssuerPara,
			pChainContext);
		if (pChainContext == NULL) {
			break;
		}

		pCertContext = pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;

		_ssl_info->SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
		_ssl_info->SchannelCred.cCreds = 1;
		_ssl_info->SchannelCred.paCred = &pCertContext;

		Status = AcquireCredentialsHandle(NULL,
			UNISP_NAME,
			SECPKG_CRED_OUTBOUND,
			NULL,
			&(_ssl_info->SchannelCred),
			NULL,
			NULL,
			&hCreds,
			&tsExpiry);
		if (Status != SEC_E_OK) {
			continue;
		}
		FreeCredentialsHandle(&(_ssl_info->hClientCreds));
		_ssl_info->hClientCreds = hCreds;
	}
}

/*
 * ssl_handshake - ハンドシェイクの開始
 */
SECURITY_STATUS ssl_handshake(SOCKET Socket, SSL* _ssl_info)
{

	SecBufferDesc OutBuffer;
	SecBuffer OutBuffers[1];
	DWORD dwSSPIFlags, dwSSPIOutFlags, cbData;
	TimeStamp tsExpiry;
	SECURITY_STATUS scRet;
	SecBuffer ExtraBuffer;

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY |
		ISC_RET_EXTENDED_ERROR | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

	OutBuffers[0].pvBuffer = NULL;
	OutBuffers[0].BufferType = SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer = 0;

	OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	scRet = InitializeSecurityContext(&(_ssl_info->hClientCreds),
		NULL,
		_ssl_info->pszServerName,
		dwSSPIFlags,
		0,
		SECURITY_NATIVE_DREP,
		NULL,
		0,
		&(_ssl_info->hContext),
		&OutBuffer,
		&dwSSPIOutFlags,
		&tsExpiry);
	if (scRet != SEC_I_CONTINUE_NEEDED) {
		return scRet;
	}

	if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL) {
		cbData = send(Socket, OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0);
		if (cbData == SOCKET_ERROR || cbData == 0) {
			FreeContextBuffer(OutBuffers[0].pvBuffer);
			DeleteSecurityContext(&(_ssl_info->hContext));
			return SEC_E_INTERNAL_ERROR;
		}
		FreeContextBuffer(OutBuffers[0].pvBuffer);
		OutBuffers[0].pvBuffer = NULL;
	}
	return ssl_handshake_loop(Socket, _ssl_info, TRUE, &ExtraBuffer);
}

/*
 * ssl_handshake_loop - 2回目以降のハンドシェイク
 */
SECURITY_STATUS ssl_handshake_loop(SOCKET Socket, SSL* _ssl_info, BOOL fDoInitialRead, SecBuffer* pExtraData)
{
	SecBufferDesc OutBuffer, InBuffer;
	SecBuffer InBuffers[2], OutBuffers[1];
	DWORD dwSSPIFlags, dwSSPIOutFlags, cbData, cbIoBuffer = 0;
	TimeStamp tsExpiry;
	SECURITY_STATUS scRet;
	PUCHAR IoBuffer;
	BOOL fDoRead;

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY |
		ISC_RET_EXTENDED_ERROR | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

	IoBuffer = mem_alloc(IO_BUFFER_SIZE);
	if (IoBuffer == NULL) {
		return SEC_E_INTERNAL_ERROR;
	}
	fDoRead = fDoInitialRead;

	scRet = SEC_I_CONTINUE_NEEDED;
	while (scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS) {
		if (0 == cbIoBuffer || scRet == SEC_E_INCOMPLETE_MESSAGE) {
			if (fDoRead) {
				cbData = recv(Socket, IoBuffer + cbIoBuffer, IO_BUFFER_SIZE - cbIoBuffer, 0);
				if (cbData == SOCKET_ERROR) {
#ifdef WSAASYNC
					if (WSAGetLastError() == WSAEWOULDBLOCK) {
						continue;
					}
#endif
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}
				else if (cbData == 0) {
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}
				cbIoBuffer += cbData;
			}
			else {
				fDoRead = TRUE;
			}
		}
		InBuffers[0].pvBuffer = IoBuffer;
		InBuffers[0].cbBuffer = cbIoBuffer;
		InBuffers[0].BufferType = SECBUFFER_TOKEN;

		InBuffers[1].pvBuffer = NULL;
		InBuffers[1].cbBuffer = 0;
		InBuffers[1].BufferType = SECBUFFER_EMPTY;

		InBuffer.cBuffers = 2;
		InBuffer.pBuffers = InBuffers;
		InBuffer.ulVersion = SECBUFFER_VERSION;

		OutBuffers[0].pvBuffer = NULL;
		OutBuffers[0].BufferType = SECBUFFER_TOKEN;
		OutBuffers[0].cbBuffer = 0;

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		scRet = InitializeSecurityContext(&(_ssl_info->hClientCreds),
			&(_ssl_info->hContext),
			NULL,
			dwSSPIFlags,
			0,
			SECURITY_NATIVE_DREP,
			&InBuffer,
			0,
			NULL,
			&OutBuffer,
			&dwSSPIOutFlags,
			&tsExpiry);
		if (scRet == SEC_E_OK || scRet == SEC_I_CONTINUE_NEEDED || FAILED(scRet) && (dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR)) {
			if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL) {
				cbData = send(Socket, OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0);
				if (cbData == SOCKET_ERROR || cbData == 0) {
					FreeContextBuffer(OutBuffers[0].pvBuffer);
					DeleteSecurityContext(&(_ssl_info->hContext));
					return SEC_E_INTERNAL_ERROR;
				}
				FreeContextBuffer(OutBuffers[0].pvBuffer);
				OutBuffers[0].pvBuffer = NULL;
			}
		}
		if (scRet == SEC_E_INCOMPLETE_MESSAGE) {
			continue;
		}
		if (scRet == SEC_E_OK) {
			if (InBuffers[1].BufferType == SECBUFFER_EXTRA) {
				pExtraData->pvBuffer = mem_alloc(InBuffers[1].cbBuffer);
				if (pExtraData->pvBuffer == NULL) {
					return SEC_E_INTERNAL_ERROR;
				}

				MoveMemory(pExtraData->pvBuffer,
					IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer),
					InBuffers[1].cbBuffer);

				pExtraData->cbBuffer = InBuffers[1].cbBuffer;
				pExtraData->BufferType = SECBUFFER_TOKEN;
			}
			else {
				pExtraData->pvBuffer = NULL;
				pExtraData->cbBuffer = 0;
				pExtraData->BufferType = SECBUFFER_EMPTY;
			}
			break;
		}
		if (FAILED(scRet)) {
			break;
		}
		if (scRet == SEC_I_INCOMPLETE_CREDENTIALS) {
			ssl_new_client_credentials(_ssl_info);
			fDoRead = FALSE;
			scRet = SEC_I_CONTINUE_NEEDED;
			continue;
		}
		if (InBuffers[1].BufferType == SECBUFFER_EXTRA) {
			MoveMemory(IoBuffer, IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer);
			cbIoBuffer = InBuffers[1].cbBuffer;
		}
		else {
			cbIoBuffer = 0;
		}
	}
	if (FAILED(scRet)) {
		DeleteSecurityContext(&(_ssl_info->hContext));
	}
	mem_free(&IoBuffer);
	return scRet;
}

/*
 * ssl_verify_certificate - サーバ証明書の検証
 */
DWORD ssl_verify_certificate(SSL* _ssl_info)
{
	HTTPSPolicyCallbackData polHttps;
	CERT_CHAIN_POLICY_PARA PolicyPara;
	CERT_CHAIN_POLICY_STATUS PolicyStatus;
	CERT_CHAIN_PARA ChainPara;
	PCCERT_CHAIN_CONTEXT pChainContext = NULL;
	DWORD Status;
	LPSTR rgszUsages[] = { szOID_PKIX_KP_SERVER_AUTH, szOID_SERVER_GATED_CRYPTO, szOID_SGC_NETSCAPE };
	DWORD cUsages = sizeof(rgszUsages) / sizeof(LPSTR);
#ifndef UNICODE
	PWSTR   pwszServerName = NULL;
	DWORD cchServerName;
#endif
	PCCERT_CONTEXT pRemoteCertContext = NULL;

	Status = QueryContextAttributes(&_ssl_info->hContext, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (PVOID)&pRemoteCertContext);
	if (Status != SEC_E_OK) {
		Status = SEC_E_WRONG_PRINCIPAL;
		goto cleanup;
	}

	if (_ssl_info->pszServerName == NULL || lstrlen(_ssl_info->pszServerName) == 0) {
		Status = SEC_E_WRONG_PRINCIPAL; goto cleanup;
	}

	ZeroMemory(&ChainPara, sizeof(ChainPara));
	ChainPara.cbSize = sizeof(ChainPara);
	ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
	ChainPara.RequestedUsage.Usage.cUsageIdentifier = cUsages;
	ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgszUsages;

	if (!CertGetCertificateChain(NULL,
		pRemoteCertContext,
		NULL,
		pRemoteCertContext->hCertStore,
		&ChainPara,
		0,
		NULL,
		&pChainContext)) {
		Status = GetLastError();
		goto cleanup;
	}

	ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
	polHttps.cbStruct = sizeof(HTTPSPolicyCallbackData);
	polHttps.dwAuthType = AUTHTYPE_SERVER;
	polHttps.fdwChecks = 0;
#ifndef UNICODE
	cchServerName = MultiByteToWideChar(CP_ACP, 0, _ssl_info->pszServerName, -1, NULL, 0);
	pwszServerName = mem_alloc(cchServerName * sizeof(WCHAR));
	if (pwszServerName == NULL) {
		Status = SEC_E_INSUFFICIENT_MEMORY; goto cleanup;
	}
	cchServerName = MultiByteToWideChar(CP_ACP, 0, _ssl_info->pszServerName, -1, pwszServerName, cchServerName);
	if (cchServerName == 0) {
		Status = SEC_E_WRONG_PRINCIPAL; goto cleanup;
	}
	polHttps.pwszServerName = pwszServerName;
#else
	polHttps.pwszServerName = _ssl_info->pszServerName;
#endif

	memset(&PolicyPara, 0, sizeof(PolicyPara));
	PolicyPara.cbSize = sizeof(PolicyPara);
	PolicyPara.pvExtraPolicyPara = &polHttps;

	memset(&PolicyStatus, 0, sizeof(PolicyStatus));
	PolicyStatus.cbSize = sizeof(PolicyStatus);

	if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,
		pChainContext,
		&PolicyPara,
		&PolicyStatus)) {
		Status = GetLastError();
		goto cleanup;
	}

	if (PolicyStatus.dwError) {
		Status = PolicyStatus.dwError;
		goto cleanup;
	}

	Status = SEC_E_OK;
cleanup:
	if (pChainContext) {
		CertFreeCertificateChain(pChainContext);
	}
#ifndef UNICODE
	if (pwszServerName) {
		mem_free(&pwszServerName);
	}
#endif
	if (pRemoteCertContext != NULL) {
		CertFreeCertificateContext(pRemoteCertContext);
	}
	return Status;
}

/*
 * ssl_send - データ送信
 */
DWORD ssl_send(SOCKET Socket, SSL* _ssl_info, PBYTE pbMessage, DWORD cbMessage)
{
	SECURITY_STATUS scRet;
	SecBufferDesc Message;
	SecBuffer Buffers[4];
	SecPkgContext_StreamSizes Sizes;
	DWORD cbIoBufferLength;
	PBYTE pbIoBuffer;
	ULONG send_size;

	// データの暗号化
	scRet = QueryContextAttributes(&(_ssl_info->hContext), SECPKG_ATTR_STREAM_SIZES, &Sizes);
	if (scRet != SEC_E_OK) {
		return -1;
	}
	send_size = (Sizes.cbMaximumMessage > cbMessage) ? cbMessage : Sizes.cbMaximumMessage;
	cbIoBufferLength = Sizes.cbHeader + send_size + Sizes.cbTrailer;
	pbIoBuffer = mem_alloc(cbIoBufferLength);
	if (pbIoBuffer == NULL) {
		return -1;
	}
	Buffers[0].pvBuffer = pbIoBuffer;
	Buffers[0].cbBuffer = Sizes.cbHeader;
	Buffers[0].BufferType = SECBUFFER_STREAM_HEADER;

	CopyMemory(pbIoBuffer + Sizes.cbHeader, pbMessage, send_size);
	Buffers[1].pvBuffer = pbIoBuffer + Sizes.cbHeader;
	Buffers[1].cbBuffer = send_size;
	Buffers[1].BufferType = SECBUFFER_DATA;

	Buffers[2].pvBuffer = pbIoBuffer + Sizes.cbHeader + send_size;
	Buffers[2].cbBuffer = Sizes.cbTrailer;
	Buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;

	Buffers[3].pvBuffer = SECBUFFER_EMPTY;
	Buffers[3].cbBuffer = SECBUFFER_EMPTY;
	Buffers[3].BufferType = SECBUFFER_EMPTY;

	Message.ulVersion = SECBUFFER_VERSION;
	Message.cBuffers = 4;
	Message.pBuffers = Buffers;
	scRet = EncryptMessage(&(_ssl_info->hContext), 0, &Message, 0);
	if (FAILED(scRet)) {
		mem_free(&pbIoBuffer);
		return -1;
	}

	// データの送信
	PBYTE p = pbIoBuffer;
	ULONG size = Buffers[0].cbBuffer + Buffers[1].cbBuffer + Buffers[2].cbBuffer;
	while (size > 0) {
		int len = send(Socket, p, size, 0);
		if (len == -1) {
			mem_free(&pbIoBuffer);
			return -1;
		}
		p += len;
		size -= len;
	}
	mem_free(&pbIoBuffer);
	return send_size;
}

/*
 * ssl_recv - データ受信
 */
DWORD ssl_recv(SOCKET Socket, SSL* _ssl_info, PBYTE pbMessage, DWORD cbMessage)
{
	SecBuffer ExtraBuffer;
	SecBuffer* pDataBuffer, * pExtraBuffer;
	SECURITY_STATUS scRet;
	SecBufferDesc Message;
	SecBuffer Buffers[4];
	DWORD cbIoBuffer, cbData, length = 0;
	SecPkgContext_StreamSizes Sizes;
	int i;
	DWORD cbIoBufferLength;
	PBYTE pbIoBuffer;

	scRet = QueryContextAttributes(&(_ssl_info->hContext), SECPKG_ATTR_STREAM_SIZES, &Sizes);
	if (scRet != SEC_E_OK) {
		return -1;
	}
	cbIoBufferLength = Sizes.cbHeader + Sizes.cbMaximumMessage + Sizes.cbTrailer;
	if (cbIoBufferLength > cbMessage) {
		cbIoBufferLength = cbMessage;
	}
	pbIoBuffer = mem_alloc(cbIoBufferLength);
	if (pbIoBuffer == NULL) {
		return -1;
	}

	cbIoBuffer = 0;
	scRet = 0;
	while (length == 0 || cbIoBuffer > 0) {
		if (cbIoBuffer == 0) {
			if (_ssl_info->cbIoBuffer != 0) {
				// 前回の未処理バッファを結合
				CopyMemory(pbIoBuffer, _ssl_info->pbIoBuffer, _ssl_info->cbIoBuffer);
				cbIoBuffer = _ssl_info->cbIoBuffer;
				mem_free(&(_ssl_info->pbIoBuffer));
				_ssl_info->pbIoBuffer = NULL;
				_ssl_info->cbIoBuffer = 0;
			}
			// データの受信
			cbData = recv(Socket, pbIoBuffer + cbIoBuffer, cbIoBufferLength - cbIoBuffer, 0);
			if (cbData == SOCKET_ERROR) {
				if (length != 0) {
					break;
				}
				mem_free(&pbIoBuffer);
				return -1;
			}
			else if (cbData == 0) {
				mem_free(&pbIoBuffer);
				return -1;
			}
			else {
				cbIoBuffer += cbData;
			}
		}

		// 複合化
		Buffers[0].pvBuffer = pbIoBuffer;
		Buffers[0].cbBuffer = cbIoBuffer;
		Buffers[0].BufferType = SECBUFFER_DATA;
		Buffers[1].BufferType = Buffers[2].BufferType = Buffers[3].BufferType = SECBUFFER_EMPTY;
		Message.ulVersion = SECBUFFER_VERSION;
		Message.cBuffers = 4;
		Message.pBuffers = Buffers;
		scRet = DecryptMessage(&(_ssl_info->hContext), &Message, 0, NULL);
		if (scRet == SEC_I_CONTEXT_EXPIRED) {
			mem_free(&pbIoBuffer);
			return -1;
		}
		if (scRet == SEC_E_INCOMPLETE_MESSAGE) {
			// データが小さく複合化できないため次の受信で処理する
			_ssl_info->pbIoBuffer = pbIoBuffer;
			_ssl_info->cbIoBuffer = cbIoBuffer;
			return length;
		}
		if (scRet != SEC_E_OK && scRet != SEC_I_RENEGOTIATE) {
			mem_free(&pbIoBuffer);
			return -1;
		}
		pDataBuffer = NULL;
		pExtraBuffer = NULL;
		for (i = 1; i < 4; i++) {
			if (pDataBuffer == NULL && Buffers[i].BufferType == SECBUFFER_DATA) {
				pDataBuffer = &Buffers[i];
			}
			if (pExtraBuffer == NULL && Buffers[i].BufferType == SECBUFFER_EXTRA) {
				pExtraBuffer = &Buffers[i];
			}
		}
		if (pDataBuffer) {
			// データのコピー
			CopyMemory(pbMessage + length, pDataBuffer->pvBuffer, pDataBuffer->cbBuffer);
			length += pDataBuffer->cbBuffer;
		}
		if (pExtraBuffer) {
			// 次のデータ
			MoveMemory(pbIoBuffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
			cbIoBuffer = pExtraBuffer->cbBuffer;
		}
		else {
			cbIoBuffer = 0;
		}
		if (scRet == SEC_I_RENEGOTIATE) {
			// 再度ハンドシェイクを行う
			scRet = ssl_handshake_loop(Socket, _ssl_info, FALSE, &ExtraBuffer);
			if (scRet != SEC_E_OK) {
				mem_free(&pbIoBuffer);
				return -1;
			}
			if (ExtraBuffer.pvBuffer) {
				MoveMemory(pbIoBuffer, ExtraBuffer.pvBuffer, ExtraBuffer.cbBuffer);
				cbIoBuffer = ExtraBuffer.cbBuffer;
			}
		}
	}
	mem_free(&pbIoBuffer);
	return length;
}

/*
 * ssl_close - SSL接続を閉じる
 */
LONG ssl_close(SOCKET Socket, SSL* _ssl_info)
{
	PBYTE pbMessage;
	DWORD dwType, dwSSPIFlags, dwSSPIOutFlags, cbMessage, cbData, Status;
	SecBufferDesc OutBuffer;
	SecBuffer OutBuffers[1];
	TimeStamp tsExpiry;

	dwType = SCHANNEL_SHUTDOWN;

	OutBuffers[0].pvBuffer = &dwType;
	OutBuffers[0].BufferType = SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer = sizeof(dwType);

	OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	Status = ApplyControlToken(&(_ssl_info->hContext), &OutBuffer);
	if (FAILED(Status)) {
		DeleteSecurityContext(&(_ssl_info->hContext));
		_ssl_info->fContextInitialized = FALSE;
		return Status;
	}

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT |
		ISC_REQ_REPLAY_DETECT |
		ISC_REQ_CONFIDENTIALITY |
		ISC_RET_EXTENDED_ERROR |
		ISC_REQ_ALLOCATE_MEMORY |
		ISC_REQ_STREAM;

	OutBuffers[0].pvBuffer = NULL;
	OutBuffers[0].BufferType = SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer = 0;

	OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	Status = InitializeSecurityContextA(&(_ssl_info->hClientCreds),
		&(_ssl_info->hContext),
		NULL,
		dwSSPIFlags,
		0,
		SECURITY_NATIVE_DREP,
		NULL,
		0,
		&(_ssl_info->hContext),
		&OutBuffer,
		&dwSSPIOutFlags,
		&tsExpiry);
	if (FAILED(Status)) {
		DeleteSecurityContext(&(_ssl_info->hContext));
		_ssl_info->fContextInitialized = FALSE;
		return Status;
	}

	pbMessage = OutBuffers[0].pvBuffer;
	cbMessage = OutBuffers[0].cbBuffer;
	if (pbMessage != NULL && cbMessage != 0) {
		cbData = send(Socket, pbMessage, cbMessage, 0);
		if (cbData == SOCKET_ERROR || cbData == 0) {
			Status = WSAGetLastError();
			DeleteSecurityContext(&(_ssl_info->hContext));
			_ssl_info->fContextInitialized = FALSE;
			return Status;
		}
		FreeContextBuffer(pbMessage);
	}
	DeleteSecurityContext(&(_ssl_info->hContext));
	_ssl_info->fContextInitialized = FALSE;
	return Status;
}

/*
 * get_protocol - 接続プロトコル名の取得
 */
void get_protocol(SSL* _ssl_info, TCHAR *buf)
{
	SECURITY_STATUS Status;
	SecPkgContext_ConnectionInfo ConnectionInfo;

	Status = QueryContextAttributes(&(_ssl_info->hContext), SECPKG_ATTR_CONNECTION_INFO, (PVOID)&ConnectionInfo);
	if (Status != SEC_E_OK) {
		*buf = TEXT('\0');
		return;
	}
	switch (ConnectionInfo.dwProtocol)
	{
	case SP_PROT_TLS1_CLIENT:
		lstrcpy(buf, TEXT("TLS1.0"));
		break;

	case SP_PROT_TLS1_1_CLIENT:
		lstrcpy(buf, TEXT("TLS1.1"));
		break;

	case SP_PROT_TLS1_2_CLIENT:
		lstrcpy(buf, TEXT("TLS1.2"));
		break;

	case SP_PROT_TLS1_3_CLIENT:
		lstrcpy(buf, TEXT("TLS1.3"));
		break;

	case SP_PROT_SSL3_CLIENT:
		lstrcpy(buf, TEXT("SSL3.0"));
		break;

	case SP_PROT_SSL2_CLIENT:
		lstrcpy(buf, TEXT("SSL2.0"));
		break;

	case SP_PROT_PCT1_CLIENT:
		lstrcpy(buf, TEXT("PCT"));
		break;

	default:
		wsprintf(buf, TEXT("0x%X"), ConnectionInfo.dwProtocol);
	}

}
/* End of source */
