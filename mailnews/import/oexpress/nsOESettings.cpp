/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/*

  Outlook Express (Win32) settings

*/

#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsOEImport.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIImportService.h"
#include "nsOERegUtil.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgAccountManager.h"
#include "nsIMsgAccount.h"
#include "nsIImportSettings.h"
#include "nsOESettings.h"
#include "nsMsgBaseCID.h"
#include "nsMsgCompCID.h"
#include "nsISmtpService.h"
#include "nsISmtpServer.h"

#include "OEDebugLog.h"

static NS_DEFINE_IID(kISupportsIID,        	NS_ISUPPORTS_IID);
static NS_DEFINE_CID(kComponentManagerCID, 	NS_COMPONENTMANAGER_CID);
static NS_DEFINE_CID(kMsgMailSessionCID,	NS_MSGMAILSESSION_CID);
static NS_DEFINE_CID(kSmtpServiceCID,		NS_SMTPSERVICE_CID); 


class OESettings {
public:
	static HKEY	Find50Key( void);
	static HKEY	Find40Key( void);
	static HKEY FindAccountsKey( void);

	static PRBool DoImport( nsIMsgAccount **ppAccount);

	static PRBool DoIMAPServer( nsIMsgAccountManager *pMgr, HKEY hKey, char *pServerName, nsIMsgAccount **ppAccount);
	static PRBool DoPOP3Server( nsIMsgAccountManager *pMgr, HKEY hKey, char *pServerName, nsIMsgAccount **ppAccount);
	
	static void SetIdentities( nsIMsgAccountManager *pMgr, nsIMsgAccount *pAcc, HKEY hKey);
	static PRBool IdentityMatches( nsIMsgIdentity *pIdent, const char *pName, const char *pServer, const char *pEmail, const char *pReply, const char *pUserName);

	static void SetSmtpServer( nsIMsgAccountManager *pMgr, nsIMsgAccount *pAcc, char *pServer, char *pUser);

};


////////////////////////////////////////////////////////////////////////
nsresult nsOESettings::Create(nsIImportSettings** aImport)
{
    NS_PRECONDITION(aImport != nsnull, "null ptr");
    if (! aImport)
        return NS_ERROR_NULL_POINTER;

    *aImport = new nsOESettings();
    if (! *aImport)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aImport);
    return NS_OK;
}

nsOESettings::nsOESettings()
{
    NS_INIT_REFCNT();
}

nsOESettings::~nsOESettings()
{
}

NS_IMPL_ISUPPORTS(nsOESettings, nsIImportSettings::GetIID());

NS_IMETHODIMP nsOESettings::AutoLocate(PRUnichar **description, nsIFileSpec **location, PRBool *_retval)
{
    NS_PRECONDITION(description != nsnull, "null ptr");
    NS_PRECONDITION(_retval != nsnull, "null ptr");
	if (!description || !_retval)
		return( NS_ERROR_NULL_POINTER);
	
	nsString	desc = "Outlook Express";
	*description = nsCRT::strdup( desc.GetUnicode());
	*_retval = PR_FALSE;

	if (location)
		*location = nsnull;
	HKEY	key;
	key = OESettings::Find50Key();
	if (key != nsnull) {
		*_retval = PR_TRUE;
		::RegCloseKey( key);
	}
	else {
		key = OESettings::Find40Key();
		if (key != nsnull) {
			*_retval = PR_TRUE;
			::RegCloseKey( key);
		}
	}
	if (*_retval) {
		key = OESettings::FindAccountsKey();
		if (key == nsnull) {
			*_retval = PR_FALSE;
		}
		else {
			::RegCloseKey( key);
		}
	}

	return( NS_OK);
}

NS_IMETHODIMP nsOESettings::SetLocation(nsIFileSpec *location)
{
	return( NS_OK);
}

NS_IMETHODIMP nsOESettings::Import(nsIMsgAccount **localMailAccount, PRBool *_retval)
{
	NS_PRECONDITION( _retval != nsnull, "null ptr");
	
	if (OESettings::DoImport( localMailAccount)) {
		*_retval = PR_TRUE;
		IMPORT_LOG0( "Settings import appears successful\n");
	}
	else {
		*_retval = PR_FALSE;
		IMPORT_LOG0( "Settings import returned FALSE\n");
	}

	return( NS_OK);
}


HKEY OESettings::FindAccountsKey( void)
{
	HKEY	sKey;
	if (::RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Account Manager\\Accounts", 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &sKey) == ERROR_SUCCESS) {
		return( sKey);
	}

	return( nsnull);
}

