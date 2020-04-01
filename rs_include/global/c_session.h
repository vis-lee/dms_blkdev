/*
 * c_session.h
 *
 *  Created on: 2010/6/20
 *      Author: root
 */

#ifndef C_SESSION_H_
#define C_SESSION_H_

#include <sqlrelay/sqlrclientwrapper.h>
#include "api_version.h"

typedef struct CSession
{
	sqlrcon conn;
	int count;
	int isConnected;
}c_session;


int initSession(c_session *, const char *, int, const char *, const char *);
int closeSession(c_session *);
int checkConnection(c_session *);
int checkVersioning(c_session *);
void setCommitMode(c_session *, int);
void setDebugMode(c_session *, int);
void transactionBegin(c_session *);
void sessionCommit(c_session *);
void sessionRollBack(c_session *);

#endif /* C_SESSION_H_ */
