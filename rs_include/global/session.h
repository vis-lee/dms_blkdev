/*
 * session.h
 *
 *  Created on: 2010/6/17
 *      Author: root
 */

#ifndef SESSION_H_
#define SESSION_H_

#include <sqlrelay/sqlrclient.h>
#include "../global/ccma.h"
#include "../global/api_version.h"
#include "../biz/boobject.h"
#include "../biz/bonode.h"
#include "../biz/bovdc.h"
#include "../biz/bovc.h"
#include "../biz/boinst.h"
#include "../biz/bovolume.h"
#include "../biz/bolb_policy.h"
#include "../biz/bolb_policy_rule.h"
#include "../biz/bobackup.h"
#include "../biz/bonetwork.h"
#include "../biz/bobackup_policy.h"
#include "../biz/boip_policy_rule.h"
#include "../biz/bovm_pm.h"
#include <semaphore.h>
#include <pthread.h>
#include <string.h>

using namespace std;

class Session
{
	private:
	char cstr_host_name[128];
	char cstr_account[128];
	char cstr_pwd[128];
	sqlrconnection *conn;
	int count;
	int isConnected;
	int errorCode;
	pthread_mutex_t work_mutex;
	public:
	void beginTransaction();
	void versionChecking();
	void setCommitMode(bool);
	void commit();
	void rollback();
	void setDebugMode(bool);
	void setErrorCode(int);
	int getErrorCode(); //check api version
	int getConnectStatus();
	sqlrconnection* getConnection();
	BoObject* createBo(int);
	Session(std::string host_name, int port, std::string account, std::string pwd);
	virtual ~Session();
};

#endif /* SESSION_H_ */