HKEY OESettings::Find50Key( void)
{
	PRBool		success = PR_FALSE;
	HKEY		sKey;

	if (::RegOpenKeyEx( HKEY_CURRENT_USER, "Identities", 0, KEY_QUERY_VALUE, &sKey) == ERROR_SUCCESS) {
		BYTE *	pBytes = nsOERegUtil::GetValueBytes( sKey, "Default User ID");
		::RegCloseKey( sKey);
		if (pBytes) {
			nsCString	key( "Identities\\");
			key += (const char *)pBytes;
			nsOERegUtil::FreeValueBytes( pBytes);
			key += "\\Software\\Microsoft\\Outlook Express\\5.0";
			if (::RegOpenKeyEx( HKEY_CURRENT_USER, key, 0, KEY_QUERY_VALUE, &sKey) == ERROR_SUCCESS) {
				return( sKey);
			}
		}
	}

	return( nsnull);
}

HKEY OESettings::Find40Key( void)
{
	HKEY	sKey;
	if (::RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Outlook Express", 0, KEY_QUERY_VALUE, &sKey) == ERROR_SUCCESS) {
		return( sKey);
	}

	return( nsnull);
}


PRBool OESettings::DoImport( nsIMsgAccount **ppAccount)
{
	HKEY	hKey = FindAccountsKey();
	if (hKey == nsnull) {
		IMPORT_LOG0( "*** Error finding Outlook Express registry account keys\n");
		return( PR_FALSE);
	}

	nsresult	rv;
	
    NS_WITH_SERVICE(nsIMsgMailSession, mailSession, kMsgMailSessionCID, &rv);
    if (NS_FAILED(rv)) {
		IMPORT_LOG0( "*** Failed to create a mail session!\n");
		return( PR_FALSE);
	}
	nsCOMPtr<nsIMsgAccountManager> accMgr;
	rv = mailSession->GetAccountManager( getter_AddRefs( accMgr));
	if (NS_FAILED( rv)) {
		IMPORT_LOG0( "*** Failed to get account manager\n");
		return( PR_FALSE);
	}


	HKEY		subKey;
	nsCString	defMailName;

	// First let's get the default mail account key name
	if (::RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Outlook Express", 0, KEY_QUERY_VALUE, &subKey) == ERROR_SUCCESS) {
		BYTE *	pBytes = nsOERegUtil::GetValueBytes( subKey, "Default Mail Account");
		::RegCloseKey( subKey);
		if (pBytes) {
			defMailName = (const char *)pBytes;
			nsOERegUtil::FreeValueBytes( pBytes);
		}
	}

	// Iterate the accounts looking for POP3 & IMAP accounts...
	// Ignore LDAP & NNTP for now!
	DWORD		index = 0;
	DWORD		numChars;
	TCHAR		keyName[256];
	FILETIME	modTime;
	LONG		result = ERROR_SUCCESS;
	BYTE *		pBytes;
	int			popCount = 0;
	int			accounts = 0;
	nsCString	keyComp;

	while (result == ERROR_SUCCESS) {
		numChars = 256;
		result = ::RegEnumKeyEx( hKey, index, keyName, &numChars, NULL, NULL, NULL, &modTime);
		index++;
		if (result == ERROR_SUCCESS) {
			if (::RegOpenKeyEx( hKey, keyName, 0, KEY_QUERY_VALUE, &subKey) == ERROR_SUCCESS) {
				// Get the values for this account.
				IMPORT_LOG1( "Opened Outlook Express account: %s\n", (char *)keyName);

				nsIMsgAccount	*anAccount = nsnull;
				pBytes = nsOERegUtil::GetValueBytes( subKey, "IMAP Server");
				if (pBytes) {
					if (DoIMAPServer( accMgr, subKey, (char *)pBytes, &anAccount))
						accounts++;
					nsOERegUtil::FreeValueBytes( pBytes);
				}

				pBytes = nsOERegUtil::GetValueBytes( subKey, "POP3 Server");
				if (pBytes) {
					if (popCount == 0) {
						if (DoPOP3Server( accMgr, subKey, (char *)pBytes, &anAccount)) {
							popCount++;
							accounts++;
							if (ppAccount && anAccount) {
								*ppAccount = anAccount;
								NS_ADDREF( anAccount);
							}
						}
					}
					else {
						if (DoPOP3Server( accMgr, subKey, (char *)pBytes, &anAccount)) {
							popCount++;
							accounts++;
							// If we created a mail account, get rid of it since
							// we have 2 POP accounts!
							if (ppAccount && *ppAccount) {
								NS_RELEASE( *ppAccount);
								*ppAccount = nsnull;
							}
						}
					}
					nsOERegUtil::FreeValueBytes( pBytes);
				}
				
				if (anAccount) {
					// Is this the default account?
					keyComp = keyName;
					if (keyComp.Equals( defMailName)) {
						accMgr->SetDefaultAccount( anAccount);
					}
					NS_RELEASE( anAccount);
				}

				::RegCloseKey( subKey);
			}
		}
	}

	return( accounts != 0);
}



PRBool OESettings::DoIMAPServer( nsIMsgAccountManager *pMgr, HKEY hKey, char *pServerName, nsIMsgAccount **ppAccount)
{
	if (ppAccount)
		*ppAccount = nsnull;

	BYTE *pBytes;
	pBytes = nsOERegUtil::GetValueBytes( hKey, "IMAP User Name");
	if (!pBytes)
		return( PR_FALSE);

	PRBool	result = PR_FALSE;

	// I now have a user name/server name pair, find out if it already exists?
	nsCOMPtr<nsIMsgIncomingServer>	in;
	nsresult rv = pMgr->FindServer( (const char *)pBytes, pServerName, "imap", getter_AddRefs( in));
	if (NS_FAILED( rv) || (in == nsnull)) {
		// Create the incoming server and an account for it?
		rv = pMgr->CreateIncomingServer( "imap", getter_AddRefs( in));
		if (NS_SUCCEEDED( rv) && in) {
			rv = in->SetHostName( pServerName);
			rv = in->SetUsername( (char *)pBytes);
			BYTE *pAccName = nsOERegUtil::GetValueBytes( hKey, "Account Name");
			nsString	prettyName;
			if (pAccName) {
				prettyName = (const char *)pAccName;
				nsOERegUtil::FreeValueBytes( pAccName);
			}
			else
				prettyName = (const char *)pServerName;

			PRUnichar *pretty = prettyName.ToNewUnicode();
			rv = in->SetPrettyName( pretty);
			nsCRT::free( pretty);
			
			// We have a server, create an account.
			nsCOMPtr<nsIMsgAccount>	account;
			rv = pMgr->CreateAccount( getter_AddRefs( account));
			if (NS_SUCCEEDED( rv) && account) {
				rv = account->SetIncomingServer( in);	
				// Fiddle with the identities
				SetIdentities( pMgr, account, hKey);
				result = PR_TRUE;
				if (ppAccount)
					account->QueryInterface( nsIMsgAccount::GetIID(), (void **)ppAccount);
			}				
		}
	}
	else
		result = PR_TRUE;
	
	nsOERegUtil::FreeValueBytes( pBytes);

	return( result);
}

PRBool OESettings::DoPOP3Server( nsIMsgAccountManager *pMgr, HKEY hKey, char *pServerName, nsIMsgAccount **ppAccount)
{
	if (ppAccount)
		*ppAccount = nsnull;

	BYTE *pBytes;
	pBytes = nsOERegUtil::GetValueBytes( hKey, "POP3 User Name");
	if (!pBytes)
		return( PR_FALSE);

	PRBool	result = PR_FALSE;

	// I now have a user name/server name pair, find out if it already exists?
	nsCOMPtr<nsIMsgIncomingServer>	in;
	nsresult rv = pMgr->FindServer( (const char *)pBytes, pServerName, "pop3", getter_AddRefs( in));
	if (NS_FAILED( rv) || (in == nsnull)) {
		// Create the incoming server and an account for it?
		rv = pMgr->CreateIncomingServer( "pop3", getter_AddRefs( in));
		if (NS_SUCCEEDED( rv) && in) {
			rv = in->SetHostName( pServerName);
			rv = in->SetUsername( (char *)pBytes);
			BYTE *pAccName = nsOERegUtil::GetValueBytes( hKey, "Account Name");
			nsString	prettyName;
			if (pAccName) {
				prettyName = (const char *)pAccName;
				nsOERegUtil::FreeValueBytes( pAccName);
			}
			else
				prettyName = (const char *)pServerName;

			PRUnichar *pretty = prettyName.ToNewUnicode();
			rv = in->SetPrettyName( pretty);
			nsCRT::free( pretty);
			
			// We have a server, create an account.
			nsCOMPtr<nsIMsgAccount>	account;
			rv = pMgr->CreateAccount( getter_AddRefs( account));
			if (NS_SUCCEEDED( rv) && account) {
				rv = account->SetIncomingServer( in);	
				// Fiddle with the identities
				SetIdentities( pMgr, account, hKey);
				result = PR_TRUE;
				if (ppAccount)
					account->QueryInterface( nsIMsgAccount::GetIID(), (void **)ppAccount);
			}				
		}
	}
	else
		result = PR_TRUE;
	
	nsOERegUtil::FreeValueBytes( pBytes);

	return( result);
}


PRBool OESettings::IdentityMatches( nsIMsgIdentity *pIdent, const char *pName, const char *pServer, const char *pEmail, const char *pReply, const char *pUserName)
{
	if (!pIdent)
		return( PR_FALSE);

	char *	pIName = nsnull;
	char *	pIEmail = nsnull;
	char *	pIReply = nsnull;
	
	PRBool	result = PR_TRUE;

	// The test here is:
	// If the smtp host is the same
	//	and the email address is the same (if it is supplied)
	//	and the reply to address is the same (if it is supplied)
	//	then we match regardless of the full name.

	nsresult rv = pIdent->GetFullName( &pIName);
	rv = pIdent->GetEmail( &pIEmail);
	rv = pIdent->GetReplyTo( &pIReply);

	// for now, if it's the same server and reply to and email then it matches
	if (pReply) {
		if (!pIReply || nsCRT::strcasecmp( pReply, pIReply))
			result = PR_FALSE;
	}
	if (pEmail) {
		if (!pIEmail || nsCRT::strcasecmp( pEmail, pIEmail))
			result = PR_FALSE;
	}

	nsCRT::free( pIName);
	nsCRT::free( pIEmail);
	nsCRT::free( pIReply);

	return( result);
}

void OESettings::SetIdentities( nsIMsgAccountManager *pMgr, nsIMsgAccount *pAcc, HKEY hKey)
{
	// Get the relevant information for an identity
	char *pName = (char *)nsOERegUtil::GetValueBytes( hKey, "SMTP Display Name");
	char *pServer = (char *)nsOERegUtil::GetValueBytes( hKey, "SMTP Server");
	char *pEmail = (char *)nsOERegUtil::GetValueBytes( hKey, "SMTP Email Address");
	char *pReply = (char *)nsOERegUtil::GetValueBytes( hKey, "SMTP Reply To Email Address");
	char *pUserName = (char *)nsOERegUtil::GetValueBytes( hKey, "SMTP User Name");

	nsresult	rv;

	if (pEmail && pName && pServer) {
		// The default identity, nor any other identities matched,
		// create a new one and add it to the account.
		nsCOMPtr<nsIMsgIdentity>	id;
		rv = pMgr->CreateIdentity( getter_AddRefs( id));
		if (id) {
			id->SetFullName( pName);
			id->SetIdentityName( pName);
			id->SetEmail( pEmail);
			if (pReply)
				id->SetReplyTo( pReply);
			pAcc->AddIdentity( id);
		}
	}
	
	SetSmtpServer( pMgr, pAcc, pServer, pUserName);

	nsOERegUtil::FreeValueBytes( (BYTE *)pName);
	nsOERegUtil::FreeValueBytes( (BYTE *)pServer);
	nsOERegUtil::FreeValueBytes( (BYTE *)pEmail);
	nsOERegUtil::FreeValueBytes( (BYTE *)pReply);
	nsOERegUtil::FreeValueBytes( (BYTE *)pUserName);
}

void OESettings::SetSmtpServer( nsIMsgAccountManager *pMgr, nsIMsgAccount *pAcc, char *pServer, char *pUser)
{
	nsresult	rv;

/*
	NS_WITH_SERVICE(nsISmtpService, smtpService, kSmtpServiceCID, &rv); 
	if (NS_SUCCEEDED(rv) && smtpService) {
		nsCOMPtr<nsISmtpServer>		smtpServer;
	
		rv = smtpService->CreateSmtpServer( getter_AddRefs( smtpServer));
		if (NS_SUCCEEDED( rv) && smtpServer) {
			smtpServer->SetHostname( pServer);
			smtpServer->SetUsername( pUser);
		}
 	}
*/
       /*
		nsXPIDLCString				hostName;
        nsXPIDLCString				senderName;
		smtpServer->GetHostname( getter_Copies(hostName));
		smtpServer->GetUsername( getter_Copies(senderName));
		*/
}


